#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, NamedTuple


ROOT = Path(__file__).resolve().parents[1]
BUILD_WORKFLOW = ROOT / ".github" / "workflows" / "build.yml"
ARTIFACT_PREFIX = ".arc-artifacts/"
BASE_EVIDENCE = (
    "source-manifest.json",
    "third-party-manifest.json",
    "safety-case.json",
    "release-evidence.json",
    "workflow-pins.json",
    "workflow-policy.json",
    "npm-lock.json",
    "license-policy.json",
    "secret-scan.json",
    "sbom.spdx.json",
)
PROVENANCE = "provenance.intoto.json"
INDEX = "evidence-index.json"
EXPECTED_GENERATED = (*BASE_EVIDENCE, PROVENANCE, INDEX)
EXPECTED_PROVENANCE_SUBJECTS = BASE_EVIDENCE
EXPECTED_INDEXED = (*BASE_EVIDENCE, PROVENANCE)
EXPECTED_UPLOADED = (INDEX, *BASE_EVIDENCE, PROVENANCE)
FIRMWARE_MANIFEST = "firmware-manifest.json"
FIRMWARE_INDEX = "firmware-evidence-index.json"
EXPECTED_FIRMWARE_GENERATED = (FIRMWARE_MANIFEST, FIRMWARE_INDEX)
EXPECTED_FIRMWARE_INDEXED = (FIRMWARE_MANIFEST,)
EXPECTED_FIRMWARE_UPLOADED = (FIRMWARE_INDEX, FIRMWARE_MANIFEST)
EXPECTED_FIRMWARE_BINARY_PATTERNS = (
    "build/*.bin",
    "build/*.elf",
    "examples/**/build/*.bin",
    "examples/**/build/*.elf",
)
ARTIFACT_RE = re.compile(r"\.arc-artifacts/[A-Za-z0-9_.-]+\.json")


class Step(NamedTuple):
    name: str
    line: int
    lines: list[str]


def artifact_path(name: str) -> str:
    return f"{ARTIFACT_PREFIX}{name}"


def artifact_name(path: str) -> str:
    return path.removeprefix(ARTIFACT_PREFIX)


def leading_spaces(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def read_lines(path: Path) -> list[str]:
    return path.read_text(encoding="utf-8").splitlines()


def find_step(lines: list[str], name: str) -> Step | None:
    pattern = re.compile(rf"^(?P<indent>\s*)-\s+name:\s+{re.escape(name)}\s*$")
    for index, line in enumerate(lines):
        match = pattern.match(line)
        if not match:
            continue
        indent = len(match.group("indent"))
        block = [line]
        for child in lines[index + 1 :]:
            if child.strip() and leading_spaces(child) <= indent and re.match(r"^\s*-\s+name:\s+", child):
                break
            block.append(child)
        return Step(name=name, line=index + 1, lines=block)
    return None


def artifact_refs(lines: list[str]) -> list[str]:
    return [artifact_name(match.group(0)) for line in lines for match in ARTIFACT_RE.finditer(line)]


def firmware_binary_patterns(lines: list[str]) -> list[str]:
    expected = set(EXPECTED_FIRMWARE_BINARY_PATTERNS)
    return [line.strip() for line in lines if line.strip() in expected]


def output_artifacts(lines: list[str]) -> list[str]:
    outputs: list[str] = []
    for line in lines:
        for match in re.finditer(r"(?:>\s*|--output\s+)(\.arc-artifacts/[A-Za-z0-9_.-]+\.json)", line):
            outputs.append(artifact_name(match.group(1)))
    return outputs


def line_index(lines: list[str], needle: str) -> int | None:
    for index, line in enumerate(lines):
        if needle in line:
            return index
    return None


def command_segment(lines: list[str], start: str, end: str | None = None) -> list[str]:
    start_index = line_index(lines, start)
    if start_index is None:
        return []
    end_index = len(lines)
    if end is not None:
        for index in range(start_index + 1, len(lines)):
            if end in lines[index]:
                end_index = index
                break
    return lines[start_index:end_index]


def provenance_subjects(step: Step | None) -> list[str]:
    if step is None:
        return []
    segment = command_segment(step.lines, "./tools/provenance.py", "./tools/evidence-index.py")
    refs: list[str] = []
    for line in segment:
        if "--output" in line:
            continue
        refs.extend(artifact_refs([line]))
    return refs


def index_requirements(step: Step | None) -> list[str]:
    if step is None:
        return []
    segment = command_segment(step.lines, "./tools/evidence-index.py", "./tools/evidence-bundle-check.py")
    requirements: list[str] = []
    for line in segment:
        match = re.search(r"--require\s+(\.arc-artifacts/[A-Za-z0-9_.-]+\.json)", line)
        if match:
            requirements.append(artifact_name(match.group(1)))
    return requirements


def index_inputs(step: Step | None) -> list[str]:
    if step is None:
        return []
    segment = command_segment(step.lines, "./tools/evidence-index.py", "./tools/evidence-bundle-check.py")
    inputs: list[str] = []
    for line in segment:
        if "--require" in line or "--output" in line or "./tools/evidence-index.py" in line:
            continue
        inputs.extend(artifact_refs([line]))
    return inputs


def compare_artifacts(label: str, actual: list[str], expected: tuple[str, ...], problems: list[str]) -> None:
    for name in expected:
        if name not in actual:
            problems.append(f"{label}: missing required artifact: {name}")
    for name in actual:
        if name not in expected:
            problems.append(f"{label}: unexpected artifact: {name}")
    seen: set[str] = set()
    for name in actual:
        if name in seen:
            problems.append(f"{label}: duplicate artifact: {name}")
        seen.add(name)
    if set(actual) == set(expected) and actual != list(expected):
        problems.append(f"{label}: artifacts must use canonical order: {', '.join(expected)}")


def collect(root: Path = ROOT) -> dict[str, Any]:
    root = root.resolve()
    workflow = root / ".github" / "workflows" / "build.yml"
    problems: list[str] = []
    if not workflow.is_file():
        return {
            "workflow": workflow.relative_to(root).as_posix(),
            "ok": False,
            "problems": [".github/workflows/build.yml: missing build workflow"],
        }

    lines = read_lines(workflow)
    repository_step = find_step(lines, "Repository evidence bundle")
    upload_step = find_step(lines, "Upload repository evidence")
    firmware_step = find_step(lines, "Firmware artifact manifest")
    firmware_upload_step = find_step(lines, "Upload binaries")
    if repository_step is None:
        problems.append(".github/workflows/build.yml: missing Repository evidence bundle step")
    if upload_step is None:
        problems.append(".github/workflows/build.yml: missing Upload repository evidence step")
    if firmware_step is None:
        problems.append(".github/workflows/build.yml: missing Firmware artifact manifest step")
    if firmware_upload_step is None:
        problems.append(".github/workflows/build.yml: missing Upload binaries step")

    generated = output_artifacts(repository_step.lines if repository_step else [])
    subjects = provenance_subjects(repository_step)
    required = index_requirements(repository_step)
    indexed = index_inputs(repository_step)
    uploaded = artifact_refs(upload_step.lines if upload_step else [])
    firmware_generated = output_artifacts(firmware_step.lines if firmware_step else [])
    firmware_required = index_requirements(firmware_step)
    firmware_indexed = index_inputs(firmware_step)
    firmware_uploaded = artifact_refs(firmware_upload_step.lines if firmware_upload_step else [])
    firmware_binaries = firmware_binary_patterns(firmware_upload_step.lines if firmware_upload_step else [])

    compare_artifacts("repository evidence generated outputs", generated, EXPECTED_GENERATED, problems)
    compare_artifacts("repository evidence provenance subjects", subjects, EXPECTED_PROVENANCE_SUBJECTS, problems)
    compare_artifacts("repository evidence index requirements", required, EXPECTED_INDEXED, problems)
    compare_artifacts("repository evidence index inputs", indexed, EXPECTED_INDEXED, problems)
    compare_artifacts("repository evidence upload paths", uploaded, EXPECTED_UPLOADED, problems)
    compare_artifacts("firmware evidence generated outputs", firmware_generated, EXPECTED_FIRMWARE_GENERATED, problems)
    compare_artifacts("firmware evidence index requirements", firmware_required, EXPECTED_FIRMWARE_INDEXED, problems)
    compare_artifacts("firmware evidence index inputs", firmware_indexed, EXPECTED_FIRMWARE_INDEXED, problems)
    compare_artifacts("firmware evidence upload paths", firmware_uploaded, EXPECTED_FIRMWARE_UPLOADED, problems)
    compare_artifacts("firmware binary upload patterns", firmware_binaries, EXPECTED_FIRMWARE_BINARY_PATTERNS, problems)

    if repository_step is not None:
        index_line = line_index(repository_step.lines, "./tools/evidence-index.py")
        bundle_line = line_index(repository_step.lines, "./tools/evidence-bundle-check.py .arc-artifacts")
        if bundle_line is None:
            problems.append("repository evidence bundle check: missing evidence-bundle-check invocation")
        elif index_line is None or bundle_line <= index_line:
            problems.append("repository evidence bundle check: must run after evidence-index generation")
    if upload_step is not None:
        upload_text = "\n".join(upload_step.lines)
        if "uses: actions/upload-artifact@" not in upload_text:
            problems.append("repository evidence upload: must use actions/upload-artifact")
        if "name: arc-evidence" not in upload_text:
            problems.append("repository evidence upload: artifact name must be arc-evidence")
        if "if-no-files-found: error" not in upload_text:
            problems.append("repository evidence upload: if-no-files-found must be error")
        if "retention-days: 30" not in upload_text:
            problems.append("repository evidence upload: retention-days must be 30")
    if firmware_step is not None:
        index_line = line_index(firmware_step.lines, "./tools/evidence-index.py")
        manifest_line = line_index(firmware_step.lines, "./tools/firmware-manifest.py")
        if manifest_line is None:
            problems.append("firmware evidence manifest: missing firmware-manifest invocation")
        elif index_line is None or index_line <= manifest_line:
            problems.append("firmware evidence index: must run after firmware-manifest generation")
    if firmware_upload_step is not None:
        upload_text = "\n".join(firmware_upload_step.lines)
        if "if: steps.firmware-plan.outputs.count != '0'" not in upload_text:
            problems.append("firmware evidence upload: must be gated on selected firmware builds")
        if "uses: actions/upload-artifact@" not in upload_text:
            problems.append("firmware evidence upload: must use actions/upload-artifact")
        if "name: arc-binaries" not in upload_text:
            problems.append("firmware evidence upload: artifact name must be arc-binaries")
        if "if-no-files-found: error" not in upload_text:
            problems.append("firmware evidence upload: if-no-files-found must be error")
        if "retention-days: 30" not in upload_text:
            problems.append("firmware evidence upload: retention-days must be 30")

    return {
        "workflow": workflow.relative_to(root).as_posix(),
        "repository_step_line": repository_step.line if repository_step else None,
        "upload_step_line": upload_step.line if upload_step else None,
        "firmware_step_line": firmware_step.line if firmware_step else None,
        "firmware_upload_step_line": firmware_upload_step.line if firmware_upload_step else None,
        "expected_generated": list(EXPECTED_GENERATED),
        "expected_provenance_subjects": list(EXPECTED_PROVENANCE_SUBJECTS),
        "expected_indexed": list(EXPECTED_INDEXED),
        "expected_uploaded": list(EXPECTED_UPLOADED),
        "expected_firmware_generated": list(EXPECTED_FIRMWARE_GENERATED),
        "expected_firmware_indexed": list(EXPECTED_FIRMWARE_INDEXED),
        "expected_firmware_uploaded": list(EXPECTED_FIRMWARE_UPLOADED),
        "expected_firmware_binary_patterns": list(EXPECTED_FIRMWARE_BINARY_PATTERNS),
        "generated": generated,
        "provenance_subjects": subjects,
        "index_requirements": required,
        "index_inputs": indexed,
        "uploaded": uploaded,
        "firmware_generated": firmware_generated,
        "firmware_index_requirements": firmware_required,
        "firmware_index_inputs": firmware_indexed,
        "firmware_uploaded": firmware_uploaded,
        "firmware_binary_patterns": firmware_binaries,
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc evidence workflow contract",
        f"- workflow: {evidence['workflow']}",
        f"- generated: {len(evidence.get('generated', []))}",
        f"- provenance subjects: {len(evidence.get('provenance_subjects', []))}",
        f"- index requirements: {len(evidence.get('index_requirements', []))}",
        f"- index inputs: {len(evidence.get('index_inputs', []))}",
        f"- uploaded: {len(evidence.get('uploaded', []))}",
        f"- firmware generated: {len(evidence.get('firmware_generated', []))}",
        f"- firmware uploaded: {len(evidence.get('firmware_uploaded', []))}",
        f"- firmware binary patterns: {len(evidence.get('firmware_binary_patterns', []))}",
    ]
    if evidence["problems"]:
        lines.append("- problems:")
        for problem in evidence["problems"]:
            lines.append(f"  - {problem}")
    return "\n".join(lines)


def payload_text(evidence: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(evidence, indent=2, sort_keys=True)
    return report_text(evidence)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate Arc repository evidence workflow artifact contract.")
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="repository root to scan",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON evidence",
    )
    args = parser.parse_args(argv)

    evidence = collect(args.root)
    print(payload_text(evidence, args.format))
    if evidence["problems"]:
        print("arc evidence workflow contract failed: repository evidence workflow drifted", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
