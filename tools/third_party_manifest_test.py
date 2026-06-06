from __future__ import annotations

import importlib.util
import json
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "third-party-manifest-check.py"
MANIFEST = ROOT / "THIRD_PARTY_MANIFEST.json"
NOTICES = ROOT / "THIRD_PARTY_NOTICES.md"

spec = importlib.util.spec_from_file_location("third_party_manifest_check", TOOL)
assert spec is not None and spec.loader is not None
third_party_manifest_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(third_party_manifest_check)


class ThirdPartyManifestTest(unittest.TestCase):
    def test_current_manifest_passes(self) -> None:
        evidence = third_party_manifest_check.collect(MANIFEST)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertIn("ESP-IDF", evidence["components"])
        self.assertIn("VitePress and npm dependencies", evidence["components"])

    def test_json_format_is_machine_readable(self) -> None:
        result = subprocess.run(
            [str(TOOL), "--format", "json"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        payload = json.loads(result.stdout)
        self.assertTrue(payload["ok"])
        self.assertIn("component_count", payload)
        self.assertNotIn("arc third-party manifest", result.stdout)

    def test_report_format_lists_components(self) -> None:
        result = subprocess.run(
            [str(TOOL), "--format", "report"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc third-party manifest", result.stdout)
        self.assertIn("- manifest: THIRD_PARTY_MANIFEST.json", result.stdout)
        self.assertIn("  - GitHub Actions", result.stdout)

    def test_rejects_missing_notice_terms(self) -> None:
        manifest = third_party_manifest_check.load_json(MANIFEST)
        policy = NOTICES.read_text(encoding="utf-8").replace("ESP-IDF", "SDK")
        problems = third_party_manifest_check.validate_manifest(manifest, policy)

        self.assertIn("ESP-IDF: notice term 'ESP-IDF' is missing from THIRD_PARTY_NOTICES.md", problems)

    def test_rejects_missing_required_component(self) -> None:
        manifest = third_party_manifest_check.load_json(MANIFEST)
        manifest["components"] = [
            component for component in manifest["components"] if component["name"] != "Arduino-ESP32"
        ]
        policy = NOTICES.read_text(encoding="utf-8")
        problems = third_party_manifest_check.validate_manifest(manifest, policy)

        self.assertIn("missing required component: Arduino-ESP32", problems)


if __name__ == "__main__":
    unittest.main()
