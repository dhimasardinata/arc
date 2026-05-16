#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


DEFAULT_CASES = (
    "i2c_fault_recovery",
    "spi_fault_recovery",
    "can_priority_spam",
    "espnow_stale_recover",
)


def load_records(path: Path) -> tuple[dict[str, dict[str, Any]], list[str]]:
    cases: dict[str, dict[str, Any]] = {}
    problems: list[str] = []

    for lineno, raw in enumerate(path.read_text(encoding="utf-8").splitlines(), start=1):
        line = raw.strip()
        if not line or line.startswith("#"):
            continue

        try:
            record = json.loads(line)
        except json.JSONDecodeError as exc:
            problems.append(f"{path}:{lineno}: invalid JSON: {exc.msg}")
            continue

        if not isinstance(record, dict):
            problems.append(f"{path}:{lineno}: record must be a JSON object")
            continue

        case = record.get("case")
        status = record.get("status")
        if not isinstance(case, str) or not case:
            problems.append(f"{path}:{lineno}: missing string case")
            continue
        if not isinstance(status, str) or not status:
            problems.append(f"{path}:{lineno}: case {case}: missing string status")
            continue

        normalized = status.lower()
        record["status"] = normalized
        cases[case] = record
        if normalized != "pass":
            problems.append(f"{path}:{lineno}: case {case}: status is {status!r}, expected 'pass'")

    return cases, problems


def check_required(cases: dict[str, dict[str, Any]], required: tuple[str, ...]) -> list[str]:
    return [f"missing required HIL case: {case}" for case in required if case not in cases]


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate Arc HIL JSONL evidence artifacts.")
    parser.add_argument("artifact", nargs="?", type=Path, help="JSONL file captured from the hardware test jig")
    parser.add_argument(
        "--require",
        action="append",
        default=[],
        help="Required case name. Repeat to override the default required set.",
    )
    parser.add_argument("--list-defaults", action="store_true", help="Print the default required case names.")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    if args.list_defaults:
        for case in DEFAULT_CASES:
            print(case)
        return 0

    if args.artifact is None:
        print("arc hil evidence check failed: artifact path is required", file=sys.stderr)
        return 1

    if not args.artifact.is_file():
        print(f"arc hil evidence check failed: artifact not found: {args.artifact}", file=sys.stderr)
        return 1

    required = tuple(args.require) if args.require else DEFAULT_CASES
    cases, problems = load_records(args.artifact)
    problems.extend(check_required(cases, required))

    if problems:
        print("arc hil evidence check failed:", file=sys.stderr)
        for problem in problems:
            print(f"- {problem}", file=sys.stderr)
        return 1

    print(f"arc hil evidence check: OK ({len(required)} required cases, {len(cases)} records)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
