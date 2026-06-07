from __future__ import annotations

import importlib.util
import json
import subprocess
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
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
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

    def test_accepts_reviewed_install_scripts(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
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
                        "node_modules/esbuild": {
                            "version": "0.27.7",
                            "resolved": "https://registry.npmjs.org/esbuild/-/esbuild-0.27.7.tgz",
                            "integrity": "sha512-test",
                            "hasInstallScript": True,
                        },
                        "node_modules/fsevents": {
                            "version": "2.3.3",
                            "resolved": "https://registry.npmjs.org/fsevents/-/fsevents-2.3.3.tgz",
                            "integrity": "sha512-test",
                            "hasInstallScript": True,
                        },
                    },
                },
            )

            evidence = npm_lock_check.collect(package_json, package_lock)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["install_script_packages"], ["esbuild", "fsevents"])
        self.assertEqual(
            [package["name"] for package in evidence["allowed_install_script_packages"]],
            ["esbuild", "fsevents"],
        )

    def test_rejects_unsynchronized_root_dependencies(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
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
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
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
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
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

    def test_rejects_unreviewed_install_script(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
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
                        "node_modules/build-helper": {
                            "version": "1.0.0",
                            "resolved": "https://registry.npmjs.org/build-helper/-/build-helper-1.0.0.tgz",
                            "integrity": "sha512-test",
                            "hasInstallScript": True,
                        },
                    },
                },
            )

            evidence = npm_lock_check.collect(package_json, package_lock)

        self.assertFalse(evidence["ok"])
        self.assertIn("build-helper", evidence["unexpected_install_script_packages"])
        self.assertIn(
            "build-helper: install script is not in reviewed docs npm allowlist",
            evidence["problems"],
        )

    def test_rejects_reviewed_install_script_version_drift(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
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
                        "node_modules/esbuild": {
                            "version": "0.27.8",
                            "resolved": "https://registry.npmjs.org/esbuild/-/esbuild-0.27.8.tgz",
                            "integrity": "sha512-test",
                            "hasInstallScript": True,
                        },
                    },
                },
            )

            evidence = npm_lock_check.collect(package_json, package_lock)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "esbuild: install script version 0.27.8 must match reviewed version 0.27.7",
            evidence["problems"],
        )

    def test_rejects_package_json_path_outside_repository(self) -> None:
        with tempfile.TemporaryDirectory() as outside_tmp:
            package_json = Path(outside_tmp) / "package.json"
            write_json(package_json, {"name": "arc-docs"})
            evidence = npm_lock_check.collect(package_json, ROOT / "package-lock.json")

        self.assertFalse(evidence["ok"])
        self.assertIn(f"package.json path must stay inside repository: {package_json}", evidence["problems"])

    def test_cli_fails_for_package_lock_path_outside_repository(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp, tempfile.TemporaryDirectory() as outside_tmp:
            package_json = Path(tmp) / "package.json"
            package_lock = Path(outside_tmp) / "package-lock.json"
            write_json(package_json, {"name": "arc-docs"})
            write_json(
                package_lock,
                {
                    "name": "arc-docs",
                    "lockfileVersion": 3,
                    "packages": {"": {"name": "arc-docs"}},
                },
            )
            result = subprocess.run(
                [
                    str(TOOL),
                    "--format",
                    "json",
                    "--package-json",
                    str(package_json),
                    "--package-lock",
                    str(package_lock),
                ],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertEqual(result.returncode, 1)
        payload = json.loads(result.stdout)
        self.assertFalse(payload["ok"])
        self.assertIn("package-lock.json path must stay inside repository", result.stdout)
        self.assertIn("arc npm lock evidence failed", result.stderr)

    def test_reports_missing_input_json(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            missing = Path(tmp) / "missing-package-lock.json"
            evidence = npm_lock_check.collect(ROOT / "package.json", missing)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            f"package-lock.json file is missing: {missing.relative_to(ROOT).as_posix()}", evidence["problems"]
        )


if __name__ == "__main__":
    unittest.main()
