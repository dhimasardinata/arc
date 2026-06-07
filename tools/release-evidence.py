#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import os
import shlex
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]

POLICY_FILES = [
    "README.md",
    "RELEASE.md",
    "SECURITY.md",
    "CONTRIBUTING.md",
    "docs/governance.md",
    "docs/security.md",
    "docs/safety-case.md",
    "docs/licensing.md",
    "docs/hil-test-jig.md",
    "docs/benchmarking.md",
    "THIRD_PARTY_NOTICES.md",
    "THIRD_PARTY_MANIFEST.json",
    ".github/CODEOWNERS",
    ".github/dependabot.yml",
    ".github/pull_request_template.md",
    ".github/ISSUE_TEMPLATE/bug_report.yml",
    ".github/ISSUE_TEMPLATE/firmware_validation.yml",
    ".github/workflows/build.yml",
    ".github/workflows/codeql.yml",
    ".github/workflows/pages.yml",
]

REQUIRED_COMMANDS = [
    "./tools/check-repo.sh",
    "./tools/format.sh --check",
    "./tools/tool-tests.sh",
    "./tools/profile-manifest-check.py",
    "./tools/topology-check.py --quiet",
    "python3 tools/compile-fail-check.py",
    "./tools/arc-prove.sh",
    "./tools/use-after-move-check.sh",
    "go run tools/arc-audit.go -root . -all",
    "./tools/host-tests.sh",
    "tools/clangd-compile-commands.py --validate-arc-headers",
    "./tools/source-manifest.py --format json --require-clean",
    "./tools/firmware-manifest.py --format json --require-artifacts",
    "./tools/safety-case-check.py --format json",
    "./tools/third-party-manifest-check.py --format json",
    "./tools/workflow-pins-check.py --format json",
    "./tools/workflow-policy-check.py --format json",
    "./tools/dependabot-policy-check.py --format json",
    "./tools/evidence-workflow-check.py --format json",
    "./tools/npm-lock-check.py --format json",
    "./tools/license-policy-check.py --format json",
    "./tools/secret-scan-check.py --format json",
    "./tools/evidence-index.py --format json",
    "./tools/evidence-bundle-check.py .arc-artifacts",
    "./tools/sbom.py --format json",
    "./tools/provenance.py --format json",
    "./tools/release-evidence.py --format json --require-clean",
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


def package_scripts() -> dict[str, str]:
    package_json = ROOT / "package.json"
    try:
        payload = json.loads(package_json.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError):
        return {}
    scripts = payload.get("scripts")
    return scripts if isinstance(scripts, dict) else {}


def repo_tool_record(command: str, path_text: str) -> dict[str, Any]:
    path_text = path_text.removeprefix("./")
    path = ROOT / path_text
    exists = path.is_file()
    executable = exists and os.access(path, os.X_OK)
    return {
        "command": command,
        "kind": "repo_tool",
        "path": path_text,
        "exists": exists,
        "executable": executable,
        "sha256": sha256(path) if exists else None,
    }


def repo_source_record(command: str, path_text: str) -> dict[str, Any]:
    path_text = path_text.removeprefix("./")
    path = ROOT / path_text
    return {
        "command": command,
        "kind": "repo_source",
        "path": path_text,
        "exists": path.is_file(),
        "sha256": sha256(path) if path.is_file() else None,
    }


def command_record(command: str, scripts: dict[str, str] | None = None) -> dict[str, Any]:
    scripts = package_scripts() if scripts is None else scripts
    try:
        parts = shlex.split(command)
    except ValueError as exc:
        return {
            "command": command,
            "kind": "invalid",
            "problem": f"cannot parse command: {exc}",
        }
    if not parts:
        return {
            "command": command,
            "kind": "invalid",
            "problem": "command is empty",
        }

    if parts[0] in {"python", "python3"} and len(parts) > 1 and parts[1].removeprefix("./").startswith("tools/"):
        return repo_tool_record(command, parts[1])
    if len(parts) > 2 and parts[0] == "go" and parts[1] == "run" and parts[2].removeprefix("./").startswith("tools/"):
        return repo_source_record(command, parts[2])
    if parts[0].removeprefix("./").startswith("tools/"):
        return repo_tool_record(command, parts[0])
    if len(parts) >= 3 and parts[0] == "npm" and parts[1] == "run":
        script = parts[2]
        package_json = ROOT / "package.json"
        package_exists = package_json.is_file()
        return {
            "command": command,
            "kind": "npm_script",
            "path": "package.json",
            "script": script,
            "exists": script in scripts,
            "sha256": sha256(package_json) if package_exists else None,
        }
    return {
        "command": command,
        "kind": "external",
        "tool": parts[0],
    }


def command_problem(record: dict[str, Any]) -> str | None:
    command = record["command"]
    kind = record["kind"]
    if kind == "invalid":
        return f"{command}: {record['problem']}"
    if kind == "repo_tool":
        if not record["exists"]:
            return f"{command}: repo tool path missing: {record['path']}"
        if not record["executable"]:
            return f"{command}: repo tool must be executable: {record['path']}"
    if kind == "repo_source" and not record["exists"]:
        return f"{command}: repo source path missing: {record['path']}"
    if kind == "npm_script" and not record["exists"]:
        return f"{command}: npm script missing from package.json: {record['script']}"
    return None


def collect() -> dict[str, Any]:
    status = git(["status", "--short", "--untracked-files=all"]).splitlines()
    policy_files = [file_record(name) for name in POLICY_FILES]
    command_records = [command_record(command) for command in REQUIRED_COMMANDS]
    problems = [f"missing policy file: {record['path']}" for record in policy_files if not record["exists"]]
    problems.extend(problem for record in command_records if (problem := command_problem(record)))
    return {
        "commit": git(["rev-parse", "HEAD"]),
        "branch": git(["rev-parse", "--abbrev-ref", "HEAD"]),
        "dirty": bool(status),
        "status": status,
        "ok": not problems,
        "policy_files": policy_files,
        "required_commands": REQUIRED_COMMANDS,
        "required_command_records": command_records,
        "problems": problems,
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
    print("- command checks:")
    for record in evidence["required_command_records"]:
        if record["kind"] == "repo_tool":
            state = "ok" if record["exists"] and record["executable"] else "problem"
            suffix = f" sha256={record['sha256']}" if record["sha256"] else ""
            print(f"  - {record['command']}: {state} path={record['path']}{suffix}")
        elif record["kind"] == "repo_source":
            state = "ok" if record["exists"] else "problem"
            suffix = f" sha256={record['sha256']}" if record["sha256"] else ""
            print(f"  - {record['command']}: {state} path={record['path']}{suffix}")
        elif record["kind"] == "npm_script":
            state = "ok" if record["exists"] else "problem"
            suffix = f" sha256={record['sha256']}" if record["sha256"] else ""
            print(f"  - {record['command']}: {state} script={record['script']}{suffix}")
        else:
            print(f"  - {record['command']}: external tool={record.get('tool', record['kind'])}")
    if evidence["problems"]:
        print("- problems:")
        for problem in evidence["problems"]:
            print(f"  - {problem}")


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
    if args.format == "json":
        print(json.dumps(evidence, indent=2, sort_keys=True))
    else:
        print_report(evidence)

    if evidence["problems"]:
        print(
            "arc release evidence failed: " + "; ".join(evidence["problems"]),
            file=sys.stderr,
        )
        return 1
    if args.require_clean and evidence["dirty"]:
        print("arc release evidence failed: worktree is dirty", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
