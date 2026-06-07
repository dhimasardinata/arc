#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
DEFAULT_MANIFEST = ROOT / "THIRD_PARTY_MANIFEST.json"
REQUIRED_COMPONENTS = {
    "Arduino-ESP32",
    "ESP-IDF",
    "GitHub Actions",
    "Toolchains, Python packages, Go tools, and host OS packages",
    "VitePress and npm dependencies",
}
REQUIRED_SCALAR_FIELDS = (
    "name",
    "category",
    "usage",
    "source",
    "repository_state",
)
REQUIRED_LIST_FIELDS = (
    "notice_terms",
    "notice_required_when",
)


def load_json(path: Path) -> Any:
    with path.open(encoding="utf-8") as handle:
        return json.load(handle)


def repo_input_path(root: Path, path: Path, label: str, problems: list[str]) -> tuple[Path | None, str]:
    full = path if path.is_absolute() else root / path
    resolved = full.resolve()
    try:
        rel = resolved.relative_to(root.resolve()).as_posix()
    except ValueError:
        problems.append(f"{label} path must stay inside repository: {path}")
        return None, str(path)
    if not resolved.is_file():
        problems.append(f"{label} file is missing: {rel}")
        return None, rel
    return resolved, rel


def load_input_json(root: Path, path: Path, label: str, problems: list[str]) -> tuple[Any | None, str]:
    resolved, name = repo_input_path(root, path, label, problems)
    if resolved is None:
        return None, name
    try:
        return load_json(resolved), name
    except (OSError, UnicodeDecodeError, json.JSONDecodeError) as exc:
        problems.append(f"{label} cannot load JSON: {exc}")
        return None, name


def load_input_text(root: Path, path: Path, label: str, problems: list[str]) -> str:
    resolved, _ = repo_input_path(root, path, label, problems)
    if resolved is None:
        return ""
    try:
        return resolved.read_text(encoding="utf-8")
    except (OSError, UnicodeDecodeError) as exc:
        problems.append(f"{label} cannot read text: {exc}")
        return ""


def non_empty_string(value: Any) -> bool:
    return isinstance(value, str) and bool(value.strip())


def non_empty_string_list(value: Any) -> bool:
    return isinstance(value, list) and bool(value) and all(non_empty_string(item) for item in value)


def validate_manifest(manifest: Any, policy_text: str) -> list[str]:
    problems: list[str] = []
    if not isinstance(manifest, dict):
        return ["manifest root must be a JSON object"]

    if manifest.get("schema") != 1:
        problems.append("schema must be 1")

    policy_file = manifest.get("policy_file")
    if policy_file != "THIRD_PARTY_NOTICES.md":
        problems.append("policy_file must be THIRD_PARTY_NOTICES.md")

    components = manifest.get("components")
    if not isinstance(components, list) or not components:
        problems.append("components must be a non-empty list")
        return problems

    names: list[str] = []
    for index, component in enumerate(components):
        if not isinstance(component, dict):
            problems.append(f"components[{index}] must be an object")
            continue

        name = component.get("name")
        label = name if non_empty_string(name) else f"components[{index}]"
        if non_empty_string(name):
            names.append(str(name))

        for field in REQUIRED_SCALAR_FIELDS:
            if not non_empty_string(component.get(field)):
                problems.append(f"{label}: {field} must be a non-empty string")

        if not isinstance(component.get("bundled_in_repository"), bool):
            problems.append(f"{label}: bundled_in_repository must be true or false")

        for field in REQUIRED_LIST_FIELDS:
            if not non_empty_string_list(component.get(field)):
                problems.append(f"{label}: {field} must be a non-empty string list")

        for term in component.get("notice_terms", []):
            if isinstance(term, str) and term not in policy_text:
                problems.append(f"{label}: notice term {term!r} is missing from THIRD_PARTY_NOTICES.md")

    duplicates = sorted({name for name in names if names.count(name) > 1})
    for name in duplicates:
        problems.append(f"duplicate component: {name}")

    missing = sorted(REQUIRED_COMPONENTS.difference(names))
    for name in missing:
        problems.append(f"missing required component: {name}")

    expected_order = sorted(names, key=str.casefold)
    if names != expected_order:
        problems.append("components must be sorted by name")

    return problems


def collect(manifest_path: Path, root: Path = ROOT) -> dict[str, Any]:
    root = root.resolve()
    load_problems: list[str] = []
    manifest, manifest_name = load_input_json(root, manifest_path, "manifest", load_problems)
    policy_name = manifest.get("policy_file") if isinstance(manifest, dict) else None
    policy_path = Path(policy_name) if isinstance(policy_name, str) else Path("THIRD_PARTY_NOTICES.md")
    policy_text = load_input_text(root, policy_path, "policy_file", load_problems)
    problems = [*load_problems, *validate_manifest(manifest, policy_text)]
    components = manifest.get("components", []) if isinstance(manifest, dict) else []
    return {
        "manifest": manifest_name,
        "policy_file": policy_name,
        "component_count": len(components) if isinstance(components, list) else 0,
        "components": [component.get("name") for component in components if isinstance(component, dict)],
        "ok": not problems,
        "problems": problems,
    }


def print_report(evidence: dict[str, Any]) -> None:
    print("arc third-party manifest")
    print(f"- manifest: {evidence['manifest']}")
    print(f"- policy file: {evidence['policy_file']}")
    print(f"- components: {evidence['component_count']}")
    for name in evidence["components"]:
        print(f"  - {name}")
    if evidence["problems"]:
        print("- problems:")
        for problem in evidence["problems"]:
            print(f"  - {problem}")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Validate Arc third-party dependency notice manifest.")
    parser.add_argument(
        "--manifest",
        type=Path,
        default=DEFAULT_MANIFEST,
        help="manifest JSON file to validate",
    )
    parser.add_argument(
        "--format",
        choices=("report", "json"),
        default="report",
        help="output style: human report or JSON evidence",
    )
    args = parser.parse_args(argv)

    evidence = collect(args.manifest)
    if args.format == "json":
        print(json.dumps(evidence, indent=2, sort_keys=True))
    else:
        print_report(evidence)

    if evidence["problems"]:
        print("arc third-party manifest failed: manifest is not synchronized", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
