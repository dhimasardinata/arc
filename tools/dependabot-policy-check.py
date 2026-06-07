#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEPENDABOT = ".github/dependabot.yml"
EXPECTED_UPDATES = {
    "github-actions": {
        "directory": "/",
        "target-branch": "main",
        "interval": "weekly",
        "day": "monday",
        "time": "04:00",
        "timezone": "Asia/Jakarta",
        "open-pull-requests-limit": "5",
        "group": "github-actions",
        "patterns": ["*"],
    },
    "npm": {
        "directory": "/",
        "target-branch": "main",
        "interval": "weekly",
        "day": "monday",
        "time": "04:30",
        "timezone": "Asia/Jakarta",
        "open-pull-requests-limit": "5",
        "group": "docs-dependencies",
        "patterns": ["*"],
    },
}


def clean_value(value: str) -> str:
    return value.split("#", 1)[0].strip().strip("\"'")


def leading_spaces(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def scalar(block: list[str], key: str) -> str | None:
    pattern = re.compile(rf"^\s+(?:-\s*)?{re.escape(key)}:\s*(.+?)\s*$")
    for line in block:
        match = pattern.match(line)
        if match:
            return clean_value(match.group(1))
    return None


def update_blocks(lines: list[str]) -> list[list[str]]:
    starts = [index for index, line in enumerate(lines) if re.match(r"^  - package-ecosystem:\s*", line)]
    blocks: list[list[str]] = []
    for offset, start in enumerate(starts):
        end = starts[offset + 1] if offset + 1 < len(starts) else len(lines)
        blocks.append(lines[start:end])
    return blocks


def group_policy(block: list[str]) -> dict[str, Any]:
    groups_index = next((index for index, line in enumerate(block) if re.match(r"^    groups:\s*$", line)), None)
    if groups_index is None:
        return {"name": None, "patterns": []}

    for index in range(groups_index + 1, len(block)):
        group_match = re.match(r"^      ([A-Za-z0-9_-]+):\s*$", block[index])
        if not group_match:
            continue
        patterns: list[str] = []
        for line in block[index + 1 :]:
            if line.strip() and leading_spaces(line) <= 6:
                break
            pattern_match = re.match(r"^\s+-\s*(.+?)\s*$", line)
            if pattern_match:
                patterns.append(clean_value(pattern_match.group(1)))
        return {"name": group_match.group(1), "patterns": patterns}

    return {"name": None, "patterns": []}


def parse_updates(lines: list[str]) -> list[dict[str, Any]]:
    updates: list[dict[str, Any]] = []
    for block in update_blocks(lines):
        group = group_policy(block)
        updates.append(
            {
                "package-ecosystem": scalar(block, "package-ecosystem"),
                "directory": scalar(block, "directory"),
                "target-branch": scalar(block, "target-branch"),
                "interval": scalar(block, "interval"),
                "day": scalar(block, "day"),
                "time": scalar(block, "time"),
                "timezone": scalar(block, "timezone"),
                "open-pull-requests-limit": scalar(block, "open-pull-requests-limit"),
                "group": group["name"],
                "patterns": group["patterns"],
            }
        )
    return updates


def top_version(lines: list[str]) -> str | None:
    for line in lines:
        match = re.match(r"^version:\s*(.+?)\s*$", line)
        if match:
            return clean_value(match.group(1))
    return None


def validate_updates(updates: list[dict[str, Any]]) -> list[str]:
    problems: list[str] = []
    by_ecosystem: dict[str, dict[str, Any]] = {}
    for update in updates:
        ecosystem = update.get("package-ecosystem")
        if not ecosystem:
            problems.append(f"{DEPENDABOT}: update entry must declare package-ecosystem")
            continue
        if ecosystem in by_ecosystem:
            problems.append(f"{DEPENDABOT}: duplicate update entry for {ecosystem}")
        by_ecosystem[ecosystem] = update

    for ecosystem in by_ecosystem:
        if ecosystem not in EXPECTED_UPDATES:
            problems.append(f"{DEPENDABOT}: unexpected ecosystem {ecosystem}")

    for ecosystem, expected in EXPECTED_UPDATES.items():
        update = by_ecosystem.get(ecosystem)
        if update is None:
            problems.append(f"{DEPENDABOT}: missing update entry for {ecosystem}")
            continue
        for key, expected_value in expected.items():
            actual = update.get(key)
            if actual != expected_value:
                problems.append(f"{DEPENDABOT}: {ecosystem} {key} must be {expected_value!r}, got {actual!r}")

    return problems


def collect(root: Path = ROOT) -> dict[str, Any]:
    root = root.resolve()
    path = root / DEPENDABOT
    if not path.is_file():
        return {
            "path": DEPENDABOT,
            "ok": False,
            "problems": [f"{DEPENDABOT}: missing Dependabot configuration"],
        }

    lines = path.read_text(encoding="utf-8").splitlines()
    version = top_version(lines)
    updates = parse_updates(lines)
    problems: list[str] = []
    if version != "2":
        problems.append(f"{DEPENDABOT}: version must be '2', got {version!r}")
    problems.extend(validate_updates(updates))

    return {
        "path": DEPENDABOT,
        "version": version,
        "expected": EXPECTED_UPDATES,
        "updates": updates,
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc dependabot policy",
        f"- path: {evidence['path']}",
        f"- updates: {len(evidence.get('updates', []))}",
    ]
    for update in evidence.get("updates", []):
        lines.append(
            f"  - {update.get('package-ecosystem')}: {update.get('directory')} "
            f"{update.get('interval')} {update.get('day')} {update.get('time')} "
            f"group={update.get('group')}"
        )
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
    parser = argparse.ArgumentParser(description="Validate Arc Dependabot version-update policy.")
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
        print("arc dependabot policy failed: version-update policy drifted", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
