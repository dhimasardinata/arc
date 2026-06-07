#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, NamedTuple


ROOT = Path(__file__).resolve().parents[1]
EXPECTED_WORKFLOWS = (
    ".github/workflows/build.yml",
    ".github/workflows/codeql.yml",
    ".github/workflows/pages.yml",
)
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
EXPECTED_TRIGGERS = {
    ".github/workflows/build.yml": {
        "events": ("push", "pull_request"),
    },
    ".github/workflows/codeql.yml": {
        "events": ("push", "pull_request", "schedule", "workflow_dispatch"),
        "push_branches": ("main",),
        "schedule_crons": ("24 3 * * 1",),
    },
    ".github/workflows/pages.yml": {
        "events": ("push", "workflow_dispatch"),
        "push_branches": ("main",),
        "push_paths": (
            ".github/workflows/pages.yml",
            "docs/**",
            "README.md",
            "package.json",
            "package-lock.json",
        ),
    },
}
ARC_BASE_SHA_EXPR = "${{ github.event.pull_request.base.sha || github.event.before }}"
ARC_BASE_SHA_HEX_GUARD = '[[ ! "$ARC_BASE_SHA" =~ ^[0-9a-fA-F]{40}$ ]]'
RUFF_SPEC = "ruff==0.15.16"
RUNNER_IMAGE = "ubuntu-24.04"
PAGES_DEPLOY_REF_GUARD = "github.ref == 'refs/heads/main'"
CODEQL_DEPENDENCY_CACHING = "false"
BASH_STRICT_PREAMBLE = "set -euo pipefail"


class Step(NamedTuple):
    name: str
    if_expr: str | None
    uses: str | None
    shell: str | None
    continue_on_error: str | None
    with_inputs: dict[str, str]
    env: dict[str, str]
    run_block: bool
    run: str | None


class Job(NamedTuple):
    job_id: str
    if_expr: str | None
    runs_on: str | None
    timeout_minutes: int | None
    permissions: dict[str, str]
    steps: list[Step]


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


def clean_scalar(value: str) -> str:
    return value.strip().strip("\"'")


def scalar_sequence(value: str) -> list[str]:
    value = value.strip()
    if not value:
        return []
    if value.startswith("[") and value.endswith("]"):
        body = value[1:-1].strip()
        if not body:
            return []
        return [clean_scalar(item) for item in body.split(",")]
    return [clean_scalar(value)]


def parse_sequence_key(block: list[str], key: str, indent: int) -> list[str]:
    pattern = re.compile(rf"^ {{{indent}}}{re.escape(key)}:\s*(.*?)\s*$")
    item_pattern = re.compile(rf"^ {{{indent + 2},}}-\s*(.+?)\s*$")
    for index, line in enumerate(block):
        match = pattern.match(line)
        if not match:
            continue
        scalar = match.group(1).strip()
        if scalar:
            return scalar_sequence(scalar)
        items: list[str] = []
        for child in block[index + 1 :]:
            if child.strip() and leading_spaces(child) <= indent:
                break
            item_match = item_pattern.match(child)
            if item_match:
                items.extend(scalar_sequence(item_match.group(1)))
        return items
    return []


def parse_trigger_blocks(block: list[str]) -> dict[str, list[str]]:
    starts: list[tuple[int, str]] = []
    pattern = re.compile(r"^  ([A-Za-z0-9_-]+):\s*(.*?)\s*$")
    for offset, line in enumerate(block):
        match = pattern.match(line)
        if match:
            starts.append((offset, match.group(1)))

    triggers: dict[str, list[str]] = {}
    for index, (start, name) in enumerate(starts):
        end = starts[index + 1][0] if index + 1 < len(starts) else len(block)
        triggers[name] = block[start + 1 : end]
    return triggers


def parse_schedule_crons(block: list[str]) -> list[str]:
    crons: list[str] = []
    for line in block:
        match = re.match(r"^\s*-\s+cron:\s*(.+?)\s*$", line)
        if match:
            crons.append(clean_scalar(match.group(1)))
    return crons


def parse_triggers(lines: list[str]) -> dict[str, Any]:
    _, block, scalar = top_section(lines, "on")
    event_blocks = parse_trigger_blocks(block) if scalar is None else {}
    return {
        "scalar": scalar,
        "events": {
            name: {
                "branches": parse_sequence_key(event_block, "branches", 4),
                "paths": parse_sequence_key(event_block, "paths", 4),
                "crons": parse_schedule_crons(event_block),
            }
            for name, event_block in event_blocks.items()
        },
    }


def parse_step_name(line: str, fallback: int) -> str | None:
    name_match = re.match(r"^      - name:\s*(.+?)\s*$", line)
    if name_match:
        return name_match.group(1).strip("\"'")
    uses_match = re.match(r"^      - uses:\s*(.+?)\s*$", line)
    if uses_match:
        return f"uses:{uses_match.group(1).strip()}"
    if re.match(r"^      -\s+", line):
        return f"step-{fallback}"
    return None


def indented_child_block(block: list[str], start: int, parent_indent: int) -> list[str]:
    children: list[str] = []
    for line in block[start:]:
        if line.strip() and leading_spaces(line) <= parent_indent:
            break
        children.append(line)
    return children


def parse_step_env(step_block: list[str]) -> dict[str, str]:
    for index, line in enumerate(step_block):
        if re.match(r"^        env:\s*$", line):
            return mapping_from_block(indented_child_block(step_block, index + 1, 8), 10)
    return {}


def parse_step_with(step_block: list[str]) -> dict[str, str]:
    for index, line in enumerate(step_block):
        if re.match(r"^        with:\s*$", line):
            return mapping_from_block(indented_child_block(step_block, index + 1, 8), 10)
    return {}


def parse_step_run(step_block: list[str]) -> str | None:
    for index, line in enumerate(step_block):
        match = re.match(r"^        run:\s*(.*?)\s*$", line)
        if not match:
            continue
        scalar = match.group(1)
        if scalar and scalar not in {"|", ">"}:
            return scalar
        run_lines: list[str] = []
        for run_line in indented_child_block(step_block, index + 1, 8):
            if len(run_line) >= 10:
                run_lines.append(run_line[10:])
            else:
                run_lines.append(run_line.strip())
        return "\n".join(run_lines)
    return None


def parse_step_run_block(step_block: list[str]) -> bool:
    for line in step_block:
        match = re.match(r"^        run:\s*(.*?)\s*$", line)
        if match:
            return match.group(1) in {"|", ">"}
    return False


def parse_step_uses(step_lines: list[str]) -> str | None:
    for line in step_lines:
        match = re.match(r"^(?:      - uses:|        uses:)\s*(.+?)\s*$", line)
        if match:
            return match.group(1).split("#", 1)[0].strip()
    return None


def parse_step_if(step_lines: list[str]) -> str | None:
    for line in step_lines:
        match = re.match(r"^        if:\s*(.+?)\s*$", line)
        if match:
            return match.group(1)
    return None


def parse_step_shell(step_lines: list[str]) -> str | None:
    for line in step_lines:
        match = re.match(r"^        shell:\s*(.+?)\s*$", line)
        if match:
            return match.group(1).strip("\"'")
    return None


def parse_step_continue_on_error(step_lines: list[str]) -> str | None:
    for line in step_lines:
        match = re.match(r"^        continue-on-error:\s*(.+?)\s*$", line)
        if match:
            return match.group(1).strip("\"'")
    return None


def ruff_install_spec(run: str | None) -> str | None:
    if run is None:
        return None
    match = re.search(r"python3 -m pip install ['\"](?P<spec>ruff[^'\"]+)['\"]", run)
    return match.group("spec") if match else None


def has_bash_strict_preamble(run: str | None) -> bool:
    if run is None:
        return False
    lines = [line.strip() for line in run.splitlines() if line.strip()]
    return bool(lines) and lines[0] == BASH_STRICT_PREAMBLE


def parse_steps(job_block: list[str]) -> list[Step]:
    starts: list[tuple[int, str]] = []
    for offset, line in enumerate(job_block):
        name = parse_step_name(line, len(starts) + 1)
        if name is not None:
            starts.append((offset, name))

    steps: list[Step] = []
    for index, (start, name) in enumerate(starts):
        end = starts[index + 1][0] if index + 1 < len(starts) else len(job_block)
        step_lines = job_block[start:end]
        step_block = job_block[start + 1 : end]
        steps.append(
            Step(
                name=name,
                if_expr=parse_step_if(step_lines),
                uses=parse_step_uses(step_lines),
                shell=parse_step_shell(step_lines),
                continue_on_error=parse_step_continue_on_error(step_lines),
                with_inputs=parse_step_with(step_block),
                env=parse_step_env(step_block),
                run_block=parse_step_run_block(step_block),
                run=parse_step_run(step_block),
            )
        )
    return steps


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
        if_expr = None
        runs_on = None
        timeout = None
        permissions: dict[str, str] = {}
        for line_index, line in enumerate(job_block):
            if_match = re.match(r"^    if:\s*(.+?)\s*$", line)
            if if_match:
                if_expr = if_match.group(1)
            runner_match = re.match(r"^    runs-on:\s*(.+?)\s*$", line)
            if runner_match:
                runs_on = runner_match.group(1).strip("\"'")
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
        jobs.append(Job(job_id, if_expr, runs_on, timeout, permissions, parse_steps(job_block)))
    return jobs


def workflow_record(path: Path, root: Path) -> dict[str, Any]:
    lines = path.read_text(encoding="utf-8").splitlines()
    permissions, permission_scalar = parse_top_permissions(lines)
    jobs = parse_jobs(lines)
    return {
        "path": relpath(path, root),
        "triggers": parse_triggers(lines),
        "top_permissions": permissions,
        "top_permissions_scalar": permission_scalar,
        "concurrency": parse_concurrency(lines),
        "jobs": [
            {
                "id": job.job_id,
                "if": job.if_expr,
                "runs_on": job.runs_on,
                "timeout_minutes": job.timeout_minutes,
                "permissions": job.permissions,
                "steps": [
                    {
                        "name": step.name,
                        "if": step.if_expr,
                        "uses": step.uses,
                        "shell": step.shell,
                        "continue_on_error": step.continue_on_error,
                        "with": step.with_inputs,
                        "env": step.env,
                        "has_run": step.run is not None,
                        "run_block": step.run_block,
                        "bash_strict_preamble": has_bash_strict_preamble(step.run),
                        "github_expression_in_run": "${{" in (step.run or ""),
                        "arc_base_sha_guarded": ARC_BASE_SHA_HEX_GUARD in (step.run or "")
                        if "ARC_BASE_SHA" in step.env
                        else None,
                        "ruff_install_spec": ruff_install_spec(step.run),
                    }
                    for step in job.steps
                ],
            }
            for job in jobs
        ],
        "pull_request_target": any(re.match(r"^\s*pull_request_target\s*:", line) for line in lines),
    }


def validate_workflow(record: dict[str, Any]) -> list[str]:
    problems: list[str] = []
    path = str(record["path"])

    if record["pull_request_target"]:
        problems.append(f"{path}: pull_request_target is not allowed")

    triggers = record["triggers"]
    trigger_policy = EXPECTED_TRIGGERS.get(path)
    if triggers["scalar"]:
        problems.append(f"{path}: workflow triggers must be an explicit map")
    if trigger_policy is not None:
        expected_events = tuple(trigger_policy["events"])
        actual_events = tuple(triggers["events"].keys())
        if actual_events != expected_events:
            problems.append(f"{path}: workflow triggers must be {', '.join(expected_events)}")
        push = triggers["events"].get("push", {})
        push_branches = tuple(push.get("branches", []))
        expected_push_branches = tuple(trigger_policy.get("push_branches", ()))
        if push_branches != expected_push_branches:
            problems.append(f"{path}: push branches must be {', '.join(expected_push_branches) or '<all>'}")
        push_paths = tuple(push.get("paths", []))
        expected_push_paths = tuple(trigger_policy.get("push_paths", ()))
        if push_paths != expected_push_paths:
            problems.append(f"{path}: push paths must be {', '.join(expected_push_paths) or '<all>'}")
        schedule = triggers["events"].get("schedule", {})
        schedule_crons = tuple(schedule.get("crons", []))
        expected_schedule_crons = tuple(trigger_policy.get("schedule_crons", ()))
        if schedule_crons != expected_schedule_crons:
            problems.append(f"{path}: schedule cron entries must be {', '.join(expected_schedule_crons) or '<none>'}")

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
        if job["runs_on"] != RUNNER_IMAGE:
            problems.append(f"{path}:{job_id}: runs-on must pin {RUNNER_IMAGE}")
        if job["timeout_minutes"] is None:
            problems.append(f"{path}:{job_id}: job must define timeout-minutes")
        expected_job_permissions = JOB_PERMISSIONS.get((path, job_id), {})
        if job["permissions"] != expected_job_permissions:
            problems.append(f"{path}:{job_id}: job permissions must be {expected_job_permissions}")
        if (path, job_id) == (".github/workflows/pages.yml", "deploy") and job["if"] != PAGES_DEPLOY_REF_GUARD:
            problems.append(f"{path}:{job_id}: Pages deploy job must be guarded to refs/heads/main")
        for step in job["steps"]:
            step_name = str(step["name"])
            env = step["env"]
            if step["has_run"] and step["shell"] != "bash":
                problems.append(f"{path}:{job_id}:{step_name}: run step must set shell: bash")
            if step["run_block"] and step["bash_strict_preamble"] is not True:
                problems.append(f"{path}:{job_id}:{step_name}: multiline bash run must start with set -euo pipefail")
            if step["github_expression_in_run"]:
                problems.append(f"{path}:{job_id}:{step_name}: run block must not interpolate GitHub expressions")
            if "ARC_BASE_SHA" in env:
                if env["ARC_BASE_SHA"] != ARC_BASE_SHA_EXPR:
                    problems.append(f"{path}:{job_id}:{step_name}: ARC_BASE_SHA expression must stay reviewed")
                if step["arc_base_sha_guarded"] is not True:
                    problems.append(f"{path}:{job_id}:{step_name}: ARC_BASE_SHA must be guarded as a 40-hex commit")
            ruff_spec = step["ruff_install_spec"]
            if ruff_spec is not None and ruff_spec != RUFF_SPEC:
                problems.append(f"{path}:{job_id}:{step_name}: Ruff install must pin {RUFF_SPEC}")
            uses = step["uses"] or ""
            continue_on_error = step["continue_on_error"]
            if continue_on_error is not None and (
                continue_on_error != "true" or not uses.startswith("actions/cache/save@")
            ):
                problems.append(f"{path}:{job_id}:{step_name}: continue-on-error is only allowed on cache saves")
            if uses.startswith("actions/cache@"):
                problems.append(f"{path}:{job_id}:{step_name}: cache use must split restore and push-gated save")
            if uses.startswith("actions/cache/save@") and "github.event_name == 'push'" not in (step["if"] or ""):
                problems.append(f"{path}:{job_id}:{step_name}: cache save must be gated to push events")
            if uses.startswith("actions/cache/save@") and continue_on_error != "true":
                problems.append(f"{path}:{job_id}:{step_name}: cache save must set continue-on-error: true")
            if uses.startswith("actions/setup-node@") and "cache" in step["with"]:
                problems.append(
                    f"{path}:{job_id}:{step_name}: setup-node cache must use explicit restore and push-gated save"
                )
            if (
                uses.startswith("github/codeql-action/init@")
                and step["with"].get("dependency-caching") != CODEQL_DEPENDENCY_CACHING
            ):
                problems.append(f"{path}:{job_id}:{step_name}: CodeQL dependency caching must stay disabled")
    return problems


def collect(root: Path = ROOT) -> dict[str, Any]:
    root = root.resolve()
    workflows = [workflow_record(path, root) for path in workflow_files(root)]
    problems: list[str] = []
    actual_paths = tuple(workflow["path"] for workflow in workflows)
    for path in EXPECTED_WORKFLOWS:
        if path not in actual_paths:
            problems.append(f"{path}: required workflow is missing")
    for path in actual_paths:
        if path not in EXPECTED_WORKFLOWS:
            problems.append(f"{path}: workflow is not in the approved workflow set")
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
        step_count = sum(len(job["steps"]) for job in workflow["jobs"])
        lines.append(f"  - {workflow['path']}: jobs={len(workflow['jobs'])} steps={step_count}")
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
