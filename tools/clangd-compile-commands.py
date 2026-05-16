#!/usr/bin/env python3

from __future__ import annotations

import argparse
import concurrent.futures
import json
import os
import re
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.dont_write_bytecode = True

from arc_projects import projects as arc_projects  # noqa: E402

DEFAULT_OUTPUT = ROOT / "compile_commands.json"
ARC_INCLUDE_DIR = ROOT / "components" / "arc" / "include"
CXX_SOURCE_SUFFIXES = {".cc", ".cpp", ".cxx", ".c++"}
HEADER_SUFFIXES = {".h", ".hh", ".hpp", ".hxx"}
INCLUDE_RE = re.compile(r"^\s*#\s*include\s*([<\"])([^>\"]+)[>\"]")
ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")
INCLUDE_FLAGS = {"-I", "-isystem", "-iquote", "-idirafter"}
INCLUDE_PREFIX_FLAGS = tuple(sorted(INCLUDE_FLAGS, key=len, reverse=True))
SYSTEM_INCLUDE_MARKER = "#include <...> search starts here:"
INCLUDE_END_MARKER = "End of search list."
PUBLIC_REQ_KEYS = ("reqs", "managed_reqs")
CMAKE_COMPONENT_KEYS = {
    "SRCS",
    "SRC_DIRS",
    "EXCLUDE_SRCS",
    "INCLUDE_DIRS",
    "PRIV_INCLUDE_DIRS",
    "LDFRAGMENTS",
    "REQUIRES",
    "PRIV_REQUIRES",
    "EMBED_FILES",
    "EMBED_TXTFILES",
    "WHOLE_ARCHIVE",
}
ESP_TARGET_DIR_RE = re.compile(r"^esp32[a-z0-9]*$")


def esp32s3_compatible_path(path: Path) -> bool:
    parts = path.parts
    for part in parts:
        if part in {"test", "tests", "dummy", "mynewt", "nuttx", "riot"}:
            return False
        if part == "openthread":
            return False
        if part == "riscv":
            return False
        if part == "FreeRTOS-Kernel-SMP":
            return False
        if part == "linux":
            return False
        if ESP_TARGET_DIR_RE.match(part) and part != "esp32s3":
            return False
    return True


def display(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def project_dirs() -> list[Path]:
    return [project.path for project in arc_projects()]


def default_database_for_project(project: Path) -> Path | None:
    candidates = [
        path
        for path in (project / "build", *sorted(project.glob("build-*")))
        if (path / "compile_commands.json").is_file()
    ]
    if not candidates:
        return None
    return max(candidates, key=lambda path: (path / "compile_commands.json").stat().st_mtime) / "compile_commands.json"


def database_directories_exist(database: Path) -> bool:
    try:
        entries = read_database(database)
    except ValueError:
        return True

    for entry in entries:
        directory = entry.get("directory")
        if isinstance(directory, str) and directory and not Path(directory).is_dir():
            return False
    return True


def default_databases() -> list[Path]:
    return [
        database
        for project in project_dirs()
        if (database := default_database_for_project(project)) is not None and database_directories_exist(database)
    ]


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


def split_compile_command(command: str) -> list[str]:
    if fast_whitespace_split_safe(command):
        return [unescape_fast_arg(arg) for arg in command.split()]

    args: list[str] = []
    current: list[str] = []
    quote: str | None = None
    started = False
    index = 0

    while index < len(command):
        char = command[index]
        if quote == "'":
            if char == "'":
                quote = None
            else:
                current.append(char)
            started = True
            index += 1
            continue

        if quote == '"':
            if char == '"':
                quote = None
            elif char == "\\" and index + 1 < len(command) and command[index + 1] in {'"', "\\", "$", "`", "\n"}:
                index += 1
                if command[index] != "\n":
                    current.append(command[index])
            else:
                current.append(char)
            started = True
            index += 1
            continue

        if char.isspace():
            if started:
                args.append("".join(current))
                current.clear()
                started = False
            index += 1
            continue

        if char in {"'", '"'}:
            quote = char
            started = True
            index += 1
            continue

        if char == "\\":
            started = True
            index += 1
            if index >= len(command):
                current.append("\\")
            elif command[index] != "\n":
                current.append(command[index])
            index += 1
            continue

        current.append(char)
        started = True
        index += 1

    if quote is not None:
        raise ValueError("No closing quotation")
    if started:
        args.append("".join(current))
    return args


def fast_whitespace_split_safe(command: str) -> bool:
    if "'" in command:
        return False

    index = command.find("\\")
    while index >= 0:
        next_index = index + 1
        if next_index >= len(command) or command[next_index].isspace():
            return False
        index = command.find("\\", index + 2)

    index = 0
    while True:
        start = command.find('"', index)
        while start > 0 and command[start - 1] == "\\":
            start = command.find('"', start + 1)
        if start < 0:
            return True

        end = command.find('"', start + 1)
        while end > 0 and command[end - 1] == "\\":
            end = command.find('"', end + 1)
        if end < 0:
            return False
        if any(char.isspace() for char in command[start + 1 : end]):
            return False
        index = end + 1


def unescape_fast_arg(arg: str) -> str:
    if "\\" not in arg and "'" not in arg and '"' not in arg:
        return arg

    value: list[str] = []
    quote: str | None = None
    index = 0
    while index < len(arg):
        char = arg[index]

        if quote == "'":
            if char == "'":
                quote = None
            else:
                value.append(char)
            index += 1
            continue

        if quote == '"':
            if char == '"':
                quote = None
            elif char == "\\" and index + 1 < len(arg) and arg[index + 1] in {'"', "\\", "$", "`", "\n"}:
                index += 1
                if arg[index] != "\n":
                    value.append(arg[index])
            else:
                value.append(char)
            index += 1
            continue

        if char in {"'", '"'}:
            quote = char
        elif char == "\\" and index + 1 < len(arg):
            index += 1
            value.append(arg[index])
        else:
            value.append(char)
        index += 1
    return "".join(value)


def normalized_entry(entry: dict[str, Any]) -> dict[str, Any]:
    arguments = entry.get("arguments")
    if isinstance(arguments, list) and all(isinstance(arg, str) for arg in arguments):
        return dict(entry)

    command = entry.get("command")
    if not isinstance(command, str) or not command:
        return dict(entry)

    clone = dict(entry)
    try:
        clone["arguments"] = split_compile_command(command)
    except ValueError:
        clone["arguments"] = shlex.split(command)
    clone.pop("command", None)
    return clone


def merge(databases: list[Path]) -> list[dict[str, Any]]:
    commands: list[dict[str, Any]] = []
    seen: set[str] = set()

    for database in databases:
        for entry in read_database(database):
            key = source_key(entry, database)
            if key in seen:
                continue
            seen.add(key)
            commands.append(normalized_entry(entry))

    return commands


def entry_args(entry: dict[str, Any]) -> list[str]:
    arguments = entry.get("arguments")
    if isinstance(arguments, list) and all(isinstance(arg, str) for arg in arguments):
        return list(arguments)

    command = entry.get("command")
    if isinstance(command, str) and command:
        return shlex.split(command)

    return []


def compiler_probe_args(args: list[str]) -> tuple[str, ...]:
    if not args:
        return ()

    probe = [args[0]]
    has_specs = False
    index = 1
    while index < len(args):
        arg = args[index]
        if arg in {
            "--target",
            "-target",
            "-mcpu",
            "-march",
            "-mabi",
            "-mfloat-abi",
            "-mfpu",
            "-isysroot",
            "--sysroot",
            "-specs",
        }:
            if index + 1 < len(args):
                probe.extend((arg, args[index + 1]))
                has_specs = has_specs or arg == "-specs"
                index += 2
                continue
        elif arg.startswith(
            ("--target=", "-mcpu=", "-march=", "-mabi=", "-mfloat-abi=", "-mfpu=", "--sysroot=", "-specs=")
        ):
            probe.append(arg)
            has_specs = has_specs or arg.startswith("-specs=")
        elif arg.startswith("-m") and not arg.startswith(("-mllvm", "-MF", "-MT", "-MQ")):
            probe.append(arg)
        index += 1

    if not has_specs and (Path(args[0]).resolve().parents[1] / "xtensa-esp-elf" / "lib" / "picolibc.specs").is_file():
        probe.append("-specs=picolibc.specs")

    return tuple(probe)


def compiler_builtin_include_dirs(args: list[str], cache: dict[tuple[str, ...], tuple[Path, ...]]) -> tuple[Path, ...]:
    probe = compiler_probe_args(args)
    if not probe:
        return ()

    cached = cache.get(probe)
    if cached is not None:
        return cached

    try:
        proc = subprocess.run(
            [*probe, "-E", "-x", "c++", "-", "-v"],
            input="",
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )
    except (FileNotFoundError, PermissionError):
        cache[probe] = ()
        return ()

    includes: list[Path] = []
    capture = False
    for raw_line in proc.stderr.splitlines():
        line = raw_line.strip()
        if line == SYSTEM_INCLUDE_MARKER:
            capture = True
            continue
        if line == INCLUDE_END_MARKER:
            break
        if capture and line and not line.startswith("(framework directory)"):
            path = Path(line).expanduser().resolve()
            if path.is_dir():
                includes.append(path)

    resolved = tuple(dict.fromkeys(includes))
    cache[probe] = resolved
    return resolved


def raw_include_values(args: list[str]) -> set[str]:
    values: set[str] = set()
    index = 0
    while index < len(args):
        arg = args[index]
        if arg in INCLUDE_FLAGS and index + 1 < len(args):
            values.add(args[index + 1])
            index += 2
            continue
        for flag in INCLUDE_PREFIX_FLAGS:
            if arg.startswith(flag) and len(arg) > len(flag):
                values.add(arg[len(flag) :])
                break
        index += 1
    return values


def add_builtin_include_dirs(
    entry: dict[str, Any],
    cache: dict[tuple[str, ...], tuple[Path, ...]],
) -> dict[str, Any]:
    args = entry_args(entry)
    if not args:
        return entry

    existing = raw_include_values(args)
    builtin_dirs = tuple(path for path in compiler_builtin_include_dirs(args, cache) if str(path) not in existing)
    if not builtin_dirs:
        return entry

    clone = dict(entry)
    enriched = [args[0], *(arg for path in builtin_dirs for arg in ("-isystem", str(path))), *args[1:]]
    if "arguments" in clone:
        clone["arguments"] = enriched
        clone.pop("command", None)
    else:
        clone["command"] = shlex.join(enriched)
    return clone


def add_builtin_includes(commands: list[dict[str, Any]]) -> list[dict[str, Any]]:
    cache: dict[tuple[str, ...], tuple[Path, ...]] = {}
    return [add_builtin_include_dirs(command, cache) for command in commands]


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


def include_directives(header: Path) -> list[str]:
    includes: list[str] = []
    for line in header.read_text(encoding="utf-8").splitlines():
        match = INCLUDE_RE.match(line)
        if match:
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


def arc_header_index() -> dict[str, Path]:
    headers: dict[str, Path] = {}
    if not ARC_INCLUDE_DIR.is_dir():
        return headers

    for header in sorted(ARC_INCLUDE_DIR.rglob("*")):
        if header.suffix.lower() not in HEADER_SUFFIXES:
            continue
        headers[header.resolve().relative_to(ARC_INCLUDE_DIR).as_posix()] = header.resolve()
    return headers


def resolve_arc_include(include: str, current: Path, headers: dict[str, Path]) -> Path | None:
    direct = headers.get(include)
    if direct is not None:
        return direct

    candidate = (current.parent / include).resolve()
    if not under(candidate, ARC_INCLUDE_DIR) or not candidate.is_file():
        return None

    try:
        relative = candidate.relative_to(ARC_INCLUDE_DIR).as_posix()
    except ValueError:
        return None
    return headers.get(relative)


def header_external_includes(
    header: Path,
    headers: dict[str, Path],
    cache: dict[Path, tuple[str, ...]],
    stack: set[Path] | None = None,
) -> tuple[str, ...]:
    cached = cache.get(header)
    if cached is not None:
        return cached

    if stack is None:
        stack = set()
    if header in stack:
        return ()
    stack.add(header)

    external: set[str] = set()
    for include in include_directives(header):
        nested = resolve_arc_include(include, header, headers)
        if nested is None:
            external.add(include)
            continue
        external.update(header_external_includes(nested, headers, cache, stack))

    stack.remove(header)
    resolved = tuple(sorted(external))
    cache[header] = resolved
    return resolved


def cxx_candidates(commands: list[dict[str, Any]]) -> list[tuple[int, dict[str, Any], tuple[Path, ...]]]:
    candidates: list[tuple[int, dict[str, Any], tuple[Path, ...]]] = []
    for index, entry in enumerate(commands):
        source = entry_source(entry)
        if source is None or source.suffix.lower() not in CXX_SOURCE_SUFFIXES:
            continue
        candidates.append((index, entry, include_dirs(entry)))

    def rank(candidate: tuple[int, dict[str, Any], tuple[Path, ...]]) -> tuple[int, ...]:
        index, entry, _ = candidate
        source = entry_source(entry)
        if source is None:
            return (2, 1, index)
        args = entry_args(entry)
        root_app = source == (ROOT / "main" / "app_main.cpp").resolve()
        example_source = under(source, ROOT / "examples")
        project_source = under(source, ROOT) and not under(source, ROOT / "esp-idf")
        app_source = source.name == "app_main.cpp"
        arduino = any("arduino-esp32" in arg or "ARDUINO" in arg for arg in args)
        managed = any("managed_components" in arg for arg in args)
        return (
            0 if root_app else 1,
            0 if project_source else 1,
            0 if app_source else 1,
            1 if example_source else 0,
            1 if arduino else 0,
            1 if managed else 0,
            len(args),
            index,
        )

    return sorted(candidates, key=rank)


def feature_rich_rank(candidate: tuple[int, dict[str, Any], tuple[Path, ...]]) -> tuple[int, int]:
    _, entry, dirs = candidate
    source = entry_source(entry)
    args = entry_args(entry)
    arduino = (source is not None and under(source, ROOT / "arduino-esp32")) or any(
        "arduino-esp32" in arg or "ARDUINO" in arg for arg in args
    )
    return (1 if arduino else 0, -len(dirs))


def project_description_paths(databases: list[Path]) -> list[Path]:
    descriptions: list[Path] = []
    seen: set[Path] = set()

    for database in databases:
        description = database.parent / "project_description.json"
        if description.is_file() and description not in seen:
            descriptions.append(description)
            seen.add(description)

    return descriptions


def load_project_descriptions(databases: list[Path]) -> list[dict[str, Any]]:
    descriptions: list[dict[str, Any]] = []
    for description in project_description_paths(databases):
        try:
            data = json.loads(description.read_text(encoding="utf-8"))
        except json.JSONDecodeError:
            continue
        if isinstance(data, dict):
            descriptions.append(data)
    return descriptions


def component_info(databases: list[Path]) -> dict[str, dict[str, Any]]:
    components: dict[str, dict[str, Any]] = {}

    for data in load_project_descriptions(databases):
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
                "root": Path(root).resolve(),
                "include_dirs": tuple(include_dirs),
                "reqs": tuple(reqs),
            }

    discover_idf_components(components, databases)
    return components


def add_unique_path(paths: list[Path], seen: set[Path], path: Path) -> None:
    resolved = path.expanduser().resolve()
    if resolved not in seen:
        paths.append(resolved)
        seen.add(resolved)


def idf_paths(databases: list[Path], components: dict[str, dict[str, Any]]) -> list[Path]:
    paths: list[Path] = []
    seen: set[Path] = set()

    for name in ("IDF_PATH", "ARC_IDF_PATH"):
        value = os.environ.get(name)
        if value:
            add_unique_path(paths, seen, Path(value))

    for data in load_project_descriptions(databases):
        value = data.get("idf_path")
        if isinstance(value, str) and value:
            add_unique_path(paths, seen, Path(value))

    for info in components.values():
        root = info.get("root")
        if not isinstance(root, Path):
            continue
        for parent in root.parents:
            if parent.name == "components" and (parent.parent / "tools" / "idf.py").is_file():
                add_unique_path(paths, seen, parent.parent)
                break

    return paths


def cmake_register_body(text: str) -> str | None:
    marker = "idf_component_register("
    start = text.find(marker)
    if start < 0:
        return None

    index = start + len(marker)
    depth = 1
    while index < len(text):
        char = text[index]
        if char == "(":
            depth += 1
        elif char == ")":
            depth -= 1
            if depth == 0:
                return text[start + len(marker) : index]
        index += 1
    return None


def cmake_tokens(text: str) -> list[str]:
    body = cmake_register_body(text)
    if body is None:
        return []

    lines = []
    for line in body.splitlines():
        lines.append(line.split("#", 1)[0])
    return shlex.split("\n".join(lines), posix=True)


def cmake_register_sections(root: Path) -> dict[str, list[str]]:
    cmake = root / "CMakeLists.txt"
    if not cmake.is_file():
        return {}

    try:
        tokens = cmake_tokens(cmake.read_text(encoding="utf-8"))
    except ValueError:
        return {}

    sections: dict[str, list[str]] = {}
    current: str | None = None
    for token in tokens:
        key = token.upper()
        if key in CMAKE_COMPONENT_KEYS:
            current = key
            sections.setdefault(current, [])
        elif current is not None:
            sections[current].append(token)
    return sections


def cmake_path(root: Path, token: str) -> Path | None:
    if not token or "$<" in token:
        return None

    token = token.replace("${CMAKE_CURRENT_SOURCE_DIR}", str(root))
    if "$" in token:
        return None

    path = Path(token)
    if not path.is_absolute():
        path = root / path
    return path.resolve()


def cmake_name(token: str) -> str | None:
    if not token or "$" in token or ";" in token:
        return None
    return token


def discover_idf_components(components: dict[str, dict[str, Any]], databases: list[Path]) -> None:
    for idf in idf_paths(databases, components):
        component_root = idf / "components"
        if not component_root.is_dir():
            continue

        for root in sorted(path for path in component_root.iterdir() if path.is_dir()):
            if root.name in components or not (root / "CMakeLists.txt").is_file():
                continue

            sections = cmake_register_sections(root)
            include_dirs = [path for token in sections.get("INCLUDE_DIRS", ()) if (path := cmake_path(root, token))]
            reqs = [name for token in sections.get("REQUIRES", ()) if (name := cmake_name(token))]
            components[root.name] = {
                "root": root.resolve(),
                "include_dirs": tuple(dict.fromkeys(include_dirs)),
                "reqs": tuple(dict.fromkeys(reqs)),
            }


def include_root_for_match(path: Path, include: str) -> Path:
    root = path.parent
    for _ in range(len(Path(include).parts) - 1):
        root = root.parent
    return root


def component_discovered_dirs(info: dict[str, Any]) -> tuple[Path, ...]:
    cached = info.get("discovered_include_dirs")
    if isinstance(cached, tuple):
        return cached

    root = info.get("root")
    if not isinstance(root, Path) or not root.is_dir():
        info["discovered_include_dirs"] = ()
        return ()

    dirs: list[Path] = []
    for include_dir in root.rglob("include"):
        if not include_dir.is_dir():
            continue
        if not esp32s3_compatible_path(include_dir):
            continue
        dirs.append(include_dir.resolve())
        for child in sorted(include_dir.iterdir()):
            if child.is_dir():
                if not esp32s3_compatible_path(child):
                    continue
                dirs.append(child.resolve())

    resolved = tuple(dict.fromkeys(dirs))
    info["discovered_include_dirs"] = resolved
    return resolved


def include_owner(
    include: str,
    components: dict[str, dict[str, Any]],
    search_cache: dict[str, str | None],
    match_cache: dict[str, tuple[tuple[int, int, str, Path], ...]],
) -> str | None:
    cached = search_cache.get(include)
    if include in search_cache:
        return cached

    for name, info in components.items():
        dirs = info.get("include_dirs")
        if not isinstance(dirs, tuple):
            continue
        if any((directory / include).is_file() for directory in dirs):
            search_cache[include] = name
            return name

    parts = Path(include).parts
    basename = parts[-1]
    matches: list[tuple[int, int, str, Path]] = []
    for rank, depth, name, candidate in component_header_matches(basename, components, match_cache):
        if tuple(candidate.parts[-len(parts) :]) != parts:
            continue
        matches.append((rank, depth, name, include_root_for_match(candidate, include)))

    if not matches:
        search_cache[include] = None
        return None

    _, _, owner, include_root = min(matches)
    info = components[owner]
    dirs = tuple(dict.fromkeys((*info.get("include_dirs", ()), include_root)))
    info["include_dirs"] = dirs
    search_cache[include] = owner
    return owner


def component_header_matches(
    basename: str,
    components: dict[str, dict[str, Any]],
    cache: dict[str, tuple[tuple[int, int, str, Path], ...]],
) -> tuple[tuple[int, int, str, Path], ...]:
    cached = cache.get(basename)
    if cached is not None:
        return cached

    matches: list[tuple[int, int, str, Path]] = []
    for name, info in components.items():
        root = info.get("root")
        if not isinstance(root, Path) or not root.is_dir():
            continue

        for candidate in root.rglob(basename):
            if not candidate.is_file() or not esp32s3_compatible_path(candidate):
                continue
            matches.append((0 if "esp32s3" in candidate.parts else 1, len(candidate.parts), name, candidate.resolve()))

    resolved = tuple(sorted(matches))
    cache[basename] = resolved
    return resolved


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

    dirs = list(dict.fromkeys(info.get("include_dirs", ())))
    if not dirs:
        dirs.extend(component_discovered_dirs(info))
    for req in info.get("reqs", ()):
        dirs.extend(component_public_dirs(req, components, seen))
    return dirs


def validation_support_dirs(
    databases: list[Path],
    components: dict[str, dict[str, Any]],
) -> tuple[Path, ...]:
    includes = (
        "mbedtls/private_access.h",
        "mbedtls/private/bignum.h",
        "mbedtls/private/config_psa.h",
        "tf-psa-crypto/build_info.h",
        "nimble/hci_common.h",
        "nimble/nimble_npl_os.h",
        "esp_nimble_cfg.h",
        "ulp_common.h",
        "ulp_fsm_common.h",
    )
    dirs: list[Path] = []
    seen: set[Path] = set()
    for idf in idf_paths(databases, components):
        root = idf / "components"
        if not root.is_dir():
            continue
        for include in includes:
            parts = Path(include).parts
            matches = []
            for candidate in root.rglob(parts[-1]):
                if tuple(candidate.parts[-len(parts) :]) != parts:
                    continue
                matches.append(candidate)
            matches.sort(key=lambda path: (0 if "freertos" in path.parts else 1, len(path.parts)))
            for candidate in matches:
                include_root = include_root_for_match(candidate, include)
                if not esp32s3_compatible_path(include_root):
                    continue
                if include_root not in seen:
                    dirs.append(include_root)
                    seen.add(include_root)
                break
    bt = components.get("bt")
    if bt is not None:
        for directory in component_discovered_dirs(bt):
            if directory not in seen and esp32s3_compatible_path(directory):
                dirs.append(directory)
                seen.add(directory)
    return tuple(dirs)


def missing_include_dirs(
    includes: tuple[str, ...],
    dirs: tuple[Path, ...],
    components: dict[str, dict[str, Any]],
    cache: dict[tuple[Path, str], bool],
    owner_cache: dict[str, str | None],
    match_cache: dict[str, tuple[tuple[int, int, str, Path], ...]],
) -> tuple[Path, ...] | None:
    available = set(dirs)
    extras: list[Path] = []

    for include in includes:
        search_dirs = dirs + tuple(extras)
        if has_include(include, search_dirs, cache):
            continue

        owner = include_owner(include, components, owner_cache, match_cache)
        if owner is None:
            continue

        for directory in component_public_dirs(owner, components):
            if directory not in available:
                extras.append(directory)
                available.add(directory)

        if not has_include(include, dirs + tuple(extras), cache):
            return None

    return tuple(extras)


def changed_arc_headers(base: str) -> set[Path]:
    try:
        proc = subprocess.run(
            [
                "git",
                "diff",
                "--name-only",
                "--diff-filter=ACMRTUXB",
                base,
                "--",
                "components/arc/include",
            ],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.DEVNULL,
            text=True,
            check=False,
        )
    except (FileNotFoundError, PermissionError):
        return set()

    if proc.returncode != 0:
        return set()

    headers: set[Path] = set()
    for line in proc.stdout.splitlines():
        path = (ROOT / line).resolve()
        if path.suffix.lower() in HEADER_SUFFIXES and under(path, ARC_INCLUDE_DIR):
            headers.add(path)
    return headers


def auto_validate_jobs(requested: int | None) -> int:
    if requested is not None and requested > 0:
        return requested
    cpus = os.cpu_count() or 2
    return max(1, min(8, cpus // 2 if cpus > 2 else cpus))


def auto_validate_batch_size(requested: int | None, headers: int) -> int:
    if requested is not None and requested > 0:
        return requested
    if headers <= 8:
        return 1
    return 8


def entry_args_for_file(
    entry: dict[str, Any],
    file: Path,
    extra_include_dirs: tuple[Path, ...] = (),
) -> list[str]:
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

    if args and extra_include_dirs:
        args = [args[0], *(f"-I{path}" for path in extra_include_dirs), *args[1:]]

    return args


def command_for_file(
    entry: dict[str, Any],
    file: Path,
    extra_include_dirs: tuple[Path, ...] = (),
) -> dict[str, Any]:
    clone = dict(entry)
    clone["file"] = str(file)

    args = entry_args_for_file(entry, file, extra_include_dirs)
    if not args:
        return clone

    if "arguments" in clone:
        clone["arguments"] = args
        clone.pop("command", None)
    else:
        clone["command"] = shlex.join(args)

    return clone


def append_include_dirs(entry: dict[str, Any], include_dirs: tuple[Path, ...]) -> dict[str, Any]:
    if not include_dirs:
        return entry

    args = entry_args(entry)
    if not args:
        return entry

    clone = dict(entry)
    args = [*args, *(f"-I{path}" for path in include_dirs)]
    if "arguments" in clone:
        clone["arguments"] = args
        clone.pop("command", None)
    else:
        clone["command"] = shlex.join(args)
    return clone


def strip_ansi(text: str) -> str:
    return ANSI_RE.sub("", text)


def summarize_stderr(stderr: str, status: int) -> str:
    lines = [strip_ansi(line).strip() for line in stderr.splitlines()]
    for marker in ("fatal error:", "error:"):
        for line in lines:
            if marker in line:
                return line
    for line in lines:
        if line:
            return line
    return f"compiler exited with status {status}"


def validate_header_command(
    entry: dict[str, Any],
    header: Path,
    extra_include_dirs: tuple[Path, ...] = (),
    timeout: float = 15.0,
) -> str | None:
    with tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False) as temp:
        include = str(header).replace("\\", "\\\\").replace('"', '\\"')
        temp.write(
            "#include <assert.h>\n"
            "#ifndef assert\n"
            "#define assert(condition) ((void)0)\n"
            "#endif\n"
            f'#include "{include}"\n'
            "int main() { return 0; }\n"
        )
        source = Path(temp.name)

    try:
        args = entry_args_for_file(entry, source, extra_include_dirs)
        if not args:
            return "candidate compile command has no arguments"

        filtered: list[str] = []
        skip_next = False
        for arg in args:
            if skip_next:
                skip_next = False
                continue
            if arg == "-o":
                skip_next = True
                continue
            if arg == "-Werror" or arg.startswith("-Werror="):
                continue
            filtered.append(arg)
        filtered.insert(1, "-fsyntax-only")
        filtered.append("-Wno-error")

        try:
            proc = subprocess.run(
                filtered,
                cwd=entry_directory(entry),
                stdout=subprocess.DEVNULL,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
                timeout=timeout,
            )
        except FileNotFoundError as err:
            return str(err)
        except subprocess.TimeoutExpired:
            return f"compiler timed out after {timeout:g}s"

        if proc.returncode == 0:
            return None
        return summarize_stderr(proc.stderr, proc.returncode)
    finally:
        source.unlink(missing_ok=True)


def validate_header_set(
    entry: dict[str, Any],
    headers: list[Path],
    extra_include_dirs: tuple[Path, ...] = (),
    timeout: float = 60.0,
) -> str | None:
    with tempfile.NamedTemporaryFile("w", suffix=".cpp", delete=False) as temp:
        temp.write("#include <assert.h>\n#ifndef assert\n#define assert(condition) ((void)0)\n#endif\n")
        for header in headers:
            include = str(header).replace("\\", "\\\\").replace('"', '\\"')
            temp.write(f'#include "{include}"\n')
        temp.write("int main() { return 0; }\n")
        source = Path(temp.name)

    try:
        return validate_source_command(entry, source, extra_include_dirs, timeout)
    finally:
        source.unlink(missing_ok=True)


def validate_source_command(
    entry: dict[str, Any],
    source: Path,
    extra_include_dirs: tuple[Path, ...] = (),
    timeout: float = 15.0,
) -> str | None:
    args = entry_args_for_file(entry, source, extra_include_dirs)
    if not args:
        return "candidate compile command has no arguments"

    filtered: list[str] = []
    skip_next = False
    for arg in args:
        if skip_next:
            skip_next = False
            continue
        if arg == "-o":
            skip_next = True
            continue
        if arg == "-Werror" or arg.startswith("-Werror="):
            continue
        filtered.append(arg)
    filtered.insert(1, "-fsyntax-only")
    filtered.append("-Wno-error")

    try:
        proc = subprocess.run(
            filtered,
            cwd=entry_directory(entry),
            stdout=subprocess.DEVNULL,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
            timeout=timeout,
        )
    except FileNotFoundError as err:
        return str(err)
    except subprocess.TimeoutExpired:
        return f"compiler timed out after {timeout:g}s"

    if proc.returncode == 0:
        return None
    return summarize_stderr(proc.stderr, proc.returncode)


def validation_key(entry: dict[str, Any]) -> tuple[Path, tuple[str, ...]]:
    return (entry_directory(entry), tuple(entry_args_for_file(entry, Path("__arc_header_validation__.cpp"))))


def validate_header_groups(
    items: list[tuple[Path, dict[str, Any]]],
    support_dirs: tuple[Path, ...],
    timeout: float,
    jobs: int | None,
    batch_size: int,
) -> list[str]:
    groups: dict[tuple[Path, tuple[str, ...]], tuple[dict[str, Any], list[Path]]] = {}
    for header, entry in items:
        validated_entry = append_include_dirs(entry, support_dirs)
        key = validation_key(validated_entry)
        group = groups.get(key)
        if group is None:
            groups[key] = (validated_entry, [header])
        else:
            group[1].append(header)

    failures: list[str] = []
    batches: list[tuple[dict[str, Any], list[Path]]] = []
    batch_size = max(1, batch_size)
    for entry, headers in groups.values():
        for index in range(0, len(headers), batch_size):
            batches.append((entry, headers[index : index + batch_size]))

    workers = min(auto_validate_jobs(jobs), max(1, len(batches)))
    with concurrent.futures.ThreadPoolExecutor(max_workers=workers) as executor:
        futures = {
            executor.submit(validate_header_set, entry, headers, (), timeout): (entry, headers)
            for entry, headers in batches
        }
        for future in concurrent.futures.as_completed(futures):
            entry, headers = futures[future]
            error = future.result()
            if error is None:
                continue
            if len(headers) == 1:
                failures.append(f"{display(headers[0])}: {error}")
                continue

            for header in headers:
                header_error = validate_header_command(entry, header, (), timeout)
                if header_error is not None:
                    failures.append(f"{display(header)}: {header_error}")

    failures.sort()
    return failures


def arc_header_commands(
    commands: list[dict[str, Any]],
    databases: list[Path],
    validate: bool = False,
    validate_timeout: float = 15.0,
    validate_jobs: int | None = None,
    validate_batch_size: int | None = None,
    only_headers: set[Path] | None = None,
) -> tuple[list[dict[str, Any]], list[str]]:
    headers = arc_header_index()
    if not headers:
        return [], []

    candidates = cxx_candidates(commands)
    components = component_info(databases)
    include_cache: dict[tuple[Path, str], bool] = {}
    owner_cache: dict[str, str | None] = {}
    match_cache: dict[str, tuple[tuple[int, int, str, Path], ...]] = {}
    dependency_cache: dict[Path, tuple[str, ...]] = {}
    header_commands: list[dict[str, Any]] = []
    selected_headers: list[Path] = []
    failures: list[str] = []
    rich_candidates = sorted(candidates, key=feature_rich_rank)

    for header in sorted(headers.values()):
        if only_headers is not None and header not in only_headers:
            continue
        includes = header_external_includes(header, headers, dependency_cache)
        selected: dict[str, Any] | None = None
        error: str | None = None
        attempted: set[tuple[str, tuple[str, ...]]] = set()
        for _, entry, dirs in rich_candidates:
            extras = ()
            if not all(has_include(include, dirs, include_cache) for include in includes):
                extras = missing_include_dirs(includes, dirs, components, include_cache, owner_cache, match_cache)
                if extras is None:
                    continue

            key = (
                source_key(entry, DEFAULT_OUTPUT),
                tuple(str(path) for path in extras),
            )
            if key in attempted:
                continue
            attempted.add(key)

            arc_include = ARC_INCLUDE_DIR.resolve()
            if arc_include not in dirs and arc_include not in extras:
                extras = (*extras, arc_include)

            selected = command_for_file(entry, header, extras)
            break

        if selected is None and not validate and candidates:
            selected = command_for_file(candidates[0][1], header)

        if selected is None:
            reason = error or "no candidate compile command satisfied transitive header dependencies"
            failures.append(f"{display(header)}: {reason}")
            continue

        header_commands.append(selected)
        selected_headers.append(header)

    if validate and header_commands:
        support_dirs = validation_support_dirs(databases, components)
        failures.extend(
            validate_header_groups(
                list(zip(selected_headers, header_commands, strict=True)),
                support_dirs,
                validate_timeout,
                validate_jobs,
                auto_validate_batch_size(validate_batch_size, len(selected_headers)),
            )
        )

    return header_commands, failures


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Merge ESP-IDF compile databases for clangd.",
    )
    parser.add_argument(
        "inputs",
        nargs="*",
        type=Path,
        help="Project directories or compile_commands.json files. Defaults to root and nested example builds.",
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
    parser.add_argument(
        "--validate-arc-headers",
        action="store_true",
        help="Syntax-check inferred Arc header commands and fail if any public Arc header cannot be compiled.",
    )
    parser.add_argument(
        "--validate-timeout",
        type=float,
        default=float(os.environ.get("ARC_CLANGD_VALIDATE_TIMEOUT", "120")),
        help="Per-header compiler timeout in seconds when --validate-arc-headers is used.",
    )
    parser.add_argument(
        "--validate-jobs",
        type=int,
        default=int(os.environ.get("ARC_CLANGD_VALIDATE_JOBS", "0")),
        help="Parallel compiler jobs when --validate-arc-headers is used. Defaults to a conservative auto value.",
    )
    parser.add_argument(
        "--validate-batch-size",
        type=int,
        default=int(os.environ.get("ARC_CLANGD_VALIDATE_BATCH_SIZE", "0")),
        help=(
            "Headers per validation compiler invocation. Defaults to auto: single-header batches for small changed sets, "
            "bounded batches for broad validation."
        ),
    )
    parser.add_argument(
        "--changed-arc-headers",
        nargs="?",
        const="HEAD~1",
        metavar="BASE",
        help=(
            "Only infer/validate public Arc headers changed since BASE. "
            "When BASE is omitted, HEAD~1 is used. Falls back to all headers if no changed headers are found."
        ),
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
    commands = add_builtin_includes(commands)

    header_commands: list[dict[str, Any]] = []
    header_failures: list[str] = []
    if not args.no_arc_headers:
        only_headers = changed_arc_headers(args.changed_arc_headers) if args.changed_arc_headers else None
        header_commands, header_failures = arc_header_commands(
            commands,
            databases,
            validate=args.validate_arc_headers,
            validate_timeout=args.validate_timeout,
            validate_jobs=args.validate_jobs if args.validate_jobs > 0 else None,
            validate_batch_size=args.validate_batch_size if args.validate_batch_size > 0 else None,
            only_headers=only_headers if only_headers else None,
        )
        commands.extend(header_commands)

    output = args.output.expanduser()
    if not output.is_absolute():
        output = ROOT / output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(commands, indent=2) + "\n", encoding="utf-8")

    header_label = "validated" if args.validate_arc_headers else "inferred"
    print(
        f"merged {len(commands)} commands from {len(databases)} databases into {display(output)}"
        f" ({len(header_commands)} {header_label} Arc header commands)",
    )
    if header_failures:
        for failure in header_failures:
            print(f"arc header command failed: {failure}", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
