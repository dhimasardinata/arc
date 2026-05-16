#!/usr/bin/env python3

from __future__ import annotations

import argparse
import json
import re
import shutil
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INCLUDE_ROOT = ROOT / "components" / "arc" / "include"
PROFILE_MANIFEST = ROOT / "components" / "arc" / "profiles.json"
DEFAULT_OUT = ROOT / "dump" / "profiles"
MARKER = ".arc-profile-export"
MARKER_TEXT = "arc profile export\n"
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"(arc/[^"]+)"')


def load_profile_manifest() -> tuple[dict[str, str], dict[str, tuple[str, ...]]]:
    data = json.loads(PROFILE_MANIFEST.read_text(encoding="utf-8"))
    raw_profiles = data.get("profiles")
    if not isinstance(raw_profiles, dict) or not raw_profiles:
        raise ValueError("profile manifest must define at least one profile")

    profiles: dict[str, str] = {}
    requires: dict[str, tuple[str, ...]] = {}
    for name, spec in sorted(raw_profiles.items()):
        if not isinstance(name, str) or not name:
            raise ValueError("profile names must be non-empty strings")
        if not isinstance(spec, dict):
            raise ValueError(f"profile {name} must be an object")
        header = spec.get("header")
        deps = spec.get("requires", [])
        if not isinstance(header, str) or not header.startswith("arc/") or not header.endswith(".hpp"):
            raise ValueError(f"profile {name} has invalid header")
        if not isinstance(deps, list) or any(not isinstance(dep, str) or not dep for dep in deps):
            raise ValueError(f"profile {name} has invalid requires list")
        profiles[name] = header
        requires[name] = tuple(deps)
    return profiles, requires


PROFILES, PROFILE_REQUIRES = load_profile_manifest()


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export one Arc profile header closure")
    parser.add_argument("profile", choices=sorted(PROFILES), help="profile header to export")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="output directory; defaults to dump/profiles/<profile>",
    )
    return parser.parse_args(argv)


def safe_output(path: Path) -> Path:
    out = path.expanduser().resolve()
    if out == ROOT:
        raise ValueError("profile export output must not be the repository root")
    if out.parent == out:
        raise ValueError("profile export output must not be the filesystem root")
    return out


def display_out(out: Path) -> str:
    try:
        return str(out.relative_to(ROOT))
    except ValueError:
        return str(out)


def is_empty(path: Path) -> bool:
    return path.is_dir() and next(path.iterdir(), None) is None


def is_marked_export(out: Path) -> bool:
    marker = out / MARKER
    if not marker.is_file():
        return False
    try:
        return marker.read_text(encoding="utf-8") == MARKER_TEXT
    except OSError:
        return False


def is_legacy_export(profile: str, out: Path) -> bool:
    manifest = out / "PROFILE.txt"
    cmake = out / "CMakeLists.txt"
    root = out / "include" / PROFILES[profile]
    if not manifest.is_file() or not cmake.is_file() or not root.is_file():
        return False
    try:
        first = manifest.read_text(encoding="utf-8").splitlines()[0]
    except (IndexError, OSError):
        return False
    return first == f"profile\t{profile}"


def prepare_output(profile: str, out: Path) -> None:
    if out.exists() and not out.is_dir():
        raise ValueError("profile export output must be a directory")

    if out.is_dir() and not is_empty(out):
        if is_marked_export(out) or is_legacy_export(profile, out):
            shutil.rmtree(out)
        else:
            raise ValueError("profile export output must be empty or a previous Arc profile export")

    out.mkdir(parents=True, exist_ok=True)
    (out / MARKER).write_text(MARKER_TEXT, encoding="utf-8")


def include_path(rel: str) -> Path:
    path = INCLUDE_ROOT / rel
    if not path.is_file():
        raise FileNotFoundError(f"missing Arc include: {rel}")
    return path


def include_deps(rel: str) -> list[str]:
    path = include_path(rel)
    deps: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        match = INCLUDE_RE.match(line)
        if match:
            deps.append(match.group(1))
    return deps


def closure(root_header: str) -> list[str]:
    seen: set[str] = set()
    ordered: list[str] = []

    def visit(rel: str) -> None:
        if rel in seen:
            return
        seen.add(rel)
        for dep in include_deps(rel):
            visit(dep)
        ordered.append(rel)

    visit(root_header)
    return sorted(ordered)


def copy_headers(headers: list[str], out: Path) -> None:
    include_out = out / "include"
    for rel in headers:
        src = include_path(rel)
        dst = include_out / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def write_cmake(profile: str, out: Path) -> None:
    requires = PROFILE_REQUIRES[profile]
    if not requires:
        text = 'idf_component_register(INCLUDE_DIRS "include")\n'
    else:
        deps = "\n".join(f"        {dep}" for dep in requires)
        text = f'idf_component_register(\n    INCLUDE_DIRS "include"\n    REQUIRES\n{deps}\n)\n'
    (out / "CMakeLists.txt").write_text(text, encoding="utf-8")


def write_manifest(profile: str, headers: list[str], out: Path) -> None:
    manifest = "\n".join(headers)
    (out / "PROFILE.txt").write_text(
        f"profile\t{profile}\nheaders\t{len(headers)}\n\n{manifest}\n",
        encoding="utf-8",
    )


def write_readme(profile: str, out: Path) -> None:
    header = PROFILES[profile]
    requires = PROFILE_REQUIRES[profile]
    lines = [
        f"# Arc {profile} Profile",
        "",
        f"This is a header-only export rooted at `{header}`.",
        "Copy it as an ESP-IDF component and include the profile header directly.",
        "",
        "```cpp",
        f'#include "{header}"',
        "```",
        "",
    ]
    if requires:
        lines.extend(
            [
                "The generated component declares these ESP-IDF requirements:",
                "",
                ", ".join(f"`{dep}`" for dep in requires),
                "",
            ]
        )
    (out / "README.md").write_text(
        "\n".join(lines),
        encoding="utf-8",
    )


def export_profile(profile: str, out: Path) -> int:
    headers = closure(PROFILES[profile])
    prepare_output(profile, out)
    copy_headers(headers, out)
    write_cmake(profile, out)
    write_manifest(profile, headers, out)
    write_readme(profile, out)
    print(f"exported {profile} profile with {len(headers)} headers into {display_out(out)}")
    return 0


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    out = safe_output(args.output if args.output else DEFAULT_OUT / args.profile)
    return export_profile(args.profile, out)


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except (OSError, ValueError) as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(2)
