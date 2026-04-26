#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import shlex
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUTPUT = ROOT / "compile_commands.json"
ARC_INCLUDE_DIR = ROOT / "components" / "arc" / "include"
CXX_SOURCE_SUFFIXES = {".cc", ".cpp", ".cxx", ".c++"}
HEADER_SUFFIXES = {".h", ".hh", ".hpp", ".hxx"}
INCLUDE_RE = re.compile(r"^\s*#\s*include\s*([<\"])([^>\"]+)[>\"]")
INCLUDE_FLAGS = {"-I", "-isystem", "-iquote", "-idirafter"}
INCLUDE_PREFIX_FLAGS = tuple(sorted(INCLUDE_FLAGS, key=len, reverse=True))
PUBLIC_REQ_KEYS = ("reqs", "managed_reqs")


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
            path for path in sorted(examples.iterdir()) if path.is_dir() and (path / "CMakeLists.txt").is_file()
        )
    return projects


def default_databases() -> list[Path]:
    return [project / "build" / "compile_commands.json" for project in project_dirs()]


def database_from_arg(path: Path) -> Path:
    resolved = path.expanduser()
    if resolved.is_dir():
        return resolved / "build" / "compile_commands.json"
    return resolved


def entry_source(entry: dict[str, Any], fallback_base: Path = ROOT) -> Path | None:
    source = entry.get("file")
    if not isinstance(source, str) or not source:
        return None

    path = Path(source)
    if path.is_absolute():
        return path.resolve()

    directory = entry.get("directory")
    base = Path(directory) if isinstance(directory, str) and directory else fallback_base
    return (base / path).resolve()


def source_key(entry: dict[str, Any], database: Path) -> str:
    source = entry_source(entry, database.parent)
    if source is None:
        return json.dumps(entry, sort_keys=True)

    return str(source)


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


def entry_args(entry: dict[str, Any]) -> list[str]:
    arguments = entry.get("arguments")
    if isinstance(arguments, list) and all(isinstance(arg, str) for arg in arguments):
        return list(arguments)

    command = entry.get("command")
    if isinstance(command, str) and command:
        return shlex.split(command)

    return []


def entry_directory(entry: dict[str, Any]) -> Path:
    directory = entry.get("directory")
    if isinstance(directory, str) and directory:
        return Path(directory).resolve()
    return ROOT


def include_dirs(entry: dict[str, Any]) -> tuple[Path, ...]:
    args = entry_args(entry)
    base = entry_directory(entry)
    paths: list[Path] = []
    index = 0

    while index < len(args):
        arg = args[index]
        value: str | None = None

        if arg in INCLUDE_FLAGS and index + 1 < len(args):
            value = args[index + 1]
            index += 2
        else:
            for flag in INCLUDE_PREFIX_FLAGS:
                if arg.startswith(flag) and len(arg) > len(flag):
                    value = arg[len(flag) :]
                    break
            index += 1

        if value:
            path = Path(value)
            if not path.is_absolute():
                path = base / path
            paths.append(path.resolve())

    return tuple(paths)


def direct_quoted_includes(header: Path) -> list[str]:
    includes: list[str] = []
    for line in header.read_text(encoding="utf-8").splitlines():
        match = INCLUDE_RE.match(line)
        if match and match.group(1) == '"':
            includes.append(match.group(2))
    return includes


def has_include(include: str, dirs: tuple[Path, ...], cache: dict[tuple[Path, str], bool]) -> bool:
    for directory in dirs:
        key = (directory, include)
        found = cache.get(key)
        if found is None:
            found = (directory / include).is_file()
            cache[key] = found
        if found:
            return True
    return False


def under(path: Path, parent: Path) -> bool:
    try:
        path.relative_to(parent)
    except ValueError:
        return False
    return True


def cxx_candidates(commands: list[dict[str, Any]]) -> list[tuple[int, dict[str, Any], tuple[Path, ...]]]:
    candidates: list[tuple[int, dict[str, Any], tuple[Path, ...]]] = []
    for index, entry in enumerate(commands):
        source = entry_source(entry)
        if source is None or source.suffix.lower() not in CXX_SOURCE_SUFFIXES:
            continue
        candidates.append((index, entry, include_dirs(entry)))

    def rank(candidate: tuple[int, dict[str, Any], tuple[Path, ...]]) -> tuple[int, int, int]:
        index, entry, _ = candidate
        source = entry_source(entry)
        if source is None:
            return (2, 1, index)
        project_source = under(source, ROOT) and not under(source, ROOT / "esp-idf")
        app_source = source.name == "app_main.cpp"
        return (0 if project_source else 1, 0 if app_source else 1, index)

    return sorted(candidates, key=rank)


def project_description_paths(databases: list[Path]) -> list[Path]:
    descriptions: list[Path] = []
    seen: set[Path] = set()

    for database in databases:
        description = database.parent / "project_description.json"
        if description.is_file() and description not in seen:
            descriptions.append(description)
            seen.add(description)

    return descriptions


def component_info(databases: list[Path]) -> dict[str, dict[str, Any]]:
    components: dict[str, dict[str, Any]] = {}

    for description in project_description_paths(databases):
        try:
            data = json.loads(description.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            continue

        infos = data.get("all_component_info") or data.get("build_component_info")
        if not isinstance(infos, dict):
            continue

        for name, info in infos.items():
            if not isinstance(name, str) or name in components or not isinstance(info, dict):
                continue

            root = info.get("dir")
            includes = info.get("include_dirs")
            if not isinstance(root, str) or not isinstance(includes, list):
                continue

            include_dirs: list[Path] = []
            for include in includes:
                if not isinstance(include, str):
                    continue
                path = Path(include)
                if not path.is_absolute():
                    path = Path(root) / path
                include_dirs.append(path.resolve())

            reqs: list[str] = []
            for key in PUBLIC_REQ_KEYS:
                value = info.get(key)
                if isinstance(value, list):
                    reqs.extend(req for req in value if isinstance(req, str))

            components[name] = {
                "include_dirs": tuple(include_dirs),
                "reqs": tuple(reqs),
            }

    return components


def include_owner(include: str, components: dict[str, dict[str, Any]]) -> str | None:
    for name, info in components.items():
        dirs = info.get("include_dirs")
        if not isinstance(dirs, tuple):
            continue
        if any((directory / include).is_file() for directory in dirs):
            return name
    return None


def component_public_dirs(
    name: str,
    components: dict[str, dict[str, Any]],
    seen: set[str] | None = None,
) -> list[Path]:
    if seen is None:
        seen = set()
    if name in seen:
        return []
    seen.add(name)

    info = components.get(name)
    if not info:
        return []

    dirs = list(info.get("include_dirs", ()))
    for req in info.get("reqs", ()):
        dirs.extend(component_public_dirs(req, components, seen))
    return dirs


def missing_include_dirs(
    includes: list[str],
    dirs: tuple[Path, ...],
    components: dict[str, dict[str, Any]],
    cache: dict[tuple[Path, str], bool],
) -> tuple[Path, ...] | None:
    available = set(dirs)
    extras: list[Path] = []

    for include in includes:
        search_dirs = dirs + tuple(extras)
        if has_include(include, search_dirs, cache):
            continue

        owner = include_owner(include, components)
        if owner is None:
            return None

        for directory in component_public_dirs(owner, components):
            if directory not in available:
                extras.append(directory)
                available.add(directory)

        if not has_include(include, dirs + tuple(extras), cache):
            return None

    return tuple(extras)


def command_for_file(
    entry: dict[str, Any],
    file: Path,
    extra_include_dirs: tuple[Path, ...] = (),
) -> dict[str, Any]:
    clone = dict(entry)
    clone["file"] = str(file)

    args = entry_args(entry)
    source = entry_source(entry)
    if args and source is not None:
        source_spellings = {str(source)}
        original = entry.get("file")
        if isinstance(original, str) and original:
            source_spellings.add(original)
        try:
            source_spellings.add(str(source.relative_to(entry_directory(entry))))
        except ValueError:
            pass

        args = [str(file) if arg in source_spellings else arg for arg in args]
        if "arguments" in clone:
            clone["arguments"] = args
            clone.pop("command", None)
        else:
            clone["command"] = shlex.join(args)

    if args and extra_include_dirs:
        args = [args[0], *(f"-I{path}" for path in extra_include_dirs), *args[1:]]
        if "arguments" in clone:
            clone["arguments"] = args
            clone.pop("command", None)
        else:
            clone["command"] = shlex.join(args)

    return clone


def arc_header_commands(commands: list[dict[str, Any]], databases: list[Path]) -> list[dict[str, Any]]:
    if not ARC_INCLUDE_DIR.is_dir():
        return []

    candidates = cxx_candidates(commands)
    components = component_info(databases)
    include_cache: dict[tuple[Path, str], bool] = {}
    header_commands: list[dict[str, Any]] = []

    for header in sorted(ARC_INCLUDE_DIR.rglob("*")):
        if header.suffix.lower() not in HEADER_SUFFIXES:
            continue

        includes = direct_quoted_includes(header)
        fallback: tuple[dict[str, Any], tuple[Path, ...]] | None = None
        for _, entry, dirs in candidates:
            if all(has_include(include, dirs, include_cache) for include in includes):
                header_commands.append(command_for_file(entry, header.resolve()))
                break
            extras = missing_include_dirs(includes, dirs, components, include_cache)
            if extras is not None and fallback is None:
                fallback = (entry, extras)
        else:
            if fallback is not None:
                entry, extras = fallback
                header_commands.append(command_for_file(entry, header.resolve(), extras))

    return header_commands


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
    parser.add_argument(
        "--no-arc-headers",
        action="store_true",
        help="Do not add exact compile commands for public Arc headers.",
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

    header_commands: list[dict[str, Any]] = []
    if not args.no_arc_headers:
        header_commands = arc_header_commands(commands, databases)
        commands.extend(header_commands)

    output = args.output.expanduser()
    if not output.is_absolute():
        output = ROOT / output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(commands, indent=2) + "\n", encoding="utf-8")

    print(
        f"merged {len(commands)} commands from {len(databases)} databases into {display(output)}"
        f" ({len(header_commands)} Arc header commands)",
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
