#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SAFETY_CASE = ROOT / "docs" / "safety-case.md"
DOCS = [ROOT / "README.md", *sorted((ROOT / "docs").glob("*.md"))]

REQUIRED_NON_CLAIMS = (
    "certification for aerospace",
    "whole-program C++ lifetime safety",
    "std::span",
    "raw ESP-IDF driver interop",
    "board wiring",
)
OVERCLAIMS = (
    re.compile(r"\barc\s+is\s+certified\b", re.IGNORECASE),
    re.compile(r"(?<!not )\bcertified\s+to\s+MISRA\b", re.IGNORECASE),
    re.compile(r"\bISO\s+26262\s+certified\b", re.IGNORECASE),
    re.compile(r"\bIEC\s+61508\s+certified\b", re.IGNORECASE),
    re.compile(r"\bDO-178C\s+certified\b", re.IGNORECASE),
)
OUTPUT_FORMATS = ("text", "json")


def die(message: str) -> int:
    print(f"arc safety-case check failed: {message}", file=sys.stderr)
    return 1


def code_spans(text: str) -> list[str]:
    return re.findall(r"`([^`]+)`", text)


def path_from_code_span(value: str) -> str | None:
    head = value.strip().split(maxsplit=1)[0].removeprefix("./")
    if not head or head.startswith("arc::") or "<" in head or ">" in head:
        return None
    if "/" not in head and not (ROOT / head).exists():
        return None
    return head


def claim_rows(text: str) -> list[tuple[str, str]]:
    rows: list[tuple[str, str]] = []
    in_table = False
    for line in text.splitlines():
        if line == "| Claim | Evidence |":
            in_table = True
            continue
        if not in_table:
            continue
        if line == "| --- | --- |":
            continue
        if not line.startswith("|"):
            if rows:
                break
            continue
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        if len(cells) != 2:
            raise ValueError(f"malformed safety claim row: {line}")
        rows.append((cells[0], cells[1]))
    return rows


def required_commands(text: str) -> list[str]:
    commands: list[str] = []
    in_block = False
    for line in text.splitlines():
        if line.startswith("```"):
            in_block = not in_block
            continue
        if not in_block:
            continue
        stripped = line.strip()
        if stripped.startswith("./") or stripped.startswith("python3 tools/"):
            commands.append(stripped)
    return commands


def command_path(command: str) -> str:
    parts = command.split()
    if not parts:
        return ""
    if parts[0] in ("python", "python3") and len(parts) > 1:
        return parts[1].removeprefix("./")
    return parts[0].removeprefix("./")


def check_paths(rows: list[tuple[str, str]], commands: list[str]) -> list[str]:
    missing: list[str] = []
    for _, evidence in rows:
        refs = [path for span in code_spans(evidence) if (path := path_from_code_span(span))]
        if not refs:
            missing.append(f"claim evidence has no source path: {evidence}")
            continue
        for ref in refs:
            if not (ROOT / ref).exists():
                missing.append(f"claim evidence path does not exist: {ref}")

    for command in commands:
        path = command_path(command)
        if not path or not (ROOT / path).is_file():
            missing.append(f"required evidence command does not exist: {command}")
    return missing


def check_non_claims(text: str) -> list[str]:
    lower = text.lower()
    return [phrase for phrase in REQUIRED_NON_CLAIMS if phrase.lower() not in lower]


def check_overclaims() -> list[str]:
    bad: list[str] = []
    for path in DOCS:
        text = path.read_text(encoding="utf-8")
        for pattern in OVERCLAIMS:
            if pattern.search(text):
                bad.append(f"{path.relative_to(ROOT)} matches {pattern.pattern}")
    return bad


def evidence_paths(evidence: str) -> list[str]:
    paths: list[str] = []
    for span in code_spans(evidence):
        path = path_from_code_span(span)
        if path is not None and path not in paths:
            paths.append(path)
    return paths


def check_safety_case(text: str) -> tuple[list[tuple[str, str]], list[str], list[str]]:
    rows: list[tuple[str, str]] = []
    problems: list[str] = []
    if "not a functional-safety certificate" not in text:
        problems.append("safety case must say it is not a functional-safety certificate")
    if "Arc is not certified" not in text:
        problems.append("safety case must explicitly reject certification claims")

    try:
        rows = claim_rows(text)
    except ValueError as exc:
        problems.append(str(exc))

    if len(rows) < 8:
        problems.append("safety case has too few claim/evidence rows")

    commands = required_commands(text)
    problems.extend(check_paths(rows, commands))
    problems.extend(f"missing non-claim phrase: {phrase}" for phrase in check_non_claims(text))
    problems.extend(check_overclaims())
    return rows, commands, problems


def report(rows: list[tuple[str, str]], commands: list[str], problems: list[str]) -> dict[str, object]:
    return {
        "ok": not problems,
        "claims": [
            {
                "claim": claim,
                "evidence": evidence,
                "paths": evidence_paths(evidence),
            }
            for claim, evidence in rows
        ],
        "required_commands": commands,
        "non_claims": list(REQUIRED_NON_CLAIMS),
        "summary": {
            "claims": len(rows),
            "required_commands": len(commands),
            "non_claims": len(REQUIRED_NON_CLAIMS),
            "problems": len(problems),
        },
        "problems": problems,
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Check Arc safety-case evidence claims")
    parser.add_argument(
        "--format",
        choices=OUTPUT_FORMATS,
        default="text",
        help="output style: text status or JSON evidence summary",
    )
    args = parser.parse_args(argv)

    text = SAFETY_CASE.read_text(encoding="utf-8")
    rows, commands, problems = check_safety_case(text)
    if args.format == "json":
        print(json.dumps(report(rows, commands, problems), indent=2, sort_keys=True))

    if problems:
        if args.format == "text":
            for problem in problems:
                print(problem, file=sys.stderr)
        return die("safety evidence map is not source-backed")

    if args.format == "text":
        print(f"arc safety-case check: OK ({len(rows)} claims)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
