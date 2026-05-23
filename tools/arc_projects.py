from __future__ import annotations

import argparse
import json
import os
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SKIP_PROJECT_PARTS = {"build", "managed_components"}
OUTPUT_FORMATS = ("text", "report", "json")


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


def selected_projects(args: argparse.Namespace, root: Path = ROOT) -> list[ArcProject]:
    selected = projects(root=root, include_root=not args.examples)
    if args.buildable:
        selected = [project for project in selected if not project.experimental]
    if args.experimental:
        selected = [project for project in selected if project.experimental]
    if args.target:
        selected = [project for project in selected if project.target == args.target]
    return selected


def project_json(project: ArcProject) -> dict[str, object]:
    return {
        "path": project.rel,
        "kind": project.kind,
        "target": project.target,
        "experimental": project.experimental,
    }


def build_command(project: ArcProject) -> str:
    if project.rel == ".":
        return "idf.py build"
    return f"idf.py -C {project.rel} build"


def report(projects: list[ArcProject]) -> dict[str, object]:
    targets = sorted({project.target for project in projects})
    return {
        "projects": [
            {
                **project_json(project),
                "build_command": build_command(project),
            }
            for project in projects
        ],
        "summary": {
            "projects": len(projects),
            "root": sum(1 for project in projects if project.kind == "root"),
            "examples": sum(1 for project in projects if project.kind == "example"),
            "experimental": sum(1 for project in projects if project.experimental),
            "targets": {target: sum(1 for project in projects if project.target == target) for target in targets},
        },
    }


def print_report(projects: list[ArcProject]) -> None:
    payload = report(projects)
    summary = payload["summary"]
    print("arc projects report")
    print(f"- projects: {summary['projects']}")
    print(f"- root projects: {summary['root']}")
    print(f"- examples: {summary['examples']}")
    print(f"- experimental: {summary['experimental']}")
    print("- targets:")
    for target, count in summary["targets"].items():
        print(f"  - {target}: {count}")
    print("- projects:")
    for project in payload["projects"]:
        marker = " experimental" if project["experimental"] else ""
        print(f"  - {project['path']} ({project['kind']}, {project['target']}{marker})")
        print(f"    build: {project['build_command']}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="List Arc firmware projects.")
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="Repository root to scan.",
    )
    parser.add_argument("--examples", action="store_true", help="List examples only, omitting the root firmware.")
    parser.add_argument("--buildable", action="store_true", help="Omit experimental projects skipped by default CI.")
    parser.add_argument("--experimental", action="store_true", help="List only experimental projects.")
    parser.add_argument("--target", choices=("esp32s3", "esp32s31", "portable", "unknown"), help="Filter by target.")
    parser.add_argument(
        "--format",
        choices=OUTPUT_FORMATS,
        default="text",
        help="output style: path list, human project report, or JSON project report",
    )
    parser.add_argument("--json", action="store_true", help="Emit legacy project metadata JSON list.")
    parser.add_argument("-0", "--null", action="store_true", help="Separate path output with NUL bytes.")
    args = parser.parse_args(argv)

    found = selected_projects(args, root=args.root)
    if args.json:
        print(json.dumps([project_json(project) for project in found], indent=2))
        return 0
    if args.format == "json":
        print(json.dumps(report(found), indent=2, sort_keys=True))
        return 0
    if args.format == "report":
        print_report(found)
        return 0

    sep = "\0" if args.null else "\n"
    output = sep.join(project.rel for project in found)
    if output:
        print(output, end=sep)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
