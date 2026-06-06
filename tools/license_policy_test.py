from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "license-policy-check.py"

spec = importlib.util.spec_from_file_location("license_policy_check", TOOL)
assert spec is not None and spec.loader is not None
license_policy_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(license_policy_check)


def write_json(path: Path, payload: object) -> None:
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def package_lock(*packages: tuple[str, str | None]) -> dict[str, object]:
    entries: dict[str, object] = {"": {"name": "arc-docs"}}
    for name, license_value in packages:
        record: dict[str, object] = {
            "version": "1.0.0",
            "resolved": f"https://registry.npmjs.org/{name}/-/{name}-1.0.0.tgz",
            "integrity": "sha512-test",
            "dev": True,
        }
        if license_value is not None:
            record["license"] = license_value
        entries[f"node_modules/{name}"] = record
    return {
        "name": "arc-docs",
        "lockfileVersion": 3,
        "packages": entries,
    }


def manifest() -> dict[str, object]:
    return {
        "schema": 1,
        "components": [
            {"name": "ESP-IDF"},
            {"name": "VitePress and npm dependencies"},
        ],
    }


class LicensePolicyTest(unittest.TestCase):
    def test_accepts_approved_npm_license_expressions(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            lock = root / "package-lock.json"
            third_party = root / "third-party.json"
            write_json(lock, package_lock(("vitepress", "MIT"), ("rollup", "(MIT OR Apache-2.0)")))
            write_json(third_party, manifest())
            evidence = license_policy_check.collect(lock, third_party)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["package_count"], 2)
        self.assertEqual(evidence["third_party_component_count"], 2)
        self.assertIn("MIT", evidence["approved_npm_licenses"])

    def test_rejects_missing_npm_license(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            lock = root / "package-lock.json"
            third_party = root / "third-party.json"
            write_json(lock, package_lock(("vitepress", None)))
            write_json(third_party, manifest())
            evidence = license_policy_check.collect(lock, third_party)

        self.assertFalse(evidence["ok"])
        self.assertIn("vitepress: license must be a non-empty SPDX expression", evidence["problems"])

    def test_rejects_disallowed_npm_license(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            lock = root / "package-lock.json"
            third_party = root / "third-party.json"
            write_json(lock, package_lock(("copyleft", "GPL-3.0-only")))
            write_json(third_party, manifest())
            evidence = license_policy_check.collect(lock, third_party)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "copyleft: license token GPL-3.0-only is not approved by Arc docs dependency policy",
            evidence["problems"],
        )

    def test_cli_outputs_json_evidence(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            lock = root / "package-lock.json"
            third_party = root / "third-party.json"
            write_json(lock, package_lock(("vitepress", "MIT")))
            write_json(third_party, manifest())
            result = subprocess.run(
                [
                    str(TOOL),
                    "--format",
                    "json",
                    "--package-lock",
                    str(lock),
                    "--third-party-manifest",
                    str(third_party),
                ],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        payload = json.loads(result.stdout)
        self.assertTrue(payload["ok"])
        self.assertEqual(payload["packages"][0]["license"], "MIT")

    def test_cli_fails_for_disallowed_license(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            lock = root / "package-lock.json"
            third_party = root / "third-party.json"
            write_json(lock, package_lock(("copyleft", "LGPL-3.0-only")))
            write_json(third_party, manifest())
            result = subprocess.run(
                [
                    str(TOOL),
                    "--package-lock",
                    str(lock),
                    "--third-party-manifest",
                    str(third_party),
                ],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("LGPL-3.0-only", result.stdout)
        self.assertIn("arc license policy failed", result.stderr)


if __name__ == "__main__":
    unittest.main()
