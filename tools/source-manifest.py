#!/usr/bin/env python3
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path
from typing import Any, NamedTuple


ROOT = Path(__file__).resolve().parents[1]


class TrackedFile(NamedTuple):
    mode: str
    object_id: str
    path: str


def git(args: list[str], *, binary: bool = False) -> bytes | str:
    output = subprocess.check_output(["git", *args], cwd=ROOT)
    if binary:
        return output
    return output.decode().strip()


def status_lines() -> list[str]:
    return str(git(["status", "--short", "--untracked-files=no"])).splitlines()


def tracked_files() -> list[TrackedFile]:
    output = git(["ls-files", "--stage", "-z"], binary=True)
    assert isinstance(output, bytes)
    files: list[TrackedFile] = []
    for raw in output.split(b"\0"):
        if not raw:
            continue
        meta, _, path = raw.partition(b"\t")
        parts = meta.decode().split()
        if len(parts) < 3:
            raise ValueError(f"unexpected git ls-files entry: {raw!r}")
        files.append(
            TrackedFile(
                mode=parts[0],
                object_id=parts[1],
                path=path.decode(),
            )
        )
    return sorted(files, key=lambda item: item.path)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def tree_sha256(records: list[dict[str, Any]]) -> str:
    digest = hashlib.sha256()
    for record in records:
        digest.update(str(record["mode"]).encode())
        digest.update(b"\0")
        digest.update(str(record["path"]).encode())
        digest.update(b"\0")
        digest.update(str(record["sha256"]).encode())
        digest.update(b"\0")
        digest.update(str(record["size"]).encode())
        digest.update(b"\0")
    return digest.hexdigest()


def collect() -> dict[str, Any]:
    records: list[dict[str, Any]] = []
    problems: list[str] = []
    for item in tracked_files():
        path = ROOT / item.path
        if not path.is_file():
            problems.append(f"tracked file missing: {item.path}")
            records.append(
                {
                    "path": item.path,
                    "mode": item.mode,
                    "git_object": item.object_id,
                    "size": None,
                    "sha256": None,
                }
            )
            continue
        records.append(
            {
                "path": item.path,
                "mode": item.mode,
                "git_object": item.object_id,
                "size": path.stat().st_size,
                "sha256": sha256(path),
            }
        )

    status = status_lines()
    return {
        "commit": git(["rev-parse", "HEAD"]),
        "branch": git(["rev-parse", "--abbrev-ref", "HEAD"]),
        "dirty": bool(status),
        "status": status,
        "file_count": len(records),
        "tree_sha256": tree_sha256(records) if not problems else None,
        "files": records,
        "ok": not problems,
        "problems": problems,
    }


def print_report(evidence: dict[str, Any]) -> None:
    print("arc source manifest")
    print(f"- commit: {evidence['commit']}")
    print(f"- branch: {evidence['branch']}")
    print(f"- dirty: {'yes' if evidence['dirty'] else 'no'}")
    print(f"- tracked files: {evidence['file_count']}")
    print(f"- tree sha256: {evidence['tree_sha256']}")
    if evidence["status"]:
        print("- status:")
        for line in evidence["status"]:
            print(f"  - {line}")
    if evidence["problems"]:
        print("- problems:")
        for problem in evidence["problems"]:
            print(f"  - {problem}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Emit a hash manifest for Arc tracked source files.")
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON manifest",
    )
    parser.add_argument(
        "--require-clean",
        action="store_true",
        help="fail when tracked files differ from HEAD",
    )
    args = parser.parse_args(argv)

    evidence = collect()
    if args.format == "json":
        print(json.dumps(evidence, indent=2, sort_keys=True))
    else:
        print_report(evidence)

    if evidence["problems"]:
        print("arc source manifest failed: tracked source cannot be hashed", file=sys.stderr)
        return 1
    if args.require_clean and evidence["dirty"]:
        print("arc source manifest failed: tracked source is dirty", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
