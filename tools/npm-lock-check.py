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
INSTALL_SCRIPT_ALLOWLIST: dict[str, dict[str, str]] = {
    "esbuild": {
        "version": "0.27.7",
        "reason": "Docs bundler selected by VitePress; lockfile pins registry URL and SHA-512 integrity.",
    },
    "fsevents": {
        "version": "2.3.3",
        "reason": "Optional macOS watcher dependency; lockfile marks it darwin-only and optional.",
    },
}


def load_json(path: Path) -> Any:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def repo_input_path(root: Path, path: Path, label: str, problems: list[str]) -> Path | None:
    full = path if path.is_absolute() else root / path
    resolved = full.resolve()
    try:
        rel = resolved.relative_to(root.resolve()).as_posix()
    except ValueError:
        problems.append(f"{label} path must stay inside repository: {path}")
        return None
    if not resolved.is_file():
        problems.append(f"{label} file is missing: {rel}")
        return None
    return resolved


def load_input_json(root: Path, path: Path, label: str, problems: list[str]) -> Any | None:
    resolved = repo_input_path(root, path, label, problems)
    if resolved is None:
        return None
    try:
        return load_json(resolved)
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        problems.append(f"{label} cannot load JSON: {exc}")
        return None


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


def allowlist_records() -> list[dict[str, str]]:
    return [
        {"name": name, "version": policy["version"], "reason": policy["reason"]}
        for name, policy in sorted(INSTALL_SCRIPT_ALLOWLIST.items())
    ]


def validate_install_script_policy(label: str, package: dict[str, Any]) -> tuple[list[str], dict[str, str] | None]:
    policy = INSTALL_SCRIPT_ALLOWLIST.get(label)
    if policy is None:
        return [f"{label}: install script is not in reviewed docs npm allowlist"], None

    version = package.get("version")
    if version != policy["version"]:
        return [
            f"{label}: install script version {version or '<missing>'} must match reviewed version {policy['version']}"
        ], None

    return (
        [],
        {
            "name": label,
            "version": policy["version"],
            "reason": policy["reason"],
        },
    )


def collect(
    package_json_path: Path = PACKAGE_JSON,
    package_lock_path: Path = PACKAGE_LOCK,
    root: Path = ROOT,
) -> dict[str, Any]:
    load_problems: list[str] = []
    package_json = load_input_json(root, package_json_path, "package.json", load_problems)
    package_lock = load_input_json(root, package_lock_path, "package-lock.json", load_problems)
    root_problems, dependency_counts = validate_root(package_json, package_lock)
    problems = [*load_problems, *root_problems]

    packages = package_lock.get("packages", {}) if isinstance(package_lock, dict) else {}
    package_records: list[dict[str, Any]] = []
    install_script_packages: list[str] = []
    allowed_install_script_packages: list[dict[str, str]] = []
    unexpected_install_script_packages: list[str] = []
    integrity_count = 0
    registry_count = 0
    if isinstance(packages, dict):
        for path, package in sorted(packages.items()):
            path_text = str(path)
            problems.extend(validate_package(path_text, package))
            if not isinstance(package, dict) or path_text == "":
                continue
            name = package_name(path_text)
            install_script = package.get("hasInstallScript") is True
            policy = INSTALL_SCRIPT_ALLOWLIST.get(name)
            if package.get("integrity"):
                integrity_count += 1
            if isinstance(package.get("resolved"), str) and package["resolved"].startswith(REGISTRY_PREFIX):
                registry_count += 1
            if install_script:
                install_script_packages.append(name)
                script_problems, allowed_record = validate_install_script_policy(name, package)
                problems.extend(script_problems)
                if allowed_record is None:
                    unexpected_install_script_packages.append(name)
                else:
                    allowed_install_script_packages.append(allowed_record)
            if path_text.startswith("node_modules/"):
                package_records.append(
                    {
                        "name": name,
                        "version": package.get("version"),
                        "integrity": bool(package.get("integrity")),
                        "install_script": install_script,
                        "install_script_allowed": install_script
                        and policy is not None
                        and package.get("version") == policy["version"],
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
        "install_script_allowlist": allowlist_records(),
        "allowed_install_script_packages": allowed_install_script_packages,
        "unexpected_install_script_packages": unexpected_install_script_packages,
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
        packages = {package["name"]: package for package in evidence["packages"]}
        allowlist = {entry["name"]: entry for entry in evidence["install_script_allowlist"]}
        for name in evidence["install_script_packages"]:
            package = packages.get(name, {})
            version = package.get("version", "<missing>")
            policy = allowlist.get(name)
            if policy and version == policy["version"]:
                lines.append(f"  - {name}@{version} (reviewed)")
            elif policy:
                lines.append(f"  - {name}@{version} (review required; expected {policy['version']})")
            else:
                lines.append(f"  - {name}@{version} (review required)")
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
