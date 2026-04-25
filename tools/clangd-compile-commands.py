#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "compile_commands.json"


def display(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def project_dirs() -> list[Path]:
    examples = ROOT / "examples"
    projects = [ROOT]
    if examples.is_dir():
        projects.extend(
            path
            for path in sorted(examples.iterdir())
            if path.is_dir() and (path / "CMakeLists.txt").is_file()
        )
    return projects


def default_databases() -> list[Path]:
    return [project / "build" / "compile_commands.json" for project in project_dirs()]


def database_from_arg(path: Path) -> Path:
    resolved = path.expanduser()
    if resolved.is_dir():
        return resolved / "build" / "compile_commands.json"
    return resolved


def source_key(entry: dict[str, Any], database: Path) -> str:
    source = entry.get("file")
    if not isinstance(source, str) or not source:
        return json.dumps(entry, sort_keys=True)

    path = Path(source)
    if not path.is_absolute():
        directory = entry.get("directory")
        base = Path(directory) if isinstance(directory, str) and directory else database.parent
        path = base / path
    return str(path.resolve())


def read_database(path: Path) -> list[dict[str, Any]]:
    try:
        data = json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as err:
        raise ValueError(f"{display(path)} is not valid JSON: {err}") from err

    if not isinstance(data, list):
        raise ValueError(f"{display(path)} is not a JSON array")

    commands: list[dict[str, Any]] = []
    for entry in data:
        if not isinstance(entry, dict):
            raise ValueError(f"{display(path)} contains a non-object entry")
        commands.append(entry)
    return commands


def merge(databases: list[Path]) -> list[dict[str, Any]]:
    commands: list[dict[str, Any]] = []
    seen: set[str] = set()

    for database in databases:
        for entry in read_database(database):
            key = source_key(entry, database)
            if key in seen:
                continue
            seen.add(key)
            commands.append(entry)

    return commands


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Merge ESP-IDF compile databases for clangd.",
    )
    parser.add_argument(
        "inputs",
        nargs="*",
        type=Path,
        help="Project directories or compile_commands.json files. Defaults to root and examples/* builds.",
    )
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help="Output compile_commands.json path.",
    )
    args = parser.parse_args()

    if args.inputs:
        databases = [database_from_arg(path) for path in args.inputs]
        missing = [path for path in databases if not path.is_file()]
        if missing:
            for path in missing:
                print(f"missing compile database: {display(path)}", file=sys.stderr)
            return 1
    else:
        databases = [path for path in default_databases() if path.is_file()]
        if not databases:
            print(
                "no ESP-IDF compile databases found; run idf.py reconfigure or idf.py build in the projects you want clangd to see",
                file=sys.stderr,
            )
            return 1

    try:
        commands = merge(databases)
    except ValueError as err:
        print(err, file=sys.stderr)
        return 1

    output = args.output.expanduser()
    if not output.is_absolute():
        output = ROOT / output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(commands, indent=2) + "\n", encoding="utf-8")

    print(
        f"merged {len(commands)} commands from {len(databases)} databases into {display(output)}",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
