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


def git(args: list[str]) -> str | None:
    try:
        return subprocess.check_output(["git", *args], cwd=ROOT, text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def relpath(path: Path) -> str:
    resolved = path.resolve()
    try:
        return resolved.relative_to(ROOT).as_posix()
    except ValueError:
        return resolved.as_posix()


def collect(paths: list[Path], required: list[str]) -> dict[str, Any]:
    records: list[dict[str, Any]] = []
    problems: list[str] = []
    seen: set[str] = set()
    for raw_path in paths:
        path = raw_path if raw_path.is_absolute() else ROOT / raw_path
        name = relpath(path)
        if name in seen:
            problems.append(f"duplicate evidence path: {name}")
            continue
        seen.add(name)
        if not path.is_file():
            problems.append(f"missing evidence file: {name}")
            continue
        records.append(
            {
                "path": name,
                "size": path.stat().st_size,
                "sha256": sha256(path),
            }
        )

    for name in required:
        if name not in seen:
            problems.append(f"required evidence file not indexed: {name}")

    return {
        "commit": git(["rev-parse", "HEAD"]),
        "branch": git(["rev-parse", "--abbrev-ref", "HEAD"]),
        "ok": not problems,
        "file_count": len(records),
        "files": sorted(records, key=lambda record: record["path"]),
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc evidence index",
        f"- commit: {evidence['commit']}",
        f"- branch: {evidence['branch']}",
        f"- files: {evidence['file_count']}",
    ]
    for record in evidence["files"]:
        lines.append(f"  - {record['path']}: size={record['size']} sha256={record['sha256']}")
    if evidence["problems"]:
        lines.append("- problems:")
        for problem in evidence["problems"]:
            lines.append(f"  - {problem}")
    return "\n".join(lines)


def payload_text(evidence: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(evidence, indent=2, sort_keys=True)
    return report_text(evidence)


def write_output(path: Path, payload: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(payload + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Index and hash Arc evidence JSON files.")
    parser.add_argument(
        "paths",
        nargs="+",
        type=Path,
        help="evidence files to hash",
    )
    parser.add_argument(
        "--require",
        action="append",
        default=[],
        help="relative evidence path that must be present in the index; can be repeated",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON index",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="write output to this file instead of stdout",
    )
    args = parser.parse_args(argv)

    evidence = collect(args.paths, args.require)
    payload = payload_text(evidence, args.format)
    if args.output:
        write_output(args.output, payload)
    else:
        print(payload)

    if evidence["problems"]:
        print("arc evidence index failed: evidence set is incomplete", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
