#!/usr/bin/env python3
from __future__ import annotations

import argparse
import base64
import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
REPOSITORY_URL = "https://github.com/dhimasardinata/arc"
THIRD_PARTY_MANIFEST = ROOT / "THIRD_PARTY_MANIFEST.json"
PACKAGE_LOCK = ROOT / "package-lock.json"


def git(args: list[str], root: Path = ROOT) -> str | None:
    try:
        return subprocess.check_output(["git", *args], cwd=root, text=True, stderr=subprocess.DEVNULL).strip()
    except (OSError, subprocess.CalledProcessError):
        return None


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


def safe_id(text: str) -> str:
    value = re.sub(r"[^A-Za-z0-9.-]+", "-", text).strip("-")
    return value or hashlib.sha256(text.encode()).hexdigest()[:12]


def unique_id(seed: str, used: set[str]) -> str:
    base = f"SPDXRef-Package-{safe_id(seed)}"
    candidate = base
    suffix = 2
    while candidate in used:
        candidate = f"{base}-{suffix}"
        suffix += 1
    used.add(candidate)
    return candidate


def string_list(value: Any) -> list[str]:
    return [item for item in value if isinstance(item, str) and item.strip()] if isinstance(value, list) else []


def npm_package_name(path: str) -> str:
    marker = "node_modules/"
    if marker not in path:
        return path
    tail = path.rsplit(marker, 1)[1]
    parts = tail.split("/")
    if parts and parts[0].startswith("@") and len(parts) > 1:
        return "/".join(parts[:2])
    return parts[0] if parts else tail


def npm_checksum(integrity: Any) -> list[dict[str, str]]:
    if not isinstance(integrity, str):
        return []
    for token in integrity.split():
        if not token.startswith("sha512-"):
            continue
        try:
            digest = base64.b64decode(token.removeprefix("sha512-"), validate=True)
        except ValueError:
            return []
        return [{"algorithm": "SHA512", "checksumValue": digest.hex()}]
    return []


def npm_license(package: dict[str, Any]) -> str:
    license_value = package.get("license")
    if isinstance(license_value, str) and license_value.strip():
        return license_value.strip()
    return "NOASSERTION"


def download_location(source: Any) -> str:
    if not isinstance(source, str):
        return "NOASSERTION"
    value = source.strip()
    if value.startswith(("http://", "https://", "git+", "pkg:")):
        return value
    return "NOASSERTION"


def base_package(commit: str | None, used: set[str]) -> dict[str, Any]:
    version = commit or "NOASSERTION"
    return {
        "SPDXID": unique_id("Arc", used),
        "name": "Arc",
        "versionInfo": version,
        "downloadLocation": f"git+{REPOSITORY_URL}.git@{commit}" if commit else REPOSITORY_URL,
        "filesAnalyzed": False,
        "licenseConcluded": "AGPL-3.0-only",
        "licenseDeclared": "AGPL-3.0-only",
        "copyrightText": "NOASSERTION",
        "homepage": REPOSITORY_URL,
    }


def third_party_packages(manifest: Any, used: set[str]) -> tuple[list[dict[str, Any]], list[str]]:
    problems: list[str] = []
    packages: list[dict[str, Any]] = []
    if not isinstance(manifest, dict):
        return packages, ["THIRD_PARTY_MANIFEST.json root must be a JSON object"]
    components = manifest.get("components")
    if not isinstance(components, list) or not components:
        return packages, ["THIRD_PARTY_MANIFEST.json components must be a non-empty list"]

    for index, component in enumerate(components):
        if not isinstance(component, dict):
            problems.append(f"components[{index}] must be an object")
            continue
        name = component.get("name")
        if not isinstance(name, str) or not name.strip():
            problems.append(f"components[{index}] name must be a non-empty string")
            continue
        summary = {
            "category": component.get("category"),
            "usage": component.get("usage"),
            "repository_state": component.get("repository_state"),
            "notice_required_when": string_list(component.get("notice_required_when")),
        }
        source = component.get("source")
        packages.append(
            {
                "SPDXID": unique_id(f"third-party-{name}", used),
                "name": name,
                "downloadLocation": download_location(source),
                "filesAnalyzed": False,
                "licenseConcluded": "NOASSERTION",
                "licenseDeclared": "NOASSERTION",
                "copyrightText": "NOASSERTION",
                "sourceInfo": source if isinstance(source, str) and source.strip() else "NOASSERTION",
                "summary": json.dumps(summary, sort_keys=True),
            }
        )
    return packages, problems


def npm_packages(package_lock: Any, used: set[str]) -> tuple[list[dict[str, Any]], list[str]]:
    problems: list[str] = []
    packages: list[dict[str, Any]] = []
    if not isinstance(package_lock, dict):
        return packages, ["package-lock.json root must be a JSON object"]
    if package_lock.get("lockfileVersion") != 3:
        problems.append("package-lock.json lockfileVersion must be 3")
    entries = package_lock.get("packages")
    if not isinstance(entries, dict):
        return packages, problems + ["package-lock.json packages must be an object"]

    for path, package in sorted(entries.items()):
        if not path or not str(path).startswith("node_modules/"):
            continue
        if not isinstance(package, dict):
            problems.append(f"{path}: package entry must be an object")
            continue
        name = npm_package_name(str(path))
        version = package.get("version")
        if not isinstance(version, str) or not version.strip():
            problems.append(f"{name}: version must be present")
            version = "NOASSERTION"
        resolved = package.get("resolved")
        download = resolved if isinstance(resolved, str) and resolved.strip() else "NOASSERTION"
        checksums = npm_checksum(package.get("integrity"))
        if not checksums:
            problems.append(f"{name}: sha512 integrity must be present for SBOM checksum evidence")
        record: dict[str, Any] = {
            "SPDXID": unique_id(f"npm-{path}", used),
            "name": name,
            "versionInfo": version,
            "downloadLocation": download,
            "filesAnalyzed": False,
            "licenseConcluded": npm_license(package),
            "licenseDeclared": npm_license(package),
            "copyrightText": "NOASSERTION",
        }
        if checksums:
            record["checksums"] = checksums
        if package.get("dev") is True:
            record["comment"] = "development dependency from package-lock.json"
        packages.append(record)
    return packages, problems


def relationships(root_id: str, package_ids: list[str]) -> list[dict[str, str]]:
    records = [
        {
            "spdxElementId": "SPDXRef-DOCUMENT",
            "relationshipType": "DESCRIBES",
            "relatedSpdxElement": root_id,
        }
    ]
    for package_id in package_ids:
        records.append(
            {
                "spdxElementId": root_id,
                "relationshipType": "DEPENDS_ON",
                "relatedSpdxElement": package_id,
            }
        )
    return records


def collect(
    manifest_path: Path = THIRD_PARTY_MANIFEST,
    package_lock_path: Path = PACKAGE_LOCK,
    root: Path = ROOT,
) -> tuple[dict[str, Any], list[str], dict[str, int]]:
    commit = git(["rev-parse", "HEAD"], root)
    created = git(["show", "-s", "--format=%cI", "HEAD"], root) or "1970-01-01T00:00:00Z"
    used: set[str] = {"SPDXRef-DOCUMENT"}
    packages: list[dict[str, Any]] = []
    problems: list[str] = []

    arc = base_package(commit, used)
    packages.append(arc)

    manifest = load_input_json(root, manifest_path, "THIRD_PARTY_MANIFEST.json", problems)
    package_lock = load_input_json(root, package_lock_path, "package-lock.json", problems)
    third_party, third_party_problems = third_party_packages(manifest, used) if manifest is not None else ([], [])
    npm, npm_problems = npm_packages(package_lock, used) if package_lock is not None else ([], [])
    packages.extend(third_party)
    packages.extend(npm)
    problems.extend(third_party_problems)
    problems.extend(npm_problems)

    doc = {
        "spdxVersion": "SPDX-2.3",
        "dataLicense": "CC0-1.0",
        "SPDXID": "SPDXRef-DOCUMENT",
        "name": "Arc software bill of materials",
        "documentNamespace": f"{REPOSITORY_URL}/spdx/{commit or 'unknown'}",
        "creationInfo": {
            "created": created,
            "creators": ["Tool: tools/sbom.py", "Organization: Arc"],
        },
        "packages": packages,
        "relationships": relationships(arc["SPDXID"], [package["SPDXID"] for package in packages[1:]]),
    }
    stats = {
        "package_count": len(packages),
        "third_party_component_count": len(third_party),
        "npm_package_count": len(npm),
        "problem_count": len(problems),
    }
    return doc, problems, stats


def report_text(stats: dict[str, int], problems: list[str]) -> str:
    lines = [
        "arc SPDX SBOM",
        f"- packages: {stats['package_count']}",
        f"- third-party components: {stats['third_party_component_count']}",
        f"- npm packages: {stats['npm_package_count']}",
        f"- problems: {stats['problem_count']}",
    ]
    if problems:
        lines.append("- problem details:")
        lines.extend(f"  - {problem}" for problem in problems)
    return "\n".join(lines)


def write_output(path: Path, payload: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(payload + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Generate an SPDX 2.3 SBOM from Arc tracked manifests.")
    parser.add_argument(
        "--third-party-manifest",
        type=Path,
        default=THIRD_PARTY_MANIFEST,
        help="third-party manifest JSON",
    )
    parser.add_argument(
        "--package-lock",
        type=Path,
        default=PACKAGE_LOCK,
        help="npm package-lock.json",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or SPDX JSON",
    )
    parser.add_argument(
        "--output",
        type=Path,
        help="write output to this file instead of stdout",
    )
    args = parser.parse_args(argv)

    document, problems, stats = collect(args.third_party_manifest, args.package_lock)
    payload = json.dumps(document, indent=2, sort_keys=True) if args.format == "json" else report_text(stats, problems)
    if args.output:
        write_output(args.output, payload)
    else:
        print(payload)

    if problems:
        print("arc SBOM failed: source manifests cannot produce complete SPDX evidence", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
