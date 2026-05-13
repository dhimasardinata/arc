from __future__ import annotations

import argparse
import json
import os
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKIP_PROJECT_PARTS = {"build", "managed_components"}


@dataclass(frozen=True)
class ArcProject:
    path: Path
    rel: str
    kind: str
    target: str
    experimental: bool


def _rel(path: Path, root: Path = ROOT) -> str:
    try:
        value = path.resolve().relative_to(root.resolve())
    except ValueError:
        return path.as_posix()
    return "." if value == Path(".") else value.as_posix()


def _skip_project(path: Path, root: Path = ROOT) -> bool:
    try:
        parts = path.resolve().relative_to(root.resolve()).parts
    except ValueError:
        parts = path.parts
    return any(_skip_part(part) for part in parts)


def _skip_part(part: str) -> bool:
    return part in SKIP_PROJECT_PARTS or part.startswith("build-")


def classify(path: Path, root: Path = ROOT) -> ArcProject:
    rel = _rel(path, root)
    if rel == ".":
        return ArcProject(path.resolve(), rel, "root", "esp32s3", False)
    if rel.startswith("examples/esp32s31/"):
        return ArcProject(path.resolve(), rel, "example", "esp32s31", True)
    if rel.startswith("examples/esp32s3/"):
        return ArcProject(path.resolve(), rel, "example", "esp32s3", False)
    if rel.startswith("examples/portable/"):
        return ArcProject(path.resolve(), rel, "example", "portable", False)
    return ArcProject(path.resolve(), rel, "example", "unknown", False)


def example_projects(root: Path = ROOT) -> list[ArcProject]:
    examples = root / "examples"
    if not examples.is_dir():
        return []

    projects: list[ArcProject] = []
    for current, dirs, files in os.walk(examples):
        dirs[:] = sorted(dirname for dirname in dirs if not _skip_part(dirname))
        if "CMakeLists.txt" not in files:
            continue
        project = Path(current)
        if _skip_project(project, root):
            continue
        if project.name == "main":
            continue
        if not (project / "main" / "CMakeLists.txt").is_file():
            continue
        projects.append(classify(project, root))
    return projects


def projects(root: Path = ROOT, include_root: bool = True) -> list[ArcProject]:
    found = [classify(root, root)] if include_root else []
    found.extend(example_projects(root))
    return found


def selected_projects(args: argparse.Namespace) -> list[ArcProject]:
    selected = projects(include_root=not args.examples)
    if args.buildable:
        selected = [project for project in selected if not project.experimental]
    if args.experimental:
        selected = [project for project in selected if project.experimental]
    if args.target:
        selected = [project for project in selected if project.target == args.target]
    return selected


def main() -> int:
    parser = argparse.ArgumentParser(description="List Arc firmware projects.")
    parser.add_argument("--examples", action="store_true", help="List examples only, omitting the root firmware.")
    parser.add_argument("--buildable", action="store_true", help="Omit experimental projects skipped by default CI.")
    parser.add_argument("--experimental", action="store_true", help="List only experimental projects.")
    parser.add_argument("--target", choices=("esp32s3", "esp32s31", "portable", "unknown"), help="Filter by target.")
    parser.add_argument("--json", action="store_true", help="Emit project metadata as JSON.")
    parser.add_argument("-0", "--null", action="store_true", help="Separate path output with NUL bytes.")
    args = parser.parse_args()

    found = selected_projects(args)
    if args.json:
        print(
            json.dumps(
                [
                    {
                        "path": project.rel,
                        "kind": project.kind,
                        "target": project.target,
                        "experimental": project.experimental,
                    }
                    for project in found
                ],
                indent=2,
            )
        )
        return 0

    sep = "\0" if args.null else "\n"
    output = sep.join(project.rel for project in found)
    if output:
        print(output, end=sep)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
