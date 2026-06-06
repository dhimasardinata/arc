#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any, NamedTuple


ROOT = Path(__file__).resolve().parents[1]
SHA_RE = re.compile(r"^[0-9a-f]{40}$", re.IGNORECASE)
USES_RE = re.compile(r"^\s*(?:-\s*)?uses:\s*(?P<value>[^#\s]+)?(?P<tail>.*)$")
PERSIST_RE = re.compile(r"^\s*persist-credentials:\s*(?P<value>[^#\s]+)")
CHECKOUT_ACTION = "actions/checkout@"


class UsesRef(NamedTuple):
    path: str
    line: int
    value: str
    comment: str
    persist_credentials: str | None


def workflow_files(root: Path) -> list[Path]:
    workflow_dir = root / ".github" / "workflows"
    if not workflow_dir.is_dir():
        return []
    files = list(workflow_dir.glob("*.yml"))
    files.extend(workflow_dir.glob("*.yaml"))
    return sorted(files, key=lambda path: path.relative_to(root).as_posix())


def relpath(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def leading_spaces(line: str) -> int:
    return len(line) - len(line.lstrip(" "))


def step_indent(lines: list[str], index: int) -> int:
    line = lines[index]
    if line.lstrip().startswith("- "):
        return leading_spaces(line)
    for cursor in range(index - 1, -1, -1):
        if lines[cursor].lstrip().startswith("- "):
            return leading_spaces(lines[cursor])
    return leading_spaces(line)


def step_block(lines: list[str], index: int) -> list[str]:
    indent = step_indent(lines, index)
    block: list[str] = []
    for line in lines[index + 1 :]:
        if line.strip() and line.lstrip().startswith("- ") and leading_spaces(line) <= indent:
            break
        block.append(line)
    return block


def checkout_persist_credentials(lines: list[str], index: int, value: str) -> str | None:
    if not value.startswith(CHECKOUT_ACTION):
        return None
    for line in step_block(lines, index):
        match = PERSIST_RE.match(line)
        if match:
            return match.group("value").strip("\"'").lower()
    return None


def parse_uses(path: Path, root: Path) -> list[UsesRef]:
    refs: list[UsesRef] = []
    lines = path.read_text(encoding="utf-8").splitlines()
    for index, line in enumerate(lines):
        match = USES_RE.match(line)
        if not match:
            continue
        value = match.group("value") or ""
        tail = match.group("tail") or ""
        comment = tail.split("#", 1)[1].strip() if "#" in tail else ""
        refs.append(
            UsesRef(relpath(path, root), index + 1, value, comment, checkout_persist_credentials(lines, index, value))
        )
    return refs


def local_action(value: str) -> bool:
    return value.startswith("./") or value.startswith("../")


def docker_digest(value: str) -> bool:
    return value.startswith("docker://") and "@sha256:" in value


def validate_ref(ref: UsesRef) -> list[str]:
    problems: list[str] = []
    label = f"{ref.path}:{ref.line}: uses {ref.value or '<empty>'}"

    if not ref.value:
        return [f"{label} must name an action"]
    if local_action(ref.value):
        return problems
    if ref.value.startswith("docker://"):
        if not docker_digest(ref.value):
            problems.append(f"{label} must pin docker actions by sha256 digest")
        return problems

    if "@" not in ref.value:
        problems.append(f"{label} must include an explicit ref")
        return problems

    _, _, version = ref.value.rpartition("@")
    if not SHA_RE.fullmatch(version):
        problems.append(f"{label} must pin to a full 40-character commit SHA")
    if not ref.comment:
        problems.append(f"{label} must include a trailing version comment")
    if ref.value.startswith(CHECKOUT_ACTION) and ref.persist_credentials != "false":
        problems.append(f"{label} must set persist-credentials: false")
    return problems


def collect(root: Path = ROOT) -> dict[str, Any]:
    root = root.resolve()
    refs: list[UsesRef] = []
    for workflow in workflow_files(root):
        refs.extend(parse_uses(workflow, root))

    problems: list[str] = []
    for ref in refs:
        problems.extend(validate_ref(ref))

    return {
        "workflow_count": len(workflow_files(root)),
        "ref_count": len(refs),
        "refs": [
            {
                "path": ref.path,
                "line": ref.line,
                "uses": ref.value,
                "comment": ref.comment,
                "persist_credentials": ref.persist_credentials,
            }
            for ref in refs
        ],
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc workflow action pins",
        f"- workflows: {evidence['workflow_count']}",
        f"- action refs: {evidence['ref_count']}",
    ]
    for ref in evidence["refs"]:
        lines.append(f"  - {ref['path']}:{ref['line']}: {ref['uses']}")
    if evidence["problems"]:
        lines.append("- problems:")
        for problem in evidence["problems"]:
            lines.append(f"  - {problem}")
    return "\n".join(lines)


def payload_text(evidence: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(evidence, indent=2, sort_keys=True)
    return report_text(evidence)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate GitHub Actions refs are immutable pins.")
    parser.add_argument(
        "--root",
        type=Path,
        default=ROOT,
        help="repository root to scan",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON evidence",
    )
    args = parser.parse_args(argv)

    evidence = collect(args.root)
    print(payload_text(evidence, args.format))
    if evidence["problems"]:
        print("arc workflow action pins failed: mutable or undocumented refs found", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
