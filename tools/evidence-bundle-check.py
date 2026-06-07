#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ARTIFACT_DIR = ROOT / ".arc-artifacts"
INDEX_NAME = "evidence-index.json"
PROVENANCE_NAME = "provenance.intoto.json"
SBOM_NAME = "sbom.spdx.json"
EXPECTED_INDEXED = (
    "source-manifest.json",
    "third-party-manifest.json",
    "safety-case.json",
    "release-evidence.json",
    "workflow-pins.json",
    "workflow-policy.json",
    "dependabot-policy.json",
    "npm-lock.json",
    "license-policy.json",
    "secret-scan.json",
    SBOM_NAME,
    PROVENANCE_NAME,
)
EXPECTED_PROVENANCE_SUBJECTS = tuple(name for name in EXPECTED_INDEXED if name != PROVENANCE_NAME)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def load_json(path: Path, problems: list[str]) -> Any:
    try:
        with path.open(encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        problems.append(f"{path.name}: cannot load JSON: {exc}")
        return None


def artifact_path(artifact_dir: Path, name: str) -> str:
    return (artifact_dir / name).relative_to(ROOT).as_posix() if (artifact_dir / name).is_relative_to(ROOT) else name


def expect_object(payload: Any, name: str, problems: list[str]) -> dict[str, Any]:
    if isinstance(payload, dict):
        return payload
    problems.append(f"{name}: JSON root must be an object")
    return {}


def indexed_records(index: dict[str, Any], artifact_dir: Path, problems: list[str]) -> dict[str, dict[str, Any]]:
    if index.get("ok") is not True:
        problems.append("evidence-index.json: ok must be true")
    if index.get("problems") not in ([], None):
        problems.append("evidence-index.json: problems must be empty")

    records: dict[str, dict[str, Any]] = {}
    files = index.get("files")
    if not isinstance(files, list):
        problems.append("evidence-index.json: files must be a list")
        return records
    for item in files:
        if not isinstance(item, dict):
            problems.append("evidence-index.json: file record must be an object")
            continue
        path = item.get("path")
        if not isinstance(path, str):
            problems.append("evidence-index.json: file record path must be a string")
            continue
        name = Path(path).name
        if name in records:
            problems.append(f"evidence-index.json: duplicate indexed file: {name}")
            continue
        records[name] = item
        expected_path = artifact_path(artifact_dir, name)
        if path != expected_path:
            problems.append(f"evidence-index.json: {name} path must be {expected_path}")
        full = artifact_dir / name
        if not full.is_file():
            problems.append(f"evidence-index.json: indexed file missing on disk: {name}")
            continue
        expected_sha = item.get("sha256")
        actual_sha = sha256(full)
        if expected_sha != actual_sha:
            problems.append(f"evidence-index.json: {name} sha256 mismatch")
        state = item.get("json")
        if isinstance(state, dict):
            if state.get("valid") is not True:
                problems.append(f"evidence-index.json: {name} JSON validity must be true")
            if state.get("ok") is False:
                problems.append(f"evidence-index.json: {name} indexed ok field is false")
            if state.get("problem_count") not in (None, 0):
                problems.append(f"evidence-index.json: {name} indexed problem count must be zero")

    missing = sorted(set(EXPECTED_INDEXED).difference(records))
    for name in missing:
        problems.append(f"evidence-index.json: missing required indexed evidence: {name}")
    extra = sorted(set(records).difference(EXPECTED_INDEXED))
    for name in extra:
        problems.append(f"evidence-index.json: unexpected indexed evidence: {name}")
    return records


def subject_records(provenance: dict[str, Any], artifact_dir: Path, problems: list[str]) -> dict[str, dict[str, Any]]:
    if provenance.get("_type") != "https://in-toto.io/Statement/v1":
        problems.append("provenance.intoto.json: _type must be in-toto Statement v1")
    if provenance.get("predicateType") != "https://slsa.dev/provenance/v1":
        problems.append("provenance.intoto.json: predicateType must be SLSA provenance v1")

    subjects = provenance.get("subject")
    if not isinstance(subjects, list):
        problems.append("provenance.intoto.json: subject must be a list")
        return {}

    records: dict[str, dict[str, Any]] = {}
    for item in subjects:
        if not isinstance(item, dict):
            problems.append("provenance.intoto.json: subject record must be an object")
            continue
        path = item.get("name")
        digest = item.get("digest")
        if not isinstance(path, str) or not isinstance(digest, dict):
            problems.append("provenance.intoto.json: subject must include name and digest")
            continue
        name = Path(path).name
        if name in records:
            problems.append(f"provenance.intoto.json: duplicate subject: {name}")
            continue
        records[name] = item
        expected_path = artifact_path(artifact_dir, name)
        if path != expected_path:
            problems.append(f"provenance.intoto.json: {name} subject path must be {expected_path}")
        full = artifact_dir / name
        if not full.is_file():
            problems.append(f"provenance.intoto.json: subject file missing on disk: {name}")
            continue
        if digest.get("sha256") != sha256(full):
            problems.append(f"provenance.intoto.json: {name} subject sha256 mismatch")

    missing = sorted(set(EXPECTED_PROVENANCE_SUBJECTS).difference(records))
    for name in missing:
        problems.append(f"provenance.intoto.json: missing required subject: {name}")
    extra = sorted(set(records).difference(EXPECTED_PROVENANCE_SUBJECTS))
    for name in extra:
        problems.append(f"provenance.intoto.json: unexpected subject: {name}")
    return records


def commit_from_sbom(sbom: dict[str, Any]) -> str | None:
    for package in sbom.get("packages", []):
        if isinstance(package, dict) and package.get("name") == "Arc":
            version = package.get("versionInfo")
            return version if isinstance(version, str) and version else None
    return None


def provenance_dependency_commit(provenance: dict[str, Any]) -> str | None:
    dependencies = provenance.get("predicate", {}).get("buildDefinition", {}).get("resolvedDependencies")
    if not isinstance(dependencies, list):
        return None
    for item in dependencies:
        if isinstance(item, dict) and item.get("name") == "arc-source":
            digest = item.get("digest")
            if isinstance(digest, dict):
                value = digest.get("gitCommit")
                return value if isinstance(value, str) else None
    return None


def check_commit_coherence(
    index: dict[str, Any],
    provenance: dict[str, Any],
    source: dict[str, Any],
    release: dict[str, Any],
    sbom: dict[str, Any],
    problems: list[str],
) -> str | None:
    commit = index.get("commit")
    values = {
        "evidence-index.json": commit,
        "source-manifest.json": source.get("commit"),
        "release-evidence.json": release.get("commit"),
        "sbom.spdx.json": commit_from_sbom(sbom),
        "provenance.intoto.json": provenance.get("predicate", {})
        .get("buildDefinition", {})
        .get("externalParameters", {})
        .get("commit"),
        "provenance.intoto.json resolved dependency": provenance_dependency_commit(provenance),
    }
    if not isinstance(commit, str) or not commit:
        problems.append("evidence-index.json: commit must be present")
        return None
    for name, value in values.items():
        if value != commit:
            problems.append(f"{name}: commit must match evidence-index commit")
    return commit


def check_release_commands(release: dict[str, Any], problems: list[str]) -> int:
    if release.get("ok") is not True:
        problems.append("release-evidence.json: ok must be true")
    if release.get("problems") not in ([], None):
        problems.append("release-evidence.json: problems must be empty")

    commands = release.get("required_commands")
    records = release.get("required_command_records")
    if not isinstance(commands, list) or not all(isinstance(command, str) for command in commands):
        problems.append("release-evidence.json: required_commands must be a string list")
        commands = []
    if not isinstance(records, list):
        problems.append("release-evidence.json: required_command_records must be a list")
        return 0

    record_commands: list[str] = []
    for item in records:
        if not isinstance(item, dict):
            problems.append("release-evidence.json: command record must be an object")
            continue
        command = item.get("command")
        kind = item.get("kind")
        if not isinstance(command, str) or not command:
            problems.append("release-evidence.json: command record must include command")
            continue
        if not isinstance(kind, str):
            problems.append(f"release-evidence.json: {command} command record must include kind")
            continue
        record_commands.append(command)
        if kind == "repo_tool":
            if item.get("exists") is not True:
                problems.append(f"release-evidence.json: {command} repo tool must exist")
            if item.get("executable") is not True:
                problems.append(f"release-evidence.json: {command} repo tool must be executable")
            if not isinstance(item.get("path"), str) or not item["path"]:
                problems.append(f"release-evidence.json: {command} repo tool must include path")
        elif kind == "npm_script":
            if item.get("exists") is not True:
                problems.append(f"release-evidence.json: {command} npm script must exist")
            if item.get("path") != "package.json":
                problems.append(f"release-evidence.json: {command} npm script path must be package.json")
            if not isinstance(item.get("script"), str) or not item["script"]:
                problems.append(f"release-evidence.json: {command} npm script must include script")
        elif kind == "external":
            if not isinstance(item.get("tool"), str) or not item["tool"]:
                problems.append(f"release-evidence.json: {command} external command must include tool")
        else:
            problems.append(f"release-evidence.json: {command} command kind is not recognized: {kind}")

    if record_commands != commands:
        problems.append("release-evidence.json: command records must match required_commands order")
    return len(record_commands)


def collect(artifact_dir: Path = DEFAULT_ARTIFACT_DIR) -> dict[str, Any]:
    artifact_dir = artifact_dir.resolve()
    problems: list[str] = []
    required = (INDEX_NAME, *EXPECTED_INDEXED)
    for name in required:
        if not (artifact_dir / name).is_file():
            problems.append(f"missing evidence file: {name}")

    index = expect_object(load_json(artifact_dir / INDEX_NAME, problems), INDEX_NAME, problems)
    provenance = expect_object(load_json(artifact_dir / PROVENANCE_NAME, problems), PROVENANCE_NAME, problems)
    source = expect_object(load_json(artifact_dir / "source-manifest.json", problems), "source-manifest.json", problems)
    release = expect_object(
        load_json(artifact_dir / "release-evidence.json", problems), "release-evidence.json", problems
    )
    sbom = expect_object(load_json(artifact_dir / SBOM_NAME, problems), SBOM_NAME, problems)

    indexed = indexed_records(index, artifact_dir, problems) if index else {}
    subjects = subject_records(provenance, artifact_dir, problems) if provenance else {}
    commit = check_commit_coherence(index, provenance, source, release, sbom, problems) if index else None
    release_command_count = check_release_commands(release, problems) if release else 0

    if source.get("dirty") is not False:
        problems.append("source-manifest.json: dirty must be false")
    if release.get("dirty") is not False:
        problems.append("release-evidence.json: dirty must be false")
    if sbom.get("spdxVersion") != "SPDX-2.3":
        problems.append("sbom.spdx.json: spdxVersion must be SPDX-2.3")

    return {
        "artifact_dir": str(artifact_dir),
        "commit": commit,
        "indexed_count": len(indexed),
        "provenance_subject_count": len(subjects),
        "release_command_count": release_command_count,
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc evidence bundle check",
        f"- artifact dir: {evidence['artifact_dir']}",
        f"- commit: {evidence['commit']}",
        f"- indexed files: {evidence['indexed_count']}",
        f"- provenance subjects: {evidence['provenance_subject_count']}",
        f"- release commands: {evidence['release_command_count']}",
        f"- problems: {len(evidence['problems'])}",
    ]
    if evidence["problems"]:
        lines.append("- problem details:")
        lines.extend(f"  - {problem}" for problem in evidence["problems"])
    return "\n".join(lines)


def payload_text(evidence: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(evidence, indent=2, sort_keys=True)
    return report_text(evidence)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate Arc evidence index, provenance, and artifact hashes.")
    parser.add_argument(
        "artifact_dir",
        nargs="?",
        type=Path,
        default=DEFAULT_ARTIFACT_DIR,
        help="directory containing arc evidence JSON files",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON evidence",
    )
    args = parser.parse_args(argv)

    evidence = collect(args.artifact_dir)
    print(payload_text(evidence, args.format))
    if evidence["problems"]:
        print("arc evidence bundle check failed: evidence bundle is inconsistent", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
