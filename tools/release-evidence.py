#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]

POLICY_FILES = [
    "RELEASE.md",
    "SECURITY.md",
    "CONTRIBUTING.md",
    "THIRD_PARTY_NOTICES.md",
    "THIRD_PARTY_MANIFEST.json",
    ".github/CODEOWNERS",
    ".github/pull_request_template.md",
    ".github/ISSUE_TEMPLATE/bug_report.yml",
    ".github/ISSUE_TEMPLATE/firmware_validation.yml",
    ".github/workflows/build.yml",
    ".github/workflows/codeql.yml",
    ".github/workflows/pages.yml",
]

REQUIRED_COMMANDS = [
    "./tools/check-repo.sh",
    "./tools/host-tests.sh",
    "tools/clangd-compile-commands.py --validate-arc-headers",
    "./tools/firmware-manifest.py --format json --require-artifacts",
    "./tools/source-manifest.py --format json --require-clean",
    "./tools/third-party-manifest-check.py --format json",
    "./tools/license-policy-check.py --format json",
    "./tools/evidence-index.py --format json",
    "./tools/evidence-bundle-check.py .arc-artifacts",
    "./tools/sbom.py --format json",
    "./tools/provenance.py --format json",
    "npm run docs:build",
    "idf.py build",
]


def git(args: list[str]) -> str:
    return subprocess.check_output(["git", *args], cwd=ROOT, text=True).strip()


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def file_record(name: str) -> dict[str, Any]:
    path = ROOT / name
    exists = path.is_file()
    return {
        "path": name,
        "exists": exists,
        "sha256": sha256(path) if exists else None,
    }


def collect() -> dict[str, Any]:
    status = git(["status", "--short", "--untracked-files=all"]).splitlines()
    return {
        "commit": git(["rev-parse", "HEAD"]),
        "branch": git(["rev-parse", "--abbrev-ref", "HEAD"]),
        "dirty": bool(status),
        "status": status,
        "policy_files": [file_record(name) for name in POLICY_FILES],
        "required_commands": REQUIRED_COMMANDS,
    }


def print_report(evidence: dict[str, Any]) -> None:
    print("arc release evidence")
    print(f"- commit: {evidence['commit']}")
    print(f"- branch: {evidence['branch']}")
    print(f"- dirty: {'yes' if evidence['dirty'] else 'no'}")
    if evidence["status"]:
        print("- status:")
        for line in evidence["status"]:
            print(f"  - {line}")
    print("- policy files:")
    for record in evidence["policy_files"]:
        state = "ok" if record["exists"] else "missing"
        suffix = f" sha256={record['sha256']}" if record["sha256"] else ""
        print(f"  - {record['path']}: {state}{suffix}")
    print("- required commands:")
    for command in evidence["required_commands"]:
        print(f"  - {command}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Emit Arc release evidence metadata.")
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON manifest",
    )
    parser.add_argument(
        "--require-clean",
        action="store_true",
        help="fail when the git worktree has tracked or untracked changes",
    )
    args = parser.parse_args(argv)

    evidence = collect()
    missing = [record["path"] for record in evidence["policy_files"] if not record["exists"]]

    if args.format == "json":
        print(json.dumps(evidence, indent=2, sort_keys=True))
    else:
        print_report(evidence)

    if missing:
        print(
            "arc release evidence failed: missing policy files: " + ", ".join(missing),
            file=sys.stderr,
        )
        return 1
    if args.require_clean and evidence["dirty"]:
        print("arc release evidence failed: worktree is dirty", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
