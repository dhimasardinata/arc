#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
PACKAGE_JSON = ROOT / "package.json"
PACKAGE_LOCK = ROOT / "package-lock.json"
DEPENDENCY_FIELDS = (
    "dependencies",
    "devDependencies",
    "optionalDependencies",
    "peerDependencies",
)
REGISTRY_PREFIX = "https://registry.npmjs.org/"


def load_json(path: Path) -> Any:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def object_or_empty(value: Any) -> dict[str, Any]:
    return value if isinstance(value, dict) else {}


def validate_root(package_json: Any, package_lock: Any) -> tuple[list[str], dict[str, int]]:
    problems: list[str] = []
    counts: dict[str, int] = {}
    if not isinstance(package_json, dict):
        return ["package.json root must be a JSON object"], counts
    if not isinstance(package_lock, dict):
        return ["package-lock.json root must be a JSON object"], counts

    if package_lock.get("lockfileVersion") != 3:
        problems.append("package-lock.json lockfileVersion must be 3")
    if package_lock.get("name") != package_json.get("name"):
        problems.append("package-lock.json name must match package.json name")

    packages = package_lock.get("packages")
    if not isinstance(packages, dict) or not packages:
        problems.append("package-lock.json packages must be a non-empty object")
        return problems, counts

    root = packages.get("")
    if not isinstance(root, dict):
        problems.append('package-lock.json packages[""] must describe the root package')
        return problems, counts

    for field in DEPENDENCY_FIELDS:
        package_deps = object_or_empty(package_json.get(field))
        lock_deps = object_or_empty(root.get(field))
        counts[field] = len(package_deps)
        if package_deps != lock_deps:
            problems.append(f"package-lock.json root {field} must match package.json")

    return problems, counts


def package_name(path: str) -> str:
    return path.removeprefix("node_modules/")


def validate_package(path: str, package: Any) -> list[str]:
    problems: list[str] = []
    if not isinstance(package, dict):
        return [f"{path}: package entry must be an object"]
    if path == "" or package.get("link") is True:
        return problems

    label = package_name(path)
    version = package.get("version")
    resolved = package.get("resolved")
    integrity = package.get("integrity")

    if not isinstance(version, str) or not version.strip():
        problems.append(f"{label}: version must be present")
    if not isinstance(resolved, str) or not resolved.startswith(REGISTRY_PREFIX):
        problems.append(f"{label}: resolved must use {REGISTRY_PREFIX}")
    if not isinstance(integrity, str) or not integrity.startswith("sha512-"):
        problems.append(f"{label}: integrity must be a sha512 npm integrity string")
    return problems


def collect(package_json_path: Path = PACKAGE_JSON, package_lock_path: Path = PACKAGE_LOCK) -> dict[str, Any]:
    package_json = load_json(package_json_path)
    package_lock = load_json(package_lock_path)
    problems, dependency_counts = validate_root(package_json, package_lock)

    packages = package_lock.get("packages", {}) if isinstance(package_lock, dict) else {}
    package_records: list[dict[str, Any]] = []
    install_script_packages: list[str] = []
    integrity_count = 0
    registry_count = 0
    if isinstance(packages, dict):
        for path, package in sorted(packages.items()):
            problems.extend(validate_package(str(path), package))
            if not isinstance(package, dict) or path == "":
                continue
            if package.get("integrity"):
                integrity_count += 1
            if isinstance(package.get("resolved"), str) and package["resolved"].startswith(REGISTRY_PREFIX):
                registry_count += 1
            if package.get("hasInstallScript") is True:
                install_script_packages.append(package_name(str(path)))
            if str(path).startswith("node_modules/"):
                package_records.append(
                    {
                        "name": package_name(str(path)),
                        "version": package.get("version"),
                        "integrity": bool(package.get("integrity")),
                        "install_script": package.get("hasInstallScript") is True,
                    }
                )

    return {
        "name": package_json.get("name") if isinstance(package_json, dict) else None,
        "lockfile_version": package_lock.get("lockfileVersion") if isinstance(package_lock, dict) else None,
        "package_count": len(package_records),
        "registry_resolved_count": registry_count,
        "integrity_count": integrity_count,
        "dependency_counts": dependency_counts,
        "install_script_packages": install_script_packages,
        "packages": package_records,
        "ok": not problems,
        "problems": problems,
    }


def report_text(evidence: dict[str, Any]) -> str:
    lines = [
        "arc npm lock evidence",
        f"- name: {evidence['name']}",
        f"- lockfile version: {evidence['lockfile_version']}",
        f"- packages: {evidence['package_count']}",
        f"- integrity hashes: {evidence['integrity_count']}",
        f"- registry resolved: {evidence['registry_resolved_count']}",
    ]
    if evidence["install_script_packages"]:
        lines.append("- install scripts:")
        for name in evidence["install_script_packages"]:
            lines.append(f"  - {name}")
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
    parser = argparse.ArgumentParser(description="Validate Arc docs npm lockfile evidence.")
    parser.add_argument(
        "--package-json",
        type=Path,
        default=PACKAGE_JSON,
        help="package.json file to compare",
    )
    parser.add_argument(
        "--package-lock",
        type=Path,
        default=PACKAGE_LOCK,
        help="package-lock.json file to validate",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON evidence",
    )
    args = parser.parse_args(argv)

    evidence = collect(args.package_json, args.package_lock)
    print(payload_text(evidence, args.format))
    if evidence["problems"]:
        print("arc npm lock evidence failed: lockfile is incomplete or unsynchronized", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
