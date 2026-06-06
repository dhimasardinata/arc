from __future__ import annotations

import importlib.util
import json
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "release-evidence.py"

spec = importlib.util.spec_from_file_location("release_evidence", TOOL)
assert spec is not None and spec.loader is not None
release_evidence = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release_evidence)


class ReleaseEvidenceTest(unittest.TestCase):
    def test_collect_records_policy_files_and_commands(self) -> None:
        evidence = release_evidence.collect()

        self.assertRegex(evidence["commit"], r"^[0-9a-f]{40}$")
        paths = {record["path"]: record for record in evidence["policy_files"]}
        self.assertTrue(paths["RELEASE.md"]["exists"])
        self.assertTrue(paths["SECURITY.md"]["exists"])
        self.assertTrue(paths["THIRD_PARTY_NOTICES.md"]["sha256"])
        self.assertTrue(paths["THIRD_PARTY_MANIFEST.json"]["sha256"])
        self.assertIn("./tools/check-repo.sh", evidence["required_commands"])
        self.assertIn("./tools/source-manifest.py --format json --require-clean", evidence["required_commands"])
        self.assertIn("./tools/third-party-manifest-check.py --format json", evidence["required_commands"])
        self.assertIn("idf.py build", evidence["required_commands"])

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
        self.assertIn("commit", payload)
        self.assertIn("policy_files", payload)
        self.assertIn("required_commands", payload)
        self.assertNotIn("arc release evidence", result.stdout)

    def test_report_format_groups_release_metadata(self) -> None:
        result = subprocess.run(
            [str(TOOL), "--format", "report"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc release evidence", result.stdout)
        self.assertIn("- policy files:", result.stdout)
        self.assertIn("  - RELEASE.md: ok sha256=", result.stdout)
        self.assertIn("- required commands:", result.stdout)
        self.assertIn("  - ./tools/check-repo.sh", result.stdout)


if __name__ == "__main__":
    unittest.main()
