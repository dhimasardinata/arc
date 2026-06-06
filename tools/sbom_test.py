from __future__ import annotations

import base64
import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "sbom.py"

spec = importlib.util.spec_from_file_location("sbom", TOOL)
assert spec is not None and spec.loader is not None
sbom = importlib.util.module_from_spec(spec)
spec.loader.exec_module(sbom)


def write_json(path: Path, payload: object) -> None:
    path.write_text(json.dumps(payload, indent=2), encoding="utf-8")


def sample_manifest() -> dict[str, object]:
    return {
        "schema": 1,
        "policy_file": "THIRD_PARTY_NOTICES.md",
        "components": [
            {
                "name": "ESP-IDF",
                "category": "firmware SDK",
                "usage": "Firmware build substrate.",
                "source": "Pinned external checkout.",
                "bundled_in_repository": False,
                "repository_state": "external SDK",
                "notice_terms": ["ESP-IDF"],
                "notice_required_when": ["firmware images ship"],
            }
        ],
    }


def sample_lock() -> dict[str, object]:
    digest = base64.b64encode(bytes(range(64))).decode()
    return {
        "name": "arc-docs",
        "lockfileVersion": 3,
        "packages": {
            "": {"name": "arc-docs", "devDependencies": {"vitepress": "2.0.0-alpha.17"}},
            "node_modules/vitepress": {
                "version": "2.0.0-alpha.17",
                "resolved": "https://registry.npmjs.org/vitepress/-/vitepress-2.0.0-alpha.17.tgz",
                "integrity": f"sha512-{digest}",
                "license": "MIT",
                "dev": True,
            },
        },
    }


class SbomTest(unittest.TestCase):
    def test_collect_emits_spdx_packages_from_tracked_manifests(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            root = Path(tmp)
            manifest = root / "third-party.json"
            lock = root / "package-lock.json"
            write_json(manifest, sample_manifest())
            write_json(lock, sample_lock())
            document, problems, stats = sbom.collect(manifest, lock)

        self.assertEqual(problems, [])
        self.assertEqual(document["spdxVersion"], "SPDX-2.3")
        self.assertEqual(document["dataLicense"], "CC0-1.0")
        self.assertEqual(stats["third_party_component_count"], 1)
        self.assertEqual(stats["npm_package_count"], 1)
        names = {package["name"]: package for package in document["packages"]}
        self.assertIn("Arc", names)
        self.assertEqual(names["Arc"]["licenseDeclared"], "AGPL-3.0-only")
        self.assertEqual(names["ESP-IDF"]["licenseDeclared"], "NOASSERTION")
        self.assertEqual(names["ESP-IDF"]["downloadLocation"], "NOASSERTION")
        self.assertEqual(names["ESP-IDF"]["sourceInfo"], "Pinned external checkout.")
        self.assertEqual(names["vitepress"]["licenseDeclared"], "MIT")
        self.assertEqual(names["vitepress"]["checksums"][0]["algorithm"], "SHA512")
        self.assertRegex(names["vitepress"]["checksums"][0]["checksumValue"], r"^[0-9a-f]{128}$")
        self.assertTrue(
            any(relationship["relationshipType"] == "DESCRIBES" for relationship in document["relationships"])
        )

    def test_rejects_missing_npm_integrity(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            root = Path(tmp)
            manifest = root / "third-party.json"
            lock = root / "package-lock.json"
            bad_lock = sample_lock()
            packages = bad_lock["packages"]
            assert isinstance(packages, dict)
            vitepress = packages["node_modules/vitepress"]
            assert isinstance(vitepress, dict)
            vitepress.pop("integrity")
            write_json(manifest, sample_manifest())
            write_json(lock, bad_lock)
            _, problems, stats = sbom.collect(manifest, lock)

        self.assertFalse(stats["problem_count"] == 0)
        self.assertIn("vitepress: sha512 integrity must be present for SBOM checksum evidence", problems)

    def test_cli_writes_spdx_json_file(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            root = Path(tmp)
            manifest = root / "third-party.json"
            lock = root / "package-lock.json"
            output = root / "sbom.spdx.json"
            write_json(manifest, sample_manifest())
            write_json(lock, sample_lock())
            result = subprocess.run(
                [
                    str(TOOL),
                    "--format",
                    "json",
                    "--third-party-manifest",
                    str(manifest),
                    "--package-lock",
                    str(lock),
                    "--output",
                    str(output),
                ],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, "")
            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(payload["spdxVersion"], "SPDX-2.3")
            self.assertEqual(payload["packages"][0]["name"], "Arc")

    def test_cli_fails_for_invalid_manifest(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            root = Path(tmp)
            manifest = root / "third-party.json"
            lock = root / "package-lock.json"
            write_json(manifest, {"schema": 1, "components": []})
            write_json(lock, sample_lock())
            result = subprocess.run(
                [
                    str(TOOL),
                    "--third-party-manifest",
                    str(manifest),
                    "--package-lock",
                    str(lock),
                ],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("THIRD_PARTY_MANIFEST.json components must be a non-empty list", result.stdout)
        self.assertIn("arc SBOM failed", result.stderr)


if __name__ == "__main__":
    unittest.main()
