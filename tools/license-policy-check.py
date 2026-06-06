#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
PACKAGE_LOCK = ROOT / "package-lock.json"
THIRD_PARTY_MANIFEST = ROOT / "THIRD_PARTY_MANIFEST.json"

APPROVED_NPM_LICENSES = {
    "0BSD",
    "Apache-2.0",
    "BSD-2-Clause",
    "BSD-3-Clause",
    "CC0-1.0",
    "ISC",
    "MIT",
    "Python-2.0",
    "Unicode-3.0",
    "Unicode-DFS-2016",
    "Unlicense",
}
EXPRESSION_WORDS = {"AND", "OR", "WITH"}
LICENSE_TOKEN_RE = re.compile(r"[A-Za-z0-9][A-Za-z0-9.+-]*")


def load_json(path: Path) -> Any:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def package_name(path: str) -> str:
    tail = path.removeprefix("node_modules/")
    parts = tail.split("/")
    if parts and parts[0].startswith("@") and len(parts) > 1:
        return "/".join(parts[:2])
    return parts[0] if parts else tail


def license_tokens(value: str) -> list[str]:
    return [token for token in LICENSE_TOKEN_RE.findall(value) if token not in EXPRESSION_WORDS]


def validate_license(name: str, value: Any) -> tuple[str | None, list[str]]:
    if not isinstance(value, str) or not value.strip():
        return None, [f"{name}: license must be a non-empty SPDX expression"]
    license_value = value.strip()
    tokens = license_tokens(license_value)
    if not tokens:
        return license_value, [f"{name}: license must contain SPDX identifiers"]
    problems = [
        f"{name}: license token {token} is not approved by Arc docs dependency policy"
        for token in tokens
        if token not in APPROVED_NPM_LICENSES
    ]
    return license_value, problems


def collect(
    package_lock_path: Path = PACKAGE_LOCK,
    third_party_manifest_path: Path = THIRD_PARTY_MANIFEST,
) -> dict[str, Any]:
    problems: list[str] = []
    package_lock = load_json(package_lock_path)
    manifest = load_json(third_party_manifest_path)
    if not isinstance(package_lock, dict):
        return {
            "ok": False,
            "problems": ["package-lock.json root must be a JSON object"],
            "approved_npm_licenses": sorted(APPROVED_NPM_LICENSES),
            "package_count": 0,
            "packages": [],
            "third_party_components": [],
        }

    packages = package_lock.get("packages")
    if not isinstance(packages, dict) or not packages:
        problems.append("package-lock.json packages must be a non-empty object")
        packages = {}

    records: list[dict[str, Any]] = []
    for path, package in sorted(packages.items()):
        if not str(path).startswith("node_modules/"):
            continue
        if not isinstance(package, dict):
            problems.append(f"{path}: package entry must be an object")
            continue
        name = package_name(str(path))
        license_value, license_problems = validate_license(name, package.get("license"))
        problems.extend(license_problems)
        records.append(
            {
                "name": name,
                "version": package.get("version") if isinstance(package.get("version"), str) else None,
                "license": license_value,
                "dev": package.get("dev") is True,
            }
        )

    third_party_components: list[str] = []
    if isinstance(manifest, dict) and isinstance(manifest.get("components"), list):
        third_party_components = [
            component["name"]
            for component in manifest["components"]
            if isinstance(component, dict) and isinstance(component.get("name"), str)
        ]
    else:
        problems.append("THIRD_PARTY_MANIFEST.json components must be a list")

    return {
        "ok": not problems,
        "problems": problems,
        "approved_npm_licenses": sorted(APPROVED_NPM_LICENSES),
        "package_count": len(records),
        "packages": records,
        "third_party_component_count": len(third_party_components),
        "third_party_components": third_party_components,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc license policy",
        f"- npm packages: {evidence['package_count']}",
        f"- third-party component groups: {evidence['third_party_component_count']}",
        f"- approved npm licenses: {', '.join(evidence['approved_npm_licenses'])}",
        f"- problems: {len(evidence['problems'])}",
    ]
    if evidence["problems"]:
        lines.append("- problem details:")
        lines.extend(f"  - {problem}" for problem in evidence["problems"])
    return "\n".join(lines)


def payload_text(evidence: dict[str, Any], output_format: str) -> str:
    if output_format == "json":
        return json.dumps(evidence, indent=2, sort_keys=True)
    return report_text(evidence)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate Arc docs dependency license policy.")
    parser.add_argument(
        "--package-lock",
        type=Path,
        default=PACKAGE_LOCK,
        help="npm package-lock.json file",
    )
    parser.add_argument(
        "--third-party-manifest",
        type=Path,
        default=THIRD_PARTY_MANIFEST,
        help="third-party manifest JSON file",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON evidence",
    )
    args = parser.parse_args(argv)

    evidence = collect(args.package_lock, args.third_party_manifest)
    print(payload_text(evidence, args.format))
    if evidence["problems"]:
        print("arc license policy failed: dependency license policy is not satisfied", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
