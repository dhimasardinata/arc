#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
PINS_RE = re.compile(r"\barc::Pins\s*<(?P<body>[^>]*)>", re.MULTILINE | re.DOTALL)
GPIO_NUM_RE = re.compile(r"^GPIO_NUM_(?P<pin>\d+|NC)$")
INT_SUFFIX_RE = re.compile(r"(ull|llu|ul|lu|u|ll|l)$", re.IGNORECASE)
OUTPUT_FORMATS = ("text", "report", "dot", "mermaid", "json")
TARGETS = ("auto", "esp32s3", "esp32s31")
TARGET_MAX_PIN = {
    "esp32s3": 48,
    "esp32s31": 61,
}
TARGET_MISSING_PINS = {
    "esp32s3": frozenset(),
    "esp32s31": frozenset({29, 41}),
}


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
        [
            "git",
            "ls-files",
            "--cached",
            "--others",
            "--exclude-standard",
            "README.md",
            "docs",
            "examples",
            "main",
            "components",
        ],
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


def parse_integer_token(token: str) -> int | None:
    cleaned = token.replace("'", "")
    if not cleaned:
        return None

    sign = 1
    if cleaned[0] in ("+", "-"):
        sign = -1 if cleaned[0] == "-" else 1
        cleaned = cleaned[1:]
    if not cleaned:
        return None
    cleaned = INT_SUFFIX_RE.sub("", cleaned)
    if not cleaned or not cleaned[0].isdigit():
        return None

    if cleaned.startswith(("0x", "0X")):
        base = 16
        digits = cleaned[2:]
    elif cleaned.startswith(("0b", "0B")):
        base = 2
        digits = cleaned[2:]
    elif cleaned != "0" and cleaned.startswith("0"):
        base = 8
        digits = cleaned[1:]
    else:
        base = 10
        digits = cleaned

    try:
        return sign * int(digits, base)
    except ValueError:
        return None


def parse_pin_token(token: str) -> int | None:
    pin = parse_integer_token(token)
    if pin is not None:
        return pin

    match = GPIO_NUM_RE.fullmatch(token.replace("'", ""))
    if match is None:
        return None
    pin = match.group("pin")
    return -1 if pin == "NC" else int(pin, 10)


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
            pin = parse_pin_token(token)
            if pin is None:
                unresolved.append(token)
            else:
                pins.append(pin)
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


def inferred_target(pack: Pack) -> str:
    path = display(pack.path).lower()
    if "esp32s31" in path or "esp32-s31" in path:
        return "esp32s31"
    return "esp32s3"


def check_pack(pack: Pack, max_pin: int, missing_pins: frozenset[int], target: str) -> list[str]:
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
        if pin in missing_pins:
            problems.append(f"{display(pack.path)}:{pack.line}: GPIO{pin} is not bonded on {target}")
        seen[pin] = index
    return problems


def check_unresolved(pack: Pack) -> list[str]:
    return [
        f"{display(pack.path)}:{pack.line}: unresolved pin token {token!r}; use an integer literal, GPIO_NUM_NC, or GPIO_NUM_<n>"
        for token in pack.unresolved
    ]


def dot_escape(value: str) -> str:
    return value.replace("\\", "\\\\").replace('"', '\\"').replace("\n", "\\n")


def mermaid_escape(value: str) -> str:
    return value.replace('"', "#quot;").replace("\n", "<br/>")


def print_text(packs: list[Pack]) -> None:
    for pack in packs:
        pins = ", ".join(str(pin) for pin in pack.pins) if pack.pins else "no literal pins"
        extra = f"; unresolved: {', '.join(pack.unresolved)}" if pack.unresolved else ""
        print(f"{display(pack.path)}:{pack.line}: arc::Pins<{pins}>{extra}")


def print_report(packs: list[Pack]) -> None:
    print("arc topology report")
    if not packs:
        print("- no literal arc::Pins packs found")
        return
    for pack in packs:
        gpios = [pin for pin in pack.pins if pin >= 0]
        sentinels = [pin for pin in pack.pins if pin < 0]
        print(f"- {display(pack.path)}:{pack.line}")
        if gpios:
            pins = ", ".join(f"GPIO{pin}" for pin in gpios)
            print(f"  pins: {pins}")
        else:
            print("  pins: none")
        if sentinels:
            print(f"  optional sentinels: {', '.join(str(pin) for pin in sentinels)}")
        if pack.unresolved:
            print(f"  unresolved tokens: {', '.join(pack.unresolved)}")


def print_dot(packs: list[Pack]) -> None:
    print("digraph arc_topology {")
    print("  rankdir=LR;")
    print('  node [fontname="monospace"];')
    for index, pack in enumerate(packs):
        pack_id = f"pack_{index}"
        pack_label = dot_escape(f"{display(pack.path)}:{pack.line}\narc::Pins")
        print(f'  {pack_id} [shape=box, label="{pack_label}"];')
        for position, pin in enumerate(pack.pins, start=1):
            if pin < 0:
                node_id = f"{pack_id}_sentinel_{position}"
                print(f'  {node_id} [shape=note, style=dotted, label="optional {pin}"];')
                print(f'  {pack_id} -> {node_id} [style=dotted, label="{position}"];')
                continue
            node_id = f"gpio_{pin}"
            print(f'  {node_id} [shape=ellipse, label="GPIO{pin}"];')
            print(f'  {pack_id} -> {node_id} [label="{position}"];')
        for position, token in enumerate(pack.unresolved, start=1):
            node_id = f"{pack_id}_unresolved_{position}"
            print(f'  {node_id} [shape=note, style=dashed, label="{dot_escape(token)}"];')
            print(f'  {pack_id} -> {node_id} [style=dashed, label="unresolved"];')
    print("}")


def print_mermaid(packs: list[Pack]) -> None:
    print("flowchart LR")
    if not packs:
        print('  empty["no literal arc::Pins packs found"]')
        return
    for index, pack in enumerate(packs):
        pack_id = f"pack_{index}"
        pack_label = mermaid_escape(f"{display(pack.path)}:{pack.line}\narc::Pins")
        print(f'  {pack_id}["{pack_label}"]')
        for position, pin in enumerate(pack.pins, start=1):
            if pin < 0:
                node_id = f"{pack_id}_sentinel_{position}"
                print(f'  {node_id}["optional {pin}"]')
                print(f"  {pack_id} -. {position} .-> {node_id}")
                continue
            node_id = f"gpio_{pin}"
            print(f'  {node_id}["GPIO{pin}"]')
            print(f"  {pack_id} -->|{position}| {node_id}")
        for position, token in enumerate(pack.unresolved, start=1):
            node_id = f"{pack_id}_unresolved_{position}"
            print(f'  {node_id}["{mermaid_escape(token)}"]')
            print(f"  {pack_id} -. unresolved .-> {node_id}")


def print_json(packs: list[Pack]) -> None:
    payload = {
        "packs": [
            {
                "path": display(pack.path),
                "line": pack.line,
                "pins": list(pack.pins),
                "gpio": [pin for pin in pack.pins if pin >= 0],
                "sentinels": [pin for pin in pack.pins if pin < 0],
                "unresolved": list(pack.unresolved),
            }
            for pack in packs
        ],
        "summary": {
            "packs": len(packs),
            "gpio": sum(1 for pack in packs for pin in pack.pins if pin >= 0),
            "sentinels": sum(1 for pack in packs for pin in pack.pins if pin < 0),
            "unresolved": sum(len(pack.unresolved) for pack in packs),
        },
    }
    print(json.dumps(payload, indent=2, sort_keys=True))


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description="Check literal arc::Pins topology packs")
    parser.add_argument(
        "paths", nargs="*", type=Path, help="files or directories to scan; defaults to repo topology docs/code"
    )
    parser.add_argument("--target", choices=TARGETS, default="auto", help="target GPIO range to check")
    parser.add_argument("--max-pin", type=int, help="override the highest valid GPIO number")
    parser.add_argument("--quiet", action="store_true", help="only print problems and the final status line")
    parser.add_argument(
        "--strict-unresolved",
        action="store_true",
        help="fail when arc::Pins contains tokens the host checker cannot reduce",
    )
    parser.add_argument(
        "--format",
        choices=OUTPUT_FORMATS,
        default="text",
        help="output style when not quiet: text list, beginner report, Graphviz DOT, Mermaid, or JSON",
    )
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
        target = inferred_target(pack) if args.target == "auto" else args.target
        max_pin = args.max_pin if args.max_pin is not None else TARGET_MAX_PIN[target]
        missing_pins = frozenset() if args.max_pin is not None else TARGET_MISSING_PINS[target]
        problems.extend(check_pack(pack, max_pin, missing_pins, target))
        if args.strict_unresolved:
            problems.extend(check_unresolved(pack))
    if not args.quiet:
        if args.format == "report":
            print_report(packs)
        elif args.format == "dot":
            print_dot(packs)
        elif args.format == "mermaid":
            print_mermaid(packs)
        elif args.format == "json":
            print_json(packs)
        else:
            print_text(packs)

    if problems:
        for problem in problems:
            print(problem, file=sys.stderr)
        return 1

    if args.quiet or args.format not in ("dot", "mermaid", "json"):
        print(f"arc topology check: OK ({len(packs)} pin packs)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv[1:]))
