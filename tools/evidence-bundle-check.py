#!/usr/bin/env python3
from __future__ import annotations

import argparse
from datetime import datetime
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_ARTIFACT_DIR = ROOT / ".arc-artifacts"
INDEX_NAME = "evidence-index.json"
PROVENANCE_NAME = "provenance.intoto.json"
SBOM_NAME = "sbom.spdx.json"
EXPECTED_SOURCE_DEPENDENCY = "arc-source"
EXPECTED_SOURCE_URI = "git+https://github.com/dhimasardinata/arc.git"
REGULAR_FILE_MODES = {"100644", "100755"}
EXPECTED_INDEXED = (
    "source-manifest.json",
    "third-party-manifest.json",
    "safety-case.json",
    "release-evidence.json",
    "workflow-pins.json",
    "workflow-policy.json",
    "evidence-workflow.json",
    "dependabot-policy.json",
    "npm-lock.json",
    "license-policy.json",
    "secret-scan.json",
    SBOM_NAME,
    PROVENANCE_NAME,
)
EXPECTED_PROVENANCE_SUBJECTS = tuple(name for name in EXPECTED_INDEXED if name != PROVENANCE_NAME)
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


def source_tree_sha256(records: list[dict[str, Any]]) -> str:
    digest = hashlib.sha256()
    for record in records:
        digest.update(str(record.get("mode")).encode())
        digest.update(b"\0")
        digest.update(str(record.get("path")).encode())
        digest.update(b"\0")
        digest.update(str(record.get("sha256")).encode())
        digest.update(b"\0")
        digest.update(str(record.get("size")).encode())
        digest.update(b"\0")
    return digest.hexdigest()


def git_tracked_regular_files(problems: list[str]) -> dict[str, dict[str, str]]:
    try:
        output = subprocess.check_output(["git", "ls-files", "--stage", "-z"], cwd=ROOT, stderr=subprocess.DEVNULL)
    except (OSError, subprocess.CalledProcessError) as exc:
        problems.append(f"source-manifest.json: cannot list tracked files: {exc}")
        return {}

    records: dict[str, dict[str, str]] = {}
    for raw in output.split(b"\0"):
        if not raw:
            continue
        meta, _, path = raw.partition(b"\t")
        parts = meta.decode().split()
        if len(parts) < 3:
            problems.append(f"source-manifest.json: cannot parse tracked file entry: {raw!r}")
            continue
        path_text = path.decode()
        mode = parts[0]
        if mode not in REGULAR_FILE_MODES:
            problems.append(f"source-manifest.json: unsupported tracked file mode: {mode} {path_text}")
            continue
        records[path_text] = {
            "mode": mode,
            "git_object": parts[1],
        }
    return records


def load_json(path: Path, problems: list[str]) -> Any:
    try:
        with path.open(encoding="utf-8") as handle:
            return json.load(handle)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        problems.append(f"{path.name}: cannot load JSON: {exc}")
        return None


def artifact_path(artifact_dir: Path, name: str) -> str:
    return (artifact_dir / name).relative_to(ROOT).as_posix() if (artifact_dir / name).is_relative_to(ROOT) else name


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
    if index.get("file_count") != len(files):
        problems.append("evidence-index.json: file_count must match files length")
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
        if item.get("size") != full.stat().st_size:
            problems.append(f"evidence-index.json: {name} size mismatch")
        expected_sha = item.get("sha256")
        actual_sha = sha256(full)
        if expected_sha != actual_sha:
            problems.append(f"evidence-index.json: {name} sha256 mismatch")
        state = item.get("json")
        if not isinstance(state, dict):
            problems.append(f"evidence-index.json: {name} JSON state must be an object")
        else:
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


def source_record_path(path_text: str, problems: list[str]) -> Path | None:
    if Path(path_text).is_absolute():
        problems.append(f"source-manifest.json: source path must be repository-relative: {path_text}")
        return None
    resolved = (ROOT / path_text).resolve()
    try:
        resolved.relative_to(ROOT)
    except ValueError:
        problems.append(f"source-manifest.json: source path must stay inside repository: {path_text}")
        return None
    return resolved


def current_file_mode(path: Path) -> str:
    return "100755" if path.stat().st_mode & 0o111 else "100644"


def check_source_manifest(source: dict[str, Any], problems: list[str]) -> int:
    if source.get("ok") is not True:
        problems.append("source-manifest.json: ok must be true")
    if source.get("problems") not in ([], None):
        problems.append("source-manifest.json: problems must be empty")
    if source.get("status") not in ([], None):
        problems.append("source-manifest.json: status must be empty")

    files = source.get("files")
    if not isinstance(files, list):
        problems.append("source-manifest.json: files must be a list")
        return 0
    if source.get("file_count") != len(files):
        problems.append("source-manifest.json: file_count must match files length")
    if not files:
        problems.append("source-manifest.json: files must not be empty")

    tracked = git_tracked_regular_files(problems)
    seen: set[str] = set()
    ordered_paths: list[str] = []
    valid_records: list[dict[str, Any]] = []
    for item in files:
        if not isinstance(item, dict):
            problems.append("source-manifest.json: file record must be an object")
            continue
        path_text = item.get("path")
        if not isinstance(path_text, str) or not path_text:
            problems.append("source-manifest.json: file record path must be a string")
            continue
        if path_text in seen:
            problems.append(f"source-manifest.json: duplicate source file: {path_text}")
            continue
        seen.add(path_text)
        ordered_paths.append(path_text)
        valid_records.append(item)

        path = source_record_path(path_text, problems)
        tracked_record = tracked.get(path_text)
        if tracked and tracked_record is None:
            problems.append(f"source-manifest.json: unexpected source file record: {path_text}")
        mode = item.get("mode")
        if mode not in REGULAR_FILE_MODES:
            problems.append(f"source-manifest.json: {path_text} mode must be a regular file mode")
        elif tracked_record is not None and mode != tracked_record["mode"]:
            problems.append(f"source-manifest.json: {path_text} mode mismatch")

        git_object = item.get("git_object")
        if not isinstance(git_object, str) or not git_object:
            problems.append(f"source-manifest.json: {path_text} git_object must be present")
        elif tracked_record is not None and git_object != tracked_record["git_object"]:
            problems.append(f"source-manifest.json: {path_text} git_object mismatch")

        if path is None:
            continue
        if not path.is_file():
            problems.append(f"source-manifest.json: source file missing on disk: {path_text}")
            continue
        if mode in REGULAR_FILE_MODES and current_file_mode(path) != mode:
            problems.append(f"source-manifest.json: {path_text} filesystem mode mismatch")
        if item.get("size") != path.stat().st_size:
            problems.append(f"source-manifest.json: {path_text} size mismatch")
        if item.get("sha256") != sha256(path):
            problems.append(f"source-manifest.json: {path_text} sha256 mismatch")

    if ordered_paths != sorted(ordered_paths):
        problems.append("source-manifest.json: files must be sorted by path")
    for path_text in sorted(set(tracked).difference(seen)):
        problems.append(f"source-manifest.json: missing tracked file record: {path_text}")

    tree_digest = source.get("tree_sha256")
    if not isinstance(tree_digest, str) or not tree_digest:
        problems.append("source-manifest.json: tree_sha256 must be present")
    elif source_tree_sha256(valid_records) != tree_digest:
        problems.append("source-manifest.json: tree_sha256 mismatch")
    return len(valid_records)


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


def provenance_source_dependency(provenance: dict[str, Any]) -> dict[str, Any] | None:
    dependencies = provenance.get("predicate", {}).get("buildDefinition", {}).get("resolvedDependencies")
    if not isinstance(dependencies, list):
        return None
    for item in dependencies:
        if isinstance(item, dict) and item.get("name") == EXPECTED_SOURCE_DEPENDENCY:
            return item
    return None


def provenance_dependency_commit(provenance: dict[str, Any]) -> str | None:
    dependency = provenance_source_dependency(provenance)
    if dependency is None:
        return None
    digest = dependency.get("digest")
    if isinstance(digest, dict):
        value = digest.get("gitCommit")
        return value if isinstance(value, str) else None
    return None


def provenance_dependency_uri(provenance: dict[str, Any]) -> str | None:
    dependency = provenance_source_dependency(provenance)
    if dependency is None:
        return None
    value = dependency.get("uri")
    if isinstance(value, str):
        return value
    return None


def byproduct_records(provenance: dict[str, Any], problems: list[str]) -> dict[str, dict[str, Any]]:
    byproducts = provenance.get("predicate", {}).get("runDetails", {}).get("byproducts")
    if not isinstance(byproducts, list):
        problems.append("provenance.intoto.json: byproducts must be a list")
        return {}
    records: dict[str, dict[str, Any]] = {}
    for item in byproducts:
        if not isinstance(item, dict):
            problems.append("provenance.intoto.json: byproduct record must be an object")
            continue
        name = item.get("name")
        digest = item.get("digest")
        if not isinstance(name, str) or not isinstance(digest, dict):
            problems.append("provenance.intoto.json: byproduct must include name and digest")
            continue
        if name in records:
            problems.append(f"provenance.intoto.json: duplicate byproduct: {name}")
            continue
        records[name] = item
        resolved = (ROOT / name).resolve()
        try:
            resolved.relative_to(ROOT)
        except ValueError:
            problems.append(f"provenance.intoto.json: {name} byproduct path must stay inside repository")
            continue
        if not resolved.is_file():
            problems.append(f"provenance.intoto.json: byproduct file missing on disk: {name}")
            continue
        if digest.get("sha256") != sha256(resolved):
            problems.append(f"provenance.intoto.json: {name} byproduct sha256 mismatch")

    missing = sorted(set(EXPECTED_PROVENANCE_BYPRODUCTS).difference(records))
    for name in missing:
        problems.append(f"provenance.intoto.json: missing required byproduct: {name}")
    extra = sorted(set(records).difference(EXPECTED_PROVENANCE_BYPRODUCTS))
    for name in extra:
        problems.append(f"provenance.intoto.json: unexpected byproduct: {name}")
    return records


def parse_metadata_time(value: Any, field: str, problems: list[str]) -> datetime | None:
    if not isinstance(value, str) or not value:
        problems.append(f"provenance.intoto.json: metadata {field} must be present")
        return None
    try:
        stamp = datetime.fromisoformat(value.replace("Z", "+00:00"))
    except ValueError:
        problems.append(f"provenance.intoto.json: metadata {field} must be ISO-8601")
        return None
    if stamp.tzinfo is None or stamp.utcoffset() is None:
        problems.append(f"provenance.intoto.json: metadata {field} must include timezone")
        return None
    return stamp


def check_provenance_predicate(provenance: dict[str, Any], problems: list[str]) -> None:
    predicate = provenance.get("predicate")
    if not isinstance(predicate, dict):
        problems.append("provenance.intoto.json: predicate must be an object")
        return

    build = predicate.get("buildDefinition")
    if not isinstance(build, dict):
        problems.append("provenance.intoto.json: buildDefinition must be an object")
        build = {}
    if not isinstance(build.get("buildType"), str) or not build["buildType"]:
        problems.append("provenance.intoto.json: buildType must be present")
    dependencies = build.get("resolvedDependencies")
    if not isinstance(dependencies, list):
        problems.append("provenance.intoto.json: resolvedDependencies must be a list")
    else:
        source_dependencies = [
            item for item in dependencies if isinstance(item, dict) and item.get("name") == EXPECTED_SOURCE_DEPENDENCY
        ]
        if len(dependencies) != 1 or len(source_dependencies) != 1:
            problems.append("provenance.intoto.json: resolvedDependencies must contain only arc-source")

    external = build.get("externalParameters")
    if not isinstance(external, dict):
        problems.append("provenance.intoto.json: externalParameters must be an object")
        external = {}
    subject_names = [
        item.get("name")
        for item in provenance.get("subject", [])
        if isinstance(item, dict) and isinstance(item.get("name"), str)
    ]
    if external.get("subjects") != subject_names:
        problems.append("provenance.intoto.json: externalParameters subjects must match provenance subjects")
    if provenance_dependency_uri(provenance) != EXPECTED_SOURCE_URI:
        problems.append(f"provenance.intoto.json: resolved dependency uri must be {EXPECTED_SOURCE_URI}")

    run = predicate.get("runDetails")
    if not isinstance(run, dict):
        problems.append("provenance.intoto.json: runDetails must be an object")
        run = {}
    builder = run.get("builder")
    if not isinstance(builder, dict) or not isinstance(builder.get("id"), str) or not builder["id"]:
        problems.append("provenance.intoto.json: builder id must be present")
    metadata = run.get("metadata")
    if not isinstance(metadata, dict):
        problems.append("provenance.intoto.json: metadata must be an object")
        metadata = {}
    if not isinstance(metadata.get("invocationId"), str) or not metadata["invocationId"]:
        problems.append("provenance.intoto.json: metadata invocationId must be present")
    started = parse_metadata_time(metadata.get("startedOn"), "startedOn", problems)
    finished = parse_metadata_time(metadata.get("finishedOn"), "finishedOn", problems)
    if started is not None and finished is not None and finished < started:
        problems.append("provenance.intoto.json: metadata finishedOn must not be before startedOn")


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


def check_branch_coherence(
    index: dict[str, Any],
    provenance: dict[str, Any],
    source: dict[str, Any],
    release: dict[str, Any],
    problems: list[str],
) -> str | None:
    branch = index.get("branch")
    values = {
        "evidence-index.json": branch,
        "source-manifest.json": source.get("branch"),
        "release-evidence.json": release.get("branch"),
        "provenance.intoto.json": provenance.get("predicate", {})
        .get("buildDefinition", {})
        .get("externalParameters", {})
        .get("branch"),
    }
    if not isinstance(branch, str) or not branch:
        problems.append("evidence-index.json: branch must be present")
        return None
    for name, value in values.items():
        if value != branch:
            problems.append(f"{name}: branch must match evidence-index branch")
    return branch


def command_record_path(item: dict[str, Any], command: str, label: str, problems: list[str]) -> Path | None:
    path_text = item.get("path")
    if not isinstance(path_text, str) or not path_text:
        problems.append(f"release-evidence.json: {command} {label} must include path")
        return None
    resolved = (ROOT / path_text).resolve()
    try:
        resolved.relative_to(ROOT)
    except ValueError:
        problems.append(f"release-evidence.json: {command} {label} path must stay inside repository")
        return None
    return resolved


def check_command_sha(item: dict[str, Any], command: str, label: str, path: Path, problems: list[str]) -> None:
    expected = item.get("sha256")
    if not isinstance(expected, str) or not expected:
        problems.append(f"release-evidence.json: {command} {label} must include sha256")
        return
    if not path.is_file():
        return
    if expected != sha256(path):
        problems.append(f"release-evidence.json: {command} {label} sha256 mismatch")


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
            path = command_record_path(item, command, "repo tool", problems)
            if path is not None:
                check_command_sha(item, command, "repo tool", path, problems)
        elif kind == "repo_source":
            if item.get("exists") is not True:
                problems.append(f"release-evidence.json: {command} repo source must exist")
            path = command_record_path(item, command, "repo source", problems)
            if path is not None:
                check_command_sha(item, command, "repo source", path, problems)
        elif kind == "npm_script":
            if item.get("exists") is not True:
                problems.append(f"release-evidence.json: {command} npm script must exist")
            if item.get("path") != "package.json":
                problems.append(f"release-evidence.json: {command} npm script path must be package.json")
            if not isinstance(item.get("script"), str) or not item["script"]:
                problems.append(f"release-evidence.json: {command} npm script must include script")
            path = command_record_path(item, command, "npm script", problems)
            if path is not None:
                check_command_sha(item, command, "npm script", path, problems)
        elif kind == "external":
            if not isinstance(item.get("tool"), str) or not item["tool"]:
                problems.append(f"release-evidence.json: {command} external command must include tool")
        else:
            problems.append(f"release-evidence.json: {command} command kind is not recognized: {kind}")

    if record_commands != commands:
        problems.append("release-evidence.json: command records must match required_commands order")
    return len(record_commands)


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
            "source_file_count": 0,
            "provenance_subject_count": 0,
            "provenance_byproduct_count": 0,
            "release_command_count": 0,
            "ok": False,
            "problems": problems,
        }

    artifact_dir = checked_dir
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
    source_file_count = check_source_manifest(source, problems) if source else 0
    subjects = subject_records(provenance, artifact_dir, problems) if provenance else {}
    byproducts = byproduct_records(provenance, problems) if provenance else {}
    if provenance:
        check_provenance_predicate(provenance, problems)
    commit = check_commit_coherence(index, provenance, source, release, sbom, problems) if index else None
    branch = check_branch_coherence(index, provenance, source, release, problems) if index else None
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
        "branch": branch,
        "indexed_count": len(indexed),
        "source_file_count": source_file_count,
        "provenance_subject_count": len(subjects),
        "provenance_byproduct_count": len(byproducts),
        "release_command_count": release_command_count,
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc evidence bundle check",
        f"- artifact dir: {evidence['artifact_dir']}",
        f"- commit: {evidence['commit']}",
        f"- branch: {evidence['branch']}",
        f"- indexed files: {evidence['indexed_count']}",
        f"- source files: {evidence['source_file_count']}",
        f"- provenance subjects: {evidence['provenance_subject_count']}",
        f"- provenance byproducts: {evidence['provenance_byproduct_count']}",
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
