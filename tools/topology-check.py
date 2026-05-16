#!/usr/bin/env python3
from __future__ import annotations

import argparse
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PINS_RE = re.compile(r"\barc::Pins\s*<(?P<body>[^>]*)>", re.MULTILINE | re.DOTALL)
INT_RE = re.compile(r"^[+-]?\d+$")


@dataclass(frozen=True)
class Pack:
    path: Path
    line: int
    pins: tuple[int, ...]
    unresolved: tuple[str, ...]


def git_files(paths: list[Path]) -> list[Path]:
    if paths:
        return paths
    result = subprocess.run(
        ["git", "ls-files", "README.md", "docs", "examples", "main", "components"],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        text=True,
    )
    return [ROOT / line for line in result.stdout.splitlines() if line]


def display(path: Path) -> str:
    try:
        return str(path.resolve().relative_to(ROOT))
    except ValueError:
        return str(path)


def line_of(text: str, offset: int) -> int:
    return text.count("\n", 0, offset) + 1


def parse_pack(path: Path, text: str) -> list[Pack]:
    packs: list[Pack] = []
    for match in PINS_RE.finditer(text):
        pins: list[int] = []
        unresolved: list[str] = []
        tokens = [raw.strip() for raw in match.group("body").split(",") if raw.strip()]
        if "..." in tokens:
            continue
        for token in tokens:
            if not token:
                continue
            if INT_RE.fullmatch(token):
                pins.append(int(token, 10))
            else:
                unresolved.append(token)
        packs.append(Pack(path=path, line=line_of(text, match.start()), pins=tuple(pins), unresolved=tuple(unresolved)))
    return packs


def read_packs(files: list[Path]) -> list[Pack]:
    packs: list[Pack] = []
    for path in files:
        if not path.is_file():
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            continue
        packs.extend(parse_pack(path, text))
    return packs


def check_pack(pack: Pack, max_pin: int) -> list[str]:
    problems: list[str] = []
    seen: dict[int, int] = {}
    for index, pin in enumerate(pack.pins):
        if pin < 0:
            continue
        if pin > max_pin:
            problems.append(f"{display(pack.path)}:{pack.line}: GPIO{pin} exceeds GPIO{max_pin}")
        previous = seen.get(pin)
        if previous is not None:
            problems.append(
                f"{display(pack.path)}:{pack.line}: duplicate GPIO{pin} at positions {previous + 1} and {index + 1}"
            )
        seen[pin] = index
    return problems


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Check literal arc::Pins topology packs")
    parser.add_argument(
        "paths", nargs="*", type=Path, help="files or directories to scan; defaults to repo topology docs/code"
    )
    parser.add_argument("--max-pin", type=int, default=48, help="highest valid ESP32-S3 GPIO number")
    parser.add_argument("--quiet", action="store_true", help="only print problems and the final status line")
    args = parser.parse_args(argv)

    paths: list[Path] = []
    for path in args.paths:
        resolved = path if path.is_absolute() else ROOT / path
        if resolved.is_dir():
            paths.extend(child for child in resolved.rglob("*") if child.is_file())
        else:
            paths.append(resolved)

    packs = read_packs(git_files(paths))
    problems: list[str] = []
    for pack in packs:
        problems.extend(check_pack(pack, args.max_pin))
        if not args.quiet:
            pins = ", ".join(str(pin) for pin in pack.pins) if pack.pins else "no literal pins"
            extra = f"; unresolved: {', '.join(pack.unresolved)}" if pack.unresolved else ""
            print(f"{display(pack.path)}:{pack.line}: arc::Pins<{pins}>{extra}")

    if problems:
        for problem in problems:
            print(problem, file=sys.stderr)
        return 1

    print(f"arc topology check: OK ({len(packs)} pin packs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
