#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Any, NamedTuple


ROOT = Path(__file__).resolve().parents[1]
SKIP_PARTS = {
    ".arc-artifacts",
    ".git",
    ".ruff_cache",
    "build",
    "dist",
    "managed_components",
    "node_modules",
}
BINARY_EXTENSIONS = {
    ".bin",
    ".elf",
    ".jpg",
    ".jpeg",
    ".png",
    ".pyc",
    ".zip",
}
MAX_BYTES = 1_000_000


class SecretPattern(NamedTuple):
    name: str
    regex: re.Pattern[str]
    description: str


class Finding(NamedTuple):
    path: str
    line: int
    pattern: str
    description: str


PATTERNS = (
    SecretPattern(
        "private-key",
        re.compile(r"-----BEGIN (?:RSA |DSA |EC |OPENSSH |PGP |ENCRYPTED )?PRIVATE KEY-----"),
        "private key material",
    ),
    SecretPattern(
        "github-token",
        re.compile(r"\b(?:(?:ghp|gho|ghu|ghs|ghr)_[A-Za-z0-9_]{36,}|github_pat_[A-Za-z0-9_]{20,}_[A-Za-z0-9_]{20,})\b"),
        "GitHub access token",
    ),
    SecretPattern(
        "aws-access-key",
        re.compile(r"\b(?:AKIA|ASIA)[0-9A-Z]{16}\b"),
        "AWS access key id",
    ),
    SecretPattern(
        "google-api-key",
        re.compile(r"\bAIza[0-9A-Za-z_-]{35}\b"),
        "Google API key",
    ),
    SecretPattern(
        "slack-token",
        re.compile(r"\bxox[abprs]-[A-Za-z0-9-]{10,}\b"),
        "Slack token",
    ),
    SecretPattern(
        "npm-token",
        re.compile(r"\bnpm_[A-Za-z0-9]{36}\b"),
        "npm access token",
    ),
    SecretPattern(
        "stripe-secret-key",
        re.compile(r"\bsk_(?:live|test)_[0-9A-Za-z]{16,}\b"),
        "Stripe secret key",
    ),
)


def git_files(root: Path) -> list[Path] | None:
    try:
        output = subprocess.check_output(
            ["git", "ls-files", "--cached", "--others", "--exclude-standard", "-z"],
            cwd=root,
            stderr=subprocess.DEVNULL,
        )
    except (OSError, subprocess.CalledProcessError):
        return None
    return [root / name.decode() for name in output.split(b"\0") if name]


def fallback_files(root: Path) -> list[Path]:
    files: list[Path] = []
    for path in root.rglob("*"):
        if not path.is_file():
            continue
        rel = path.relative_to(root)
        if any(part in SKIP_PARTS for part in rel.parts):
            continue
        files.append(path)
    return files


def candidate_files(root: Path) -> list[Path]:
    files = git_files(root)
    if files is None:
        files = fallback_files(root)
    return sorted(set(files), key=lambda path: path.relative_to(root).as_posix())


def relpath(path: Path, root: Path) -> str:
    try:
        return path.relative_to(root).as_posix()
    except ValueError:
        return path.as_posix()


def skip_file(path: Path, root: Path) -> str | None:
    rel = path.relative_to(root)
    if any(part in SKIP_PARTS for part in rel.parts):
        return "generated or dependency directory"
    if path.suffix.lower() in BINARY_EXTENSIONS:
        return "binary extension"
    try:
        if path.stat().st_size > MAX_BYTES:
            return "file too large"
    except OSError:
        return "stat failed"
    return None


def scan_text(path: Path, root: Path, text: str) -> list[Finding]:
    findings: list[Finding] = []
    for line_number, line in enumerate(text.splitlines(), start=1):
        for pattern in PATTERNS:
            if pattern.regex.search(line):
                findings.append(Finding(relpath(path, root), line_number, pattern.name, pattern.description))
    return findings


def collect(root: Path = ROOT) -> dict[str, Any]:
    root = root.resolve()
    findings: list[Finding] = []
    skipped: list[dict[str, str]] = []
    scanned_files = 0
    for path in candidate_files(root):
        if not path.is_file():
            continue
        reason = skip_file(path, root)
        if reason:
            skipped.append({"path": relpath(path, root), "reason": reason})
            continue
        try:
            text = path.read_text(encoding="utf-8")
        except UnicodeDecodeError:
            skipped.append({"path": relpath(path, root), "reason": "non-utf8"})
            continue
        except OSError:
            skipped.append({"path": relpath(path, root), "reason": "read failed"})
            continue
        scanned_files += 1
        findings.extend(scan_text(path, root, text))

    problems = [
        f"{finding.path}:{finding.line}: {finding.pattern} matched {finding.description}" for finding in findings
    ]
    return {
        "scanned_files": scanned_files,
        "skipped_files": skipped,
        "finding_count": len(findings),
        "findings": [
            {
                "path": finding.path,
                "line": finding.line,
                "pattern": finding.pattern,
                "description": finding.description,
            }
            for finding in findings
        ],
        "patterns": [
            {
                "name": pattern.name,
                "description": pattern.description,
            }
            for pattern in PATTERNS
        ],
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc secret scan",
        f"- scanned files: {evidence['scanned_files']}",
        f"- skipped files: {len(evidence['skipped_files'])}",
        f"- findings: {evidence['finding_count']}",
    ]
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
    parser = argparse.ArgumentParser(description="Scan Arc source files for high-confidence secret leaks.")
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
        print("arc secret scan failed: possible secret material found", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
