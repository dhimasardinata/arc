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
OUTPUT_FORMATS = ("text", "report", "json")


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


def collect(artifact: Path | None, required: tuple[str, ...]) -> dict[str, Any]:
    cases: dict[str, dict[str, Any]] = {}
    problems: list[str] = []

    if artifact is None:
        problems.append("artifact path is required")
    elif not artifact.is_file():
        problems.append(f"artifact not found: {artifact}")
    else:
        try:
            cases, problems = load_records(artifact)
        except (OSError, UnicodeDecodeError) as exc:
            problems.append(f"{artifact}: cannot read artifact: {exc}")
        problems.extend(check_required(cases, required))

    return {
        "artifact": str(artifact) if artifact is not None else None,
        "required_cases": list(required),
        "record_count": len(cases),
        "cases": [cases[name] for name in sorted(cases)],
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc hil evidence check",
        f"- artifact: {evidence['artifact']}",
        f"- required cases: {len(evidence['required_cases'])}",
        f"- records: {evidence['record_count']}",
        f"- problems: {len(evidence['problems'])}",
    ]
    if evidence["required_cases"]:
        lines.append("- required:")
        lines.extend(f"  - {case}" for case in evidence["required_cases"])
    if evidence["cases"]:
        lines.append("- cases:")
        for record in evidence["cases"]:
            lines.append(f"  - {record['case']}: {record['status']}")
    if evidence["problems"]:
        lines.append("- problem details:")
        lines.extend(f"  - {problem}" for problem in evidence["problems"])
    return "\n".join(lines)


def payload_text(evidence: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(evidence, indent=2, sort_keys=True)
    if output_format == "report":
        return report_text(evidence)
    return f"arc hil evidence check: OK ({len(evidence['required_cases'])} required cases, {evidence['record_count']} records)"


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Validate Arc HIL JSONL evidence artifacts.")
    parser.add_argument("artifact", nargs="?", type=Path, help="JSONL file captured from the hardware test jig")
    parser.add_argument(
        "--require",
        action="append",
        default=[],
        help="Required case name. Repeat to override the default required set.",
    )
    parser.add_argument(
        "--format",
        choices=OUTPUT_FORMATS,
        default="text",
        help="output style: text status, human report, or JSON evidence",
    )
    parser.add_argument("--list-defaults", action="store_true", help="Print the default required case names.")
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(sys.argv[1:] if argv is None else argv)

    if args.list_defaults:
        for case in DEFAULT_CASES:
            print(case)
        return 0

    required = tuple(args.require) if args.require else DEFAULT_CASES
    evidence = collect(args.artifact, required)

    if args.format in ("json", "report") or evidence["ok"]:
        print(payload_text(evidence, args.format))

    if evidence["problems"]:
        print("arc hil evidence check failed:", file=sys.stderr)
        for problem in evidence["problems"]:
            print(f"- {problem}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
