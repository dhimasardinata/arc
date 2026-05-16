#!/usr/bin/env python3

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
sys.dont_write_bytecode = True

from arc_projects import ArcProject, projects  # noqa: E402

SHARED_PREFIXES = (
    "cmake/",
    "components/",
)
SHARED_FILES = {
    ".github/workflows/build.yml",
    "tools/arc-projects.py",
    "tools/ci-build-plan.py",
}
ROOT_PREFIXES = ("main/",)
ROOT_FILES = {
    "CMakeLists.txt",
    "env.fish",
    "env.sh",
    "partitions_16mb.csv",
    "sdkconfig.defaults",
    "sdkconfig.defaults.release",
}
NON_FIRMWARE_PREFIXES = (
    ".github/",
    ".vscode/",
    ".zed/",
    "AGENTS.md",
    "README.md",
    "docs/",
    "tests/",
    "tools/",
)


def changed_since(base: str, root: Path) -> list[str] | None:
    proc = subprocess.run(
        ["git", "diff", "--name-only", base, "HEAD", "--"],
        cwd=root,
        check=False,
        stdout=subprocess.PIPE,
        stderr=subprocess.DEVNULL,
        text=True,
    )
    if proc.returncode != 0:
        return None
    return [line for line in proc.stdout.splitlines() if line]


def all_buildable(root: Path) -> list[ArcProject]:
    return [project for project in projects(root=root) if not project.experimental]


def project_for(path: str, buildable: list[ArcProject]) -> ArcProject | None:
    matches = [
        project
        for project in buildable
        if project.rel != "." and (path == project.rel or path.startswith(f"{project.rel}/"))
    ]
    if not matches:
        return None
    return max(matches, key=lambda project: len(project.rel))


def is_shared(path: str) -> bool:
    return path in SHARED_FILES or any(path.startswith(prefix) for prefix in SHARED_PREFIXES)


def is_root(path: str) -> bool:
    return path in ROOT_FILES or any(path.startswith(prefix) for prefix in ROOT_PREFIXES)


def is_non_firmware(path: str) -> bool:
    return any(path == prefix.rstrip("/") or path.startswith(prefix) for prefix in NON_FIRMWARE_PREFIXES)


def plan(changed: list[str] | None, buildable: list[ArcProject]) -> list[ArcProject]:
    if changed is None:
        return buildable

    selected: dict[str, ArcProject] = {}
    root_project = next((project for project in buildable if project.rel == "."), None)

    for path in changed:
        if is_shared(path):
            return buildable

        if is_root(path):
            if root_project is not None:
                selected[root_project.rel] = root_project
            continue

        project = project_for(path, buildable)
        if project is not None:
            selected[project.rel] = project
            continue

        if path.startswith("examples/"):
            return buildable

        if is_non_firmware(path):
            continue

        return buildable

    return [selected[key] for key in sorted(selected)]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Select Arc firmware projects affected by a CI change set.")
    parser.add_argument("--root", type=Path, default=ROOT, help="Repository root.")
    parser.add_argument(
        "--base", help="Git base ref to diff against. If omitted or unavailable, all buildable projects are selected."
    )
    parser.add_argument(
        "--changed-file", action="append", default=[], help="Inject a changed path; intended for tests."
    )
    parser.add_argument(
        "--buildable",
        action="store_true",
        help="Accepted for workflow readability; experimental projects are always skipped.",
    )
    args = parser.parse_args(argv)

    root = args.root.resolve()
    buildable = all_buildable(root)
    changed = args.changed_file if args.changed_file else changed_since(args.base, root) if args.base else None

    for project in plan(changed, buildable):
        print(project.rel)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
