#!/usr/bin/env python3

from __future__ import annotations

import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
INCLUDE_ROOT = ROOT / "components" / "arc" / "include"
PROFILE_MANIFEST = ROOT / "components" / "arc" / "profiles.json"
CMAKE_DEPS = ROOT / "cmake" / "arc-deps.cmake"

INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"(arc/[^"]+)"')
FEATURE_RE = re.compile(r'^(?:if|elseif)\(feature STREQUAL "([^"]+)"\)$')
APPEND_RE = re.compile(r"^list\(APPEND result ([^)]+)\)$")
PROFILE_RE = re.compile(r"^[a-z][a-z0-9_]*$")


def die(message: str) -> int:
    print(f"arc profile manifest check failed: {message}", file=sys.stderr)
    return 1


def load_profiles(path: Path = PROFILE_MANIFEST) -> dict[str, dict[str, Any]]:
    data = json.loads(path.read_text(encoding="utf-8"))
    profiles = data.get("profiles")
    if not isinstance(profiles, dict) or not profiles:
        raise ValueError("profiles must be a non-empty object")
    return profiles


def include_deps(rel: str) -> list[str]:
    path = INCLUDE_ROOT / rel
    if not path.is_file():
        raise FileNotFoundError(f"missing profile header: {rel}")
    return [
        match.group(1) for line in path.read_text(encoding="utf-8").splitlines() if (match := INCLUDE_RE.match(line))
    ]


def closure(root_header: str) -> set[str]:
    seen: set[str] = set()

    def visit(rel: str) -> None:
        if rel in seen:
            return
        seen.add(rel)
        for dep in include_deps(rel):
            visit(dep)

    visit(root_header)
    return seen


def cmake_requires(text: str) -> dict[str, tuple[str, ...]]:
    deps: dict[str, list[str]] = {}
    current: str | None = None
    for raw in text.splitlines():
        line = raw.strip()
        if match := FEATURE_RE.match(line):
            current = match.group(1)
            deps.setdefault(current, [])
            continue
        if line.startswith("elseif(") or line.startswith("else(") or line == "endif()":
            current = None
            continue
        if current is not None and (match := APPEND_RE.match(line)):
            deps[current].extend(match.group(1).split())
    return {feature: tuple(values) for feature, values in deps.items()}


def validate_profiles(profiles: dict[str, dict[str, Any]]) -> list[str]:
    problems: list[str] = []
    cmake = cmake_requires(CMAKE_DEPS.read_text(encoding="utf-8"))
    domain_roots = {
        spec.get("header")
        for spec in profiles.values()
        if isinstance(spec, dict) and spec.get("kind") == "domain" and isinstance(spec.get("header"), str)
    }
    for name, spec in sorted(profiles.items()):
        if not isinstance(name, str) or not PROFILE_RE.fullmatch(name):
            problems.append(f"invalid profile name: {name!r}")
            continue
        if not isinstance(spec, dict):
            problems.append(f"profile {name} must be an object")
            continue
        kind = spec.get("kind")
        header = spec.get("header")
        requires = spec.get("requires")
        if kind not in {"substrate", "domain"}:
            problems.append(f"profile {name} has invalid kind")
        if not isinstance(header, str) or not header.startswith("arc/") or not header.endswith(".hpp"):
            problems.append(f"profile {name} has invalid header")
        elif not (INCLUDE_ROOT / header).is_file():
            problems.append(f"profile {name} header does not exist: {header}")
        else:
            try:
                headers = closure(header)
            except FileNotFoundError as exc:
                problems.append(str(exc))
            else:
                if kind == "substrate":
                    leaked = sorted(headers & domain_roots)
                    if leaked:
                        problems.append(
                            f"substrate profile {name} includes domain profile header(s): {', '.join(leaked)}"
                        )
        if not isinstance(requires, list) or any(not isinstance(dep, str) or not dep for dep in requires):
            problems.append(f"profile {name} has invalid requires list")
            continue
        if requires != sorted(set(requires)):
            problems.append(f"profile {name} requires must be sorted and unique")
        cmake_deps = cmake.get(name)
        if cmake_deps is None:
            problems.append(f"profile {name} is missing from cmake/arc-deps.cmake")
        elif tuple(requires) != cmake_deps:
            problems.append(f"profile {name} requires drift: manifest {tuple(requires)!r} != cmake {cmake_deps!r}")
    return problems


def main() -> int:
    try:
        profiles = load_profiles()
        problems = validate_profiles(profiles)
    except (OSError, ValueError, json.JSONDecodeError) as exc:
        return die(str(exc))
    if problems:
        return die("\n".join(problems))
    print(f"arc profile manifest check: OK ({len(profiles)} profiles)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
