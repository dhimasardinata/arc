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
ARTIFACT_SUFFIXES = (".bin", ".elf")
DEFAULT_BUILD_DIRS = ("build", "examples")


def git(args: list[str], root: Path) -> str | None:
    try:
        return subprocess.check_output(["git", *args], cwd=root, text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return None


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as handle:
        for chunk in iter(lambda: handle.read(65536), b""):
            digest.update(chunk)
    return digest.hexdigest()


def artifact_kind(path: Path) -> str:
    if path.suffix == ".elf":
        return "elf"
    if path.name == "bootloader.bin":
        return "bootloader"
    if path.name == "partition-table.bin":
        return "partition-table"
    if path.name == "ota_data_initial.bin":
        return "ota-data"
    return "firmware-bin"


def repo_relpath(root: Path, path: Path) -> str | None:
    try:
        return path.resolve().relative_to(root).as_posix()
    except ValueError:
        return None


def find_artifacts(root: Path, search_roots: list[Path]) -> tuple[list[Path], list[str]]:
    artifacts: list[Path] = []
    problems: list[str] = []
    for search in search_roots:
        base = search if search.is_absolute() else root / search
        if repo_relpath(root, base) is None:
            problems.append(f"firmware search root must stay inside repository: {search}")
            continue
        if not base.exists():
            continue
        if base.is_file() and base.suffix in ARTIFACT_SUFFIXES:
            artifacts.append(base)
            continue
        if base.is_dir():
            build_dirs = [base]
            if base.name == "examples":
                build_dirs = []
                for path in base.rglob("build"):
                    if not path.is_dir():
                        continue
                    if repo_relpath(root, path) is None:
                        problems.append(f"firmware build directory must stay inside repository: {path}")
                        continue
                    build_dirs.append(path)
            for build_dir in build_dirs:
                for path in build_dir.iterdir():
                    if not path.is_file() or path.suffix not in ARTIFACT_SUFFIXES:
                        continue
                    if repo_relpath(root, path) is None:
                        problems.append(f"firmware artifact must stay inside repository: {path}")
                        continue
                    artifacts.append(path)
    return sorted(set(artifacts), key=lambda path: repo_relpath(root, path) or path.as_posix()), problems


def collect(root: Path, search_roots: list[Path]) -> dict[str, Any]:
    root = root.resolve()
    records: list[dict[str, Any]] = []
    artifacts, problems = find_artifacts(root, search_roots)
    for path in artifacts:
        rel = repo_relpath(root, path)
        if rel is None:
            problems.append(f"firmware artifact must stay inside repository: {path}")
            continue
        records.append(
            {
                "path": rel,
                "kind": artifact_kind(path),
                "size": path.stat().st_size,
                "sha256": sha256(path),
            }
        )

    status = git(["status", "--short", "--untracked-files=no"], root)
    return {
        "commit": git(["rev-parse", "HEAD"], root),
        "branch": git(["rev-parse", "--abbrev-ref", "HEAD"], root),
        "dirty": bool(status),
        "status": status.splitlines() if status else [],
        "artifact_count": len(records),
        "artifacts": records,
        "ok": not problems,
        "problems": problems,
    }


def print_report(evidence: dict[str, Any]) -> None:
    print(report_text(evidence))


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc firmware manifest",
        f"- commit: {evidence['commit']}",
        f"- branch: {evidence['branch']}",
        f"- dirty: {'yes' if evidence['dirty'] else 'no'}",
        f"- artifacts: {evidence['artifact_count']}",
    ]
    for record in evidence["artifacts"]:
        lines.append(f"  - {record['path']}: {record['kind']} size={record['size']} sha256={record['sha256']}")
    if evidence["problems"]:
        lines.append("- problems:")
        lines.extend(f"  - {problem}" for problem in evidence["problems"])
    return "\n".join(lines)


def write_output(path: Path, payload: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(payload + "\n", encoding="utf-8")


def payload_text(evidence: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(evidence, indent=2, sort_keys=True)
    return report_text(evidence)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Emit SHA-256 evidence for Arc firmware artifacts.")
    parser.add_argument(
        "search_roots",
        nargs="*",
        type=Path,
        help="build directories or artifact files to scan; defaults to build/ and examples/",
    )
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="repository root for relative paths",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON manifest",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="write output to this file instead of stdout",
    )
    parser.add_argument(
        "--require-artifacts",
        action="store_true",
        help="fail when no firmware artifacts are found",
    )
    args = parser.parse_args(argv)

    search_roots = args.search_roots or [Path(path) for path in DEFAULT_BUILD_DIRS]
    evidence = collect(args.root, search_roots)
    payload = payload_text(evidence, args.format)

    if args.output:
        write_output(args.output, payload)
    else:
        print(payload)

    if evidence["problems"]:
        print("arc firmware manifest failed: artifact search is outside repository scope", file=sys.stderr)
        return 1
    if args.require_artifacts and not evidence["artifacts"]:
        print("arc firmware manifest failed: no firmware artifacts found", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
