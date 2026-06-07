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
INDEX_NAME = "firmware-evidence-index.json"
MANIFEST_NAME = "firmware-manifest.json"
PROVENANCE_NAME = "firmware-provenance.intoto.json"
EXPECTED_INDEXED = (MANIFEST_NAME, PROVENANCE_NAME)
EXPECTED_PROVENANCE_SUBJECTS = (MANIFEST_NAME,)
EXPECTED_PROVENANCE_BYPRODUCTS = (
    "tools/source-manifest.py",
    "tools/third-party-manifest-check.py",
    "tools/safety-case-check.py",
    "tools/firmware-evidence-check.py",
    "tools/release-evidence.py",
    "tools/workflow-pins-check.py",
    "tools/workflow-policy-check.py",
    "tools/evidence-workflow-check.py",
    "tools/dependabot-policy-check.py",
    "tools/npm-lock-check.py",
    "tools/license-policy-check.py",
    "tools/secret-scan-check.py",
    "tools/sbom.py",
    "tools/provenance.py",
    "tools/evidence-index.py",
    "tools/evidence-bundle-check.py",
)


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
    full = artifact_dir / name
    return full.relative_to(ROOT).as_posix() if full.is_relative_to(ROOT) else name


def repo_artifact_dir(path: Path, problems: list[str]) -> Path | None:
    resolved = path.resolve()
    try:
        rel = resolved.relative_to(ROOT).as_posix()
    except ValueError:
        problems.append(f"artifact directory must stay inside repository: {path}")
        return None
    if not resolved.is_dir():
        problems.append(f"artifact directory is missing: {rel}")
        return None
    return resolved


def repo_file(path_text: str, label: str, problems: list[str]) -> Path | None:
    resolved = (ROOT / path_text).resolve()
    try:
        resolved.relative_to(ROOT)
    except ValueError:
        problems.append(f"{label} must stay inside repository: {path_text}")
        return None
    if not resolved.is_file():
        problems.append(f"{label} missing on disk: {path_text}")
        return None
    return resolved


def expect_object(payload: Any, name: str, problems: list[str]) -> dict[str, Any]:
    if isinstance(payload, dict):
        return payload
    problems.append(f"{name}: JSON root must be an object")
    return {}


def indexed_records(index: dict[str, Any], artifact_dir: Path, problems: list[str]) -> dict[str, dict[str, Any]]:
    if index.get("ok") is not True:
        problems.append(f"{INDEX_NAME}: ok must be true")
    if index.get("problems") not in ([], None):
        problems.append(f"{INDEX_NAME}: problems must be empty")

    files = index.get("files")
    records: dict[str, dict[str, Any]] = {}
    if not isinstance(files, list):
        problems.append(f"{INDEX_NAME}: files must be a list")
        return records
    if index.get("file_count") != len(files):
        problems.append(f"{INDEX_NAME}: file_count must match files length")

    for item in files:
        if not isinstance(item, dict):
            problems.append(f"{INDEX_NAME}: file record must be an object")
            continue
        path = item.get("path")
        if not isinstance(path, str):
            problems.append(f"{INDEX_NAME}: file record path must be a string")
            continue
        name = Path(path).name
        if name in records:
            problems.append(f"{INDEX_NAME}: duplicate indexed file: {name}")
            continue
        records[name] = item
        expected_path = artifact_path(artifact_dir, name)
        if path != expected_path:
            problems.append(f"{INDEX_NAME}: {name} path must be {expected_path}")
        full = artifact_dir / name
        if not full.is_file():
            problems.append(f"{INDEX_NAME}: indexed file missing on disk: {name}")
            continue
        if item.get("size") != full.stat().st_size:
            problems.append(f"{INDEX_NAME}: {name} size mismatch")
        if item.get("sha256") != sha256(full):
            problems.append(f"{INDEX_NAME}: {name} sha256 mismatch")
        state = item.get("json")
        if not isinstance(state, dict):
            problems.append(f"{INDEX_NAME}: {name} JSON state must be an object")
        else:
            if state.get("valid") is not True:
                problems.append(f"{INDEX_NAME}: {name} JSON validity must be true")
            if state.get("ok") is False:
                problems.append(f"{INDEX_NAME}: {name} indexed ok field is false")
            if state.get("problem_count") not in (None, 0):
                problems.append(f"{INDEX_NAME}: {name} indexed problem count must be zero")

    missing = sorted(set(EXPECTED_INDEXED).difference(records))
    for name in missing:
        problems.append(f"{INDEX_NAME}: missing required indexed evidence: {name}")
    extra = sorted(set(records).difference(EXPECTED_INDEXED))
    for name in extra:
        problems.append(f"{INDEX_NAME}: unexpected indexed evidence: {name}")
    return records


def subject_records(provenance: dict[str, Any], artifact_dir: Path, problems: list[str]) -> dict[str, dict[str, Any]]:
    if provenance.get("_type") != "https://in-toto.io/Statement/v1":
        problems.append(f"{PROVENANCE_NAME}: _type must be in-toto Statement v1")
    if provenance.get("predicateType") != "https://slsa.dev/provenance/v1":
        problems.append(f"{PROVENANCE_NAME}: predicateType must be SLSA provenance v1")

    subjects = provenance.get("subject")
    records: dict[str, dict[str, Any]] = {}
    if not isinstance(subjects, list):
        problems.append(f"{PROVENANCE_NAME}: subject must be a list")
        return records

    for item in subjects:
        if not isinstance(item, dict):
            problems.append(f"{PROVENANCE_NAME}: subject record must be an object")
            continue
        path = item.get("name")
        digest = item.get("digest")
        if not isinstance(path, str) or not isinstance(digest, dict):
            problems.append(f"{PROVENANCE_NAME}: subject must include name and digest")
            continue
        name = Path(path).name
        if name in records:
            problems.append(f"{PROVENANCE_NAME}: duplicate subject: {name}")
            continue
        records[name] = item
        expected_path = artifact_path(artifact_dir, name)
        if path != expected_path:
            problems.append(f"{PROVENANCE_NAME}: {name} subject path must be {expected_path}")
        full = artifact_dir / name
        if not full.is_file():
            problems.append(f"{PROVENANCE_NAME}: subject file missing on disk: {name}")
            continue
        if digest.get("sha256") != sha256(full):
            problems.append(f"{PROVENANCE_NAME}: {name} subject sha256 mismatch")

    missing = sorted(set(EXPECTED_PROVENANCE_SUBJECTS).difference(records))
    for name in missing:
        problems.append(f"{PROVENANCE_NAME}: missing required subject: {name}")
    extra = sorted(set(records).difference(EXPECTED_PROVENANCE_SUBJECTS))
    for name in extra:
        problems.append(f"{PROVENANCE_NAME}: unexpected subject: {name}")
    return records


def byproduct_records(provenance: dict[str, Any], problems: list[str]) -> dict[str, dict[str, Any]]:
    byproducts = provenance.get("predicate", {}).get("runDetails", {}).get("byproducts")
    records: dict[str, dict[str, Any]] = {}
    if not isinstance(byproducts, list):
        problems.append(f"{PROVENANCE_NAME}: byproducts must be a list")
        return records
    for item in byproducts:
        if not isinstance(item, dict):
            problems.append(f"{PROVENANCE_NAME}: byproduct record must be an object")
            continue
        name = item.get("name")
        digest = item.get("digest")
        if not isinstance(name, str) or not isinstance(digest, dict):
            problems.append(f"{PROVENANCE_NAME}: byproduct must include name and digest")
            continue
        if name in records:
            problems.append(f"{PROVENANCE_NAME}: duplicate byproduct: {name}")
            continue
        records[name] = item
        path = repo_file(name, f"{PROVENANCE_NAME}: byproduct", problems)
        if path is not None and digest.get("sha256") != sha256(path):
            problems.append(f"{PROVENANCE_NAME}: {name} byproduct sha256 mismatch")

    missing = sorted(set(EXPECTED_PROVENANCE_BYPRODUCTS).difference(records))
    for name in missing:
        problems.append(f"{PROVENANCE_NAME}: missing required byproduct: {name}")
    extra = sorted(set(records).difference(EXPECTED_PROVENANCE_BYPRODUCTS))
    for name in extra:
        problems.append(f"{PROVENANCE_NAME}: unexpected byproduct: {name}")
    return records


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


def check_provenance_predicate(provenance: dict[str, Any], problems: list[str]) -> None:
    predicate = provenance.get("predicate")
    if not isinstance(predicate, dict):
        problems.append(f"{PROVENANCE_NAME}: predicate must be an object")
        return

    build = predicate.get("buildDefinition")
    if not isinstance(build, dict):
        problems.append(f"{PROVENANCE_NAME}: buildDefinition must be an object")
        build = {}
    if not isinstance(build.get("buildType"), str) or not build["buildType"]:
        problems.append(f"{PROVENANCE_NAME}: buildType must be present")

    external = build.get("externalParameters")
    if not isinstance(external, dict):
        problems.append(f"{PROVENANCE_NAME}: externalParameters must be an object")
        external = {}
    subject_names = [
        item.get("name")
        for item in provenance.get("subject", [])
        if isinstance(item, dict) and isinstance(item.get("name"), str)
    ]
    if external.get("subjects") != subject_names:
        problems.append(f"{PROVENANCE_NAME}: externalParameters subjects must match provenance subjects")

    run = predicate.get("runDetails")
    if not isinstance(run, dict):
        problems.append(f"{PROVENANCE_NAME}: runDetails must be an object")
        run = {}
    builder = run.get("builder")
    if not isinstance(builder, dict) or not isinstance(builder.get("id"), str) or not builder["id"]:
        problems.append(f"{PROVENANCE_NAME}: builder id must be present")
    metadata = run.get("metadata")
    if not isinstance(metadata, dict):
        problems.append(f"{PROVENANCE_NAME}: metadata must be an object")
        metadata = {}
    for field in ("invocationId", "startedOn", "finishedOn"):
        if not isinstance(metadata.get(field), str) or not metadata[field]:
            problems.append(f"{PROVENANCE_NAME}: metadata {field} must be present")


def check_commit_coherence(
    index: dict[str, Any], manifest: dict[str, Any], provenance: dict[str, Any], problems: list[str]
) -> str | None:
    commit = index.get("commit")
    values = {
        INDEX_NAME: commit,
        MANIFEST_NAME: manifest.get("commit"),
        PROVENANCE_NAME: provenance.get("predicate", {})
        .get("buildDefinition", {})
        .get("externalParameters", {})
        .get("commit"),
        f"{PROVENANCE_NAME} resolved dependency": provenance_dependency_commit(provenance),
    }
    if not isinstance(commit, str) or not commit:
        problems.append(f"{INDEX_NAME}: commit must be present")
        return None
    for name, value in values.items():
        if value != commit:
            problems.append(f"{name}: commit must match {INDEX_NAME} commit")
    return commit


def check_branch_coherence(index: dict[str, Any], manifest: dict[str, Any], problems: list[str]) -> str | None:
    branch = index.get("branch")
    values = {
        INDEX_NAME: branch,
        MANIFEST_NAME: manifest.get("branch"),
    }
    if not isinstance(branch, str) or not branch:
        problems.append(f"{INDEX_NAME}: branch must be present")
        return None
    for name, value in values.items():
        if value != branch:
            problems.append(f"{name}: branch must match {INDEX_NAME} branch")
    return branch


def check_manifest_artifacts(manifest: dict[str, Any], problems: list[str]) -> int:
    if manifest.get("ok") is not True:
        problems.append(f"{MANIFEST_NAME}: ok must be true")
    if manifest.get("problems") not in ([], None):
        problems.append(f"{MANIFEST_NAME}: problems must be empty")
    if manifest.get("dirty") is not False:
        problems.append(f"{MANIFEST_NAME}: dirty must be false")

    artifacts = manifest.get("artifacts")
    if not isinstance(artifacts, list):
        problems.append(f"{MANIFEST_NAME}: artifacts must be a list")
        return 0
    if not artifacts:
        problems.append(f"{MANIFEST_NAME}: artifacts must not be empty")
        return 0
    if manifest.get("artifact_count") != len(artifacts):
        problems.append(f"{MANIFEST_NAME}: artifact_count must match artifacts length")

    count = 0
    seen: set[str] = set()
    for item in artifacts:
        if not isinstance(item, dict):
            problems.append(f"{MANIFEST_NAME}: artifact record must be an object")
            continue
        path_text = item.get("path")
        if not isinstance(path_text, str) or not path_text:
            problems.append(f"{MANIFEST_NAME}: artifact path must be a string")
            continue
        if path_text in seen:
            problems.append(f"{MANIFEST_NAME}: duplicate artifact: {path_text}")
            continue
        seen.add(path_text)
        path = repo_file(path_text, f"{MANIFEST_NAME}: firmware artifact", problems)
        if path is None:
            continue
        count += 1
        if item.get("size") != path.stat().st_size:
            problems.append(f"{MANIFEST_NAME}: {path_text} size mismatch")
        if item.get("sha256") != sha256(path):
            problems.append(f"{MANIFEST_NAME}: {path_text} sha256 mismatch")
        if not isinstance(item.get("kind"), str) or not item["kind"]:
            problems.append(f"{MANIFEST_NAME}: {path_text} kind must be present")
    return count


def collect(artifact_dir: Path = DEFAULT_ARTIFACT_DIR) -> dict[str, Any]:
    problems: list[str] = []
    requested_dir = artifact_dir.resolve()
    checked_dir = repo_artifact_dir(artifact_dir, problems)
    if checked_dir is None:
        return {
            "artifact_dir": str(requested_dir),
            "commit": None,
            "branch": None,
            "indexed_count": 0,
            "provenance_subject_count": 0,
            "provenance_byproduct_count": 0,
            "firmware_artifact_count": 0,
            "ok": False,
            "problems": problems,
        }

    artifact_dir = checked_dir
    for name in (INDEX_NAME, *EXPECTED_INDEXED):
        if not (artifact_dir / name).is_file():
            problems.append(f"missing firmware evidence file: {name}")

    index = expect_object(load_json(artifact_dir / INDEX_NAME, problems), INDEX_NAME, problems)
    manifest = expect_object(load_json(artifact_dir / MANIFEST_NAME, problems), MANIFEST_NAME, problems)
    provenance = expect_object(load_json(artifact_dir / PROVENANCE_NAME, problems), PROVENANCE_NAME, problems)

    indexed = indexed_records(index, artifact_dir, problems) if index else {}
    subjects = subject_records(provenance, artifact_dir, problems) if provenance else {}
    byproducts = byproduct_records(provenance, problems) if provenance else {}
    if provenance:
        check_provenance_predicate(provenance, problems)
    firmware_artifact_count = check_manifest_artifacts(manifest, problems) if manifest else 0
    commit = check_commit_coherence(index, manifest, provenance, problems) if index else None
    branch = check_branch_coherence(index, manifest, problems) if index else None

    return {
        "artifact_dir": str(artifact_dir),
        "commit": commit,
        "branch": branch,
        "indexed_count": len(indexed),
        "provenance_subject_count": len(subjects),
        "provenance_byproduct_count": len(byproducts),
        "firmware_artifact_count": firmware_artifact_count,
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc firmware evidence check",
        f"- artifact dir: {evidence['artifact_dir']}",
        f"- commit: {evidence['commit']}",
        f"- branch: {evidence['branch']}",
        f"- indexed files: {evidence['indexed_count']}",
        f"- provenance subjects: {evidence['provenance_subject_count']}",
        f"- provenance byproducts: {evidence['provenance_byproduct_count']}",
        f"- firmware artifacts: {evidence['firmware_artifact_count']}",
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
    parser = argparse.ArgumentParser(description="Validate Arc firmware evidence, provenance, and binary hashes.")
    parser.add_argument(
        "artifact_dir",
        nargs="?",
        type=Path,
        default=DEFAULT_ARTIFACT_DIR,
        help="directory containing firmware evidence JSON files",
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
        print("arc firmware evidence check failed: firmware evidence bundle is inconsistent", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
