from __future__ import annotations

import importlib.util
import json
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "npm-lock-check.py"

spec = importlib.util.spec_from_file_location("npm_lock_check", TOOL)
assert spec and spec.loader
npm_lock_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(npm_lock_check)


def write_json(path: Path, payload: object) -> None:
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


class NpmLockCheckTest(unittest.TestCase):
    def test_accepts_synced_registry_lockfile(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package_json = root / "package.json"
            package_lock = root / "package-lock.json"
            write_json(
                package_json,
                {
                    "name": "arc-docs",
                    "devDependencies": {
                        "vitepress": "2.0.0-alpha.17",
                    },
                },
            )
            write_json(
                package_lock,
                {
                    "name": "arc-docs",
                    "lockfileVersion": 3,
                    "packages": {
                        "": {
                            "name": "arc-docs",
                            "devDependencies": {
                                "vitepress": "2.0.0-alpha.17",
                            },
                        },
                        "node_modules/vitepress": {
                            "version": "2.0.0-alpha.17",
                            "resolved": "https://registry.npmjs.org/vitepress/-/vitepress-2.0.0-alpha.17.tgz",
                            "integrity": "sha512-test",
                        },
                    },
                },
            )

            evidence = npm_lock_check.collect(package_json, package_lock)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["package_count"], 1)
        self.assertEqual(evidence["integrity_count"], 1)

    def test_rejects_unsynchronized_root_dependencies(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package_json = root / "package.json"
            package_lock = root / "package-lock.json"
            write_json(package_json, {"name": "arc-docs", "devDependencies": {"vitepress": "2"}})
            write_json(
                package_lock,
                {
                    "name": "arc-docs",
                    "lockfileVersion": 3,
                    "packages": {
                        "": {
                            "name": "arc-docs",
                            "devDependencies": {},
                        }
                    },
                },
            )

            evidence = npm_lock_check.collect(package_json, package_lock)

        self.assertFalse(evidence["ok"])
        self.assertIn("package-lock.json root devDependencies must match package.json", evidence["problems"])

    def test_rejects_non_registry_resolved_packages(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package_json = root / "package.json"
            package_lock = root / "package-lock.json"
            write_json(package_json, {"name": "arc-docs"})
            write_json(
                package_lock,
                {
                    "name": "arc-docs",
                    "lockfileVersion": 3,
                    "packages": {
                        "": {"name": "arc-docs"},
                        "node_modules/vitepress": {
                            "version": "2.0.0-alpha.17",
                            "resolved": "git+https://example.invalid/vitepress.git",
                            "integrity": "sha512-test",
                        },
                    },
                },
            )

            evidence = npm_lock_check.collect(package_json, package_lock)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "vitepress: resolved must use https://registry.npmjs.org/",
            evidence["problems"],
        )

    def test_rejects_missing_integrity_hashes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            package_json = root / "package.json"
            package_lock = root / "package-lock.json"
            write_json(package_json, {"name": "arc-docs"})
            write_json(
                package_lock,
                {
                    "name": "arc-docs",
                    "lockfileVersion": 3,
                    "packages": {
                        "": {"name": "arc-docs"},
                        "node_modules/vitepress": {
                            "version": "2.0.0-alpha.17",
                            "resolved": "https://registry.npmjs.org/vitepress/-/vitepress-2.0.0-alpha.17.tgz",
                        },
                    },
                },
            )

            evidence = npm_lock_check.collect(package_json, package_lock)

        self.assertFalse(evidence["ok"])
        self.assertIn("vitepress: integrity must be a sha512 npm integrity string", evidence["problems"])


if __name__ == "__main__":
    unittest.main()
