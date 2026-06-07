#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
from json import JSONDecodeError
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


def repo_path(path: Path) -> tuple[Path, str | None]:
    full = path if path.is_absolute() else ROOT / path
    resolved = full.resolve()
    try:
        return full, resolved.relative_to(ROOT).as_posix()
    except ValueError:
        return full, None


def json_state(path: Path, name: str) -> tuple[dict[str, Any], list[str]]:
    try:
        payload = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeDecodeError, JSONDecodeError) as exc:
        return {
            "valid": False,
            "ok": None,
            "problem_count": None,
        }, [f"{name}: invalid JSON evidence: {exc}"]

    if not isinstance(payload, dict):
        return {
            "valid": True,
            "ok": None,
            "problem_count": None,
        }, [f"{name}: evidence JSON root must be an object"]

    problems: list[str] = []
    ok = payload.get("ok")
    if "ok" in payload:
        if not isinstance(ok, bool):
            problems.append(f"{name}: evidence ok field must be boolean")
        elif not ok:
            problems.append(f"{name}: evidence ok field is false")

    problem_count: int | None = None
    reported = payload.get("problems")
    if "problems" in payload:
        if not isinstance(reported, list):
            problems.append(f"{name}: evidence problems field must be a list")
        else:
            problem_count = len(reported)
            if reported:
                problems.append(f"{name}: evidence reports {problem_count} problem(s)")

    return {
        "valid": True,
        "ok": ok if isinstance(ok, bool) else None,
        "problem_count": problem_count,
    }, problems


def collect(paths: list[Path], required: list[str]) -> dict[str, Any]:
    records: list[dict[str, Any]] = []
    problems: list[str] = []
    seen: set[str] = set()
    for raw_path in paths:
        path, name = repo_path(raw_path)
        if name is None:
            problems.append(f"evidence path must stay inside repository: {raw_path}")
            continue
        if name in seen:
            problems.append(f"duplicate evidence path: {name}")
            continue
        seen.add(name)
        if not path.is_file():
            problems.append(f"missing evidence file: {name}")
            continue
        state, state_problems = json_state(path, name)
        problems.extend(state_problems)
        records.append(
            {
                "path": name,
                "size": path.stat().st_size,
                "sha256": sha256(path),
                "json": state,
            }
        )

    for raw_required in required:
        _, name = repo_path(Path(raw_required))
        if name is None:
            problems.append(f"required evidence path must stay inside repository: {raw_required}")
            continue
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
        state = record["json"]
        details = f"json={'valid' if state['valid'] else 'invalid'}"
        if state["ok"] is not None:
            details += f" ok={str(state['ok']).lower()}"
        if state["problem_count"] is not None:
            details += f" problems={state['problem_count']}"
        lines.append(f"  - {record['path']}: size={record['size']} sha256={record['sha256']} {details}")
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
