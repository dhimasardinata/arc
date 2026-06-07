#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
REPOSITORY_URL = "https://github.com/dhimasardinata/arc"
STATEMENT_TYPE = "https://in-toto.io/Statement/v1"
PREDICATE_TYPE = "https://slsa.dev/provenance/v1"
EVIDENCE_TOOLCHAIN = (
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


def git(args: list[str], root: Path) -> str | None:
    try:
        return subprocess.check_output(["git", *args], cwd=root, text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def relpath(path: Path, root: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(root.resolve()).as_posix()
    except ValueError:
        return resolved.as_posix()


def subject(path: Path, root: Path) -> tuple[dict[str, Any] | None, str | None]:
    full = path if path.is_absolute() else root / path
    resolved = full.resolve()
    try:
        name = resolved.relative_to(root.resolve()).as_posix()
    except ValueError:
        return None, f"subject path must stay inside repository: {path}"
    if not resolved.is_file():
        return None, f"missing subject file: {name}"
    return {
        "name": name,
        "digest": {
            "sha256": sha256(resolved),
        },
    }, None


def byproduct(path_text: str, root: Path) -> tuple[dict[str, Any] | None, str | None]:
    path = root / path_text
    if not path.is_file():
        return None, f"missing byproduct file: {path_text}"
    return {
        "name": path_text,
        "digest": {
            "sha256": sha256(path),
        },
    }, None


def env_map(keys: list[str]) -> dict[str, str]:
    return {key: value for key in keys if (value := os.environ.get(key))}


def invocation_id(root: Path, commit: str | None) -> str:
    server = os.environ.get("GITHUB_SERVER_URL")
    repo = os.environ.get("GITHUB_REPOSITORY")
    run_id = os.environ.get("GITHUB_RUN_ID")
    attempt = os.environ.get("GITHUB_RUN_ATTEMPT")
    if server and repo and run_id:
        suffix = f"/attempts/{attempt}" if attempt else ""
        return f"{server}/{repo}/actions/runs/{run_id}{suffix}"
    return f"local:{root.resolve()}:{commit or 'unknown'}"


def builder_id() -> str:
    if os.environ.get("GITHUB_ACTIONS") == "true":
        server = os.environ.get("GITHUB_SERVER_URL", "https://github.com")
        repo = os.environ.get("GITHUB_REPOSITORY", "dhimasardinata/arc")
        workflow = os.environ.get("GITHUB_WORKFLOW", "build")
        return f"{server}/{repo}/actions/workflows/{workflow}"
    return "local://arc/tools/provenance.py"


def build_type() -> str:
    workflow_ref = os.environ.get("GITHUB_WORKFLOW_REF")
    if workflow_ref:
        return f"https://github.com/{workflow_ref}"
    return f"{REPOSITORY_URL}/tools/provenance.py"


def collect(root: Path, subjects: list[Path]) -> tuple[dict[str, Any], list[str], dict[str, Any]]:
    root = root.resolve()
    commit = git(["rev-parse", "HEAD"], root)
    commit_time = git(["show", "-s", "--format=%cI", "HEAD"], root) or "1970-01-01T00:00:00Z"
    branch = git(["rev-parse", "--abbrev-ref", "HEAD"], root)
    statement_subjects: list[dict[str, Any]] = []
    tool_byproducts: list[dict[str, Any]] = []
    problems: list[str] = []

    for path in subjects:
        record, problem = subject(path, root)
        if problem is not None:
            problems.append(problem)
            continue
        assert record is not None
        statement_subjects.append(record)

    for path_text in EVIDENCE_TOOLCHAIN:
        record, problem = byproduct(path_text, root)
        if problem is not None:
            problems.append(problem)
            continue
        assert record is not None
        tool_byproducts.append(record)

    if not statement_subjects:
        problems.append("at least one provenance subject file is required")

    workflow_env = env_map(
        [
            "GITHUB_WORKFLOW",
            "GITHUB_WORKFLOW_REF",
            "GITHUB_JOB",
            "GITHUB_EVENT_NAME",
            "GITHUB_REF",
            "GITHUB_SHA",
            "GITHUB_REPOSITORY",
            "GITHUB_RUN_ID",
            "GITHUB_RUN_ATTEMPT",
            "RUNNER_OS",
            "RUNNER_ARCH",
        ]
    )
    predicate = {
        "buildDefinition": {
            "buildType": build_type(),
            "externalParameters": {
                "branch": branch,
                "commit": commit,
                "workflow": workflow_env,
                "subjects": [item["name"] for item in statement_subjects],
            },
            "internalParameters": {
                "tool": "tools/provenance.py",
                "predicateType": PREDICATE_TYPE,
            },
            "resolvedDependencies": [
                {
                    "uri": f"git+{REPOSITORY_URL}.git",
                    "digest": {
                        "gitCommit": commit or "unknown",
                    },
                    "name": "arc-source",
                }
            ],
        },
        "runDetails": {
            "builder": {
                "id": builder_id(),
            },
            "metadata": {
                "invocationId": invocation_id(root, commit),
                "startedOn": commit_time,
                "finishedOn": commit_time,
            },
            "byproducts": tool_byproducts,
        },
    }
    statement = {
        "_type": STATEMENT_TYPE,
        "subject": statement_subjects,
        "predicateType": PREDICATE_TYPE,
        "predicate": predicate,
    }
    stats = {
        "subject_count": len(statement_subjects),
        "byproduct_count": len(tool_byproducts),
        "problem_count": len(problems),
        "commit": commit,
        "builder": predicate["runDetails"]["builder"]["id"],
    }
    return statement, problems, stats


def report_text(stats: dict[str, Any], problems: list[str]) -> str:
    lines = [
        "arc provenance",
        f"- commit: {stats['commit']}",
        f"- builder: {stats['builder']}",
        f"- subjects: {stats['subject_count']}",
        f"- byproducts: {stats['byproduct_count']}",
        f"- problems: {stats['problem_count']}",
    ]
    if problems:
        lines.append("- problem details:")
        lines.extend(f"  - {problem}" for problem in problems)
    return "\n".join(lines)


def write_output(path: Path, payload: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(payload + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Generate in-toto SLSA provenance for Arc artifact subjects.")
    parser.add_argument(
        "subjects",
        nargs="+",
        type=Path,
        help="artifact files to include as statement subjects",
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="repository root for relative subject paths",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or in-toto JSON statement",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="write output to this file instead of stdout",
    )
    args = parser.parse_args(argv)

    statement, problems, stats = collect(args.root, args.subjects)
    payload = json.dumps(statement, indent=2, sort_keys=True) if args.format == "json" else report_text(stats, problems)
    if args.output:
        write_output(args.output, payload)
    else:
        print(payload)

    if problems:
        print("arc provenance failed: provenance subjects are incomplete", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
