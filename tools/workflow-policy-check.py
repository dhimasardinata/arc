#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, NamedTuple


ROOT = Path(__file__).resolve().parents[1]
TOP_LEVEL_PERMISSIONS = {
    ".github/workflows/build.yml": {"contents": "read"},
    ".github/workflows/codeql.yml": {
        "actions": "read",
        "contents": "read",
        "security-events": "write",
    },
    ".github/workflows/pages.yml": {"contents": "read"},
}
JOB_PERMISSIONS = {
    (".github/workflows/pages.yml", "deploy"): {
        "id-token": "write",
        "pages": "write",
    },
}


class Job(NamedTuple):
    job_id: str
    timeout_minutes: int | None
    permissions: dict[str, str]


def workflow_files(root: Path) -> list[Path]:
    workflow_dir = root / ".github" / "workflows"
    if not workflow_dir.is_dir():
        return []
    files = list(workflow_dir.glob("*.yml"))
    files.extend(workflow_dir.glob("*.yaml"))
    return sorted(files, key=lambda path: path.relative_to(root).as_posix())


def relpath(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def leading_spaces(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def top_section(lines: list[str], key: str) -> tuple[int | None, list[str], str | None]:
    pattern = re.compile(rf"^{re.escape(key)}:\s*(?P<value>.*)$")
    for index, line in enumerate(lines):
        if leading_spaces(line) != 0:
            continue
        match = pattern.match(line)
        if not match:
            continue
        scalar = match.group("value").strip() or None
        block: list[str] = []
        for child in lines[index + 1 :]:
            if child.strip() and leading_spaces(child) == 0:
                break
            block.append(child)
        return index + 1, block, scalar
    return None, [], None


def mapping_from_block(block: list[str], indent: int) -> dict[str, str]:
    mapping: dict[str, str] = {}
    pattern = re.compile(rf"^ {{{indent}}}([A-Za-z0-9_-]+):\s*(.+?)\s*$")
    for line in block:
        match = pattern.match(line)
        if match:
            mapping[match.group(1)] = match.group(2).strip("\"'")
    return mapping


def parse_top_permissions(lines: list[str]) -> tuple[dict[str, str], str | None]:
    _, block, scalar = top_section(lines, "permissions")
    if scalar:
        return {}, scalar
    return mapping_from_block(block, 2), None


def parse_concurrency(lines: list[str]) -> dict[str, str]:
    _, block, scalar = top_section(lines, "concurrency")
    if scalar:
        return {"value": scalar}
    return mapping_from_block(block, 2)


def parse_jobs(lines: list[str]) -> list[Job]:
    jobs_line, block, scalar = top_section(lines, "jobs")
    if jobs_line is None or scalar:
        return []

    starts: list[tuple[int, str]] = []
    pattern = re.compile(r"^  ([A-Za-z0-9_-]+):\s*$")
    for offset, line in enumerate(block):
        match = pattern.match(line)
        if match:
            starts.append((offset, match.group(1)))

    jobs: list[Job] = []
    for index, (start, job_id) in enumerate(starts):
        end = starts[index + 1][0] if index + 1 < len(starts) else len(block)
        job_block = block[start + 1 : end]
        timeout = None
        permissions: dict[str, str] = {}
        for line_index, line in enumerate(job_block):
            timeout_match = re.match(r"^    timeout-minutes:\s*([0-9]+)\s*$", line)
            if timeout_match:
                timeout = int(timeout_match.group(1))
            if re.match(r"^    permissions:\s*$", line):
                permission_block: list[str] = []
                for permission_line in job_block[line_index + 1 :]:
                    if permission_line.strip() and leading_spaces(permission_line) <= 4:
                        break
                    permission_block.append(permission_line)
                permissions = mapping_from_block(permission_block, 6)
        jobs.append(Job(job_id, timeout, permissions))
    return jobs


def workflow_record(path: Path, root: Path) -> dict[str, Any]:
    lines = path.read_text(encoding="utf-8").splitlines()
    permissions, permission_scalar = parse_top_permissions(lines)
    return {
        "path": relpath(path, root),
        "top_permissions": permissions,
        "top_permissions_scalar": permission_scalar,
        "concurrency": parse_concurrency(lines),
        "jobs": [
            {
                "id": job.job_id,
                "timeout_minutes": job.timeout_minutes,
                "permissions": job.permissions,
            }
            for job in parse_jobs(lines)
        ],
        "pull_request_target": any(re.match(r"^\s*pull_request_target\s*:", line) for line in lines),
    }


def validate_workflow(record: dict[str, Any]) -> list[str]:
    problems: list[str] = []
    path = str(record["path"])

    if record["pull_request_target"]:
        problems.append(f"{path}: pull_request_target is not allowed")

    if record["top_permissions_scalar"]:
        problems.append(f"{path}: top-level permissions must be an explicit map")
    expected_permissions = TOP_LEVEL_PERMISSIONS.get(path)
    if expected_permissions is None:
        problems.append(f"{path}: workflow permissions policy is not declared")
    elif record["top_permissions"] != expected_permissions:
        problems.append(f"{path}: top-level permissions must be {expected_permissions}")

    concurrency = record["concurrency"]
    if not concurrency.get("group") or not concurrency.get("cancel-in-progress"):
        problems.append(f"{path}: concurrency must define group and cancel-in-progress")

    jobs = record["jobs"]
    if not jobs:
        problems.append(f"{path}: workflow must define at least one job")
    for job in jobs:
        job_id = str(job["id"])
        if job["timeout_minutes"] is None:
            problems.append(f"{path}:{job_id}: job must define timeout-minutes")
        expected_job_permissions = JOB_PERMISSIONS.get((path, job_id), {})
        if job["permissions"] != expected_job_permissions:
            problems.append(f"{path}:{job_id}: job permissions must be {expected_job_permissions}")
    return problems


def collect(root: Path = ROOT) -> dict[str, Any]:
    root = root.resolve()
    workflows = [workflow_record(path, root) for path in workflow_files(root)]
    problems: list[str] = []
    for record in workflows:
        problems.extend(validate_workflow(record))
    return {
        "workflow_count": len(workflows),
        "workflows": workflows,
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc workflow policy",
        f"- workflows: {evidence['workflow_count']}",
    ]
    for workflow in evidence["workflows"]:
        lines.append(f"  - {workflow['path']}: jobs={len(workflow['jobs'])}")
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
    parser = argparse.ArgumentParser(description="Validate GitHub workflow permissions and runtime policy.")
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
        print("arc workflow policy failed: workflow permissions or runtime policy regressed", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
