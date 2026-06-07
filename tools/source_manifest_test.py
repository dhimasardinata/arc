from __future__ import annotations

import importlib.util
import json
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "source-manifest.py"

spec = importlib.util.spec_from_file_location("source_manifest", TOOL)
assert spec is not None and spec.loader is not None
source_manifest = importlib.util.module_from_spec(spec)
spec.loader.exec_module(source_manifest)


class SourceManifestTest(unittest.TestCase):
    def test_collect_records_tracked_source_hashes(self) -> None:
        evidence = source_manifest.collect()

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertRegex(evidence["commit"], r"^[0-9a-f]{40}$")
        self.assertRegex(evidence["tree_sha256"], r"^[0-9a-f]{64}$")
        paths = {record["path"]: record for record in evidence["files"]}
        self.assertEqual(paths["LICENSE"]["mode"], "100644")
        self.assertRegex(paths["README.md"]["sha256"], r"^[0-9a-f]{64}$")

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
        self.assertIn("files", payload)
        self.assertIn("tree_sha256", payload)
        self.assertNotIn("arc source manifest", result.stdout)

    def test_report_format_summarizes_manifest(self) -> None:
        result = subprocess.run(
            [str(TOOL), "--format", "report"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc source manifest", result.stdout)
        self.assertIn("- tracked files:", result.stdout)
        self.assertIn("- tree sha256:", result.stdout)

    def test_tree_digest_depends_on_mode_path_hash_and_size(self) -> None:
        first = [
            {
                "mode": "100644",
                "path": "a.txt",
                "sha256": "0" * 64,
                "size": 1,
            }
        ]
        second = [
            {
                "mode": "100755",
                "path": "a.txt",
                "sha256": "0" * 64,
                "size": 1,
            }
        ]

        self.assertNotEqual(source_manifest.tree_sha256(first), source_manifest.tree_sha256(second))

    def test_source_record_accepts_regular_executable_file(self) -> None:
        problems: list[str] = []
        record = source_manifest.source_record(
            source_manifest.TrackedFile("100755", "0" * 40, "tools/source-manifest.py"),
            problems,
        )

        self.assertEqual(problems, [])
        self.assertEqual(record["mode"], "100755")
        self.assertGreater(record["size"], 0)
        self.assertRegex(record["sha256"], r"^[0-9a-f]{64}$")

    def test_source_record_rejects_unsupported_git_mode(self) -> None:
        problems: list[str] = []
        record = source_manifest.source_record(
            source_manifest.TrackedFile("120000", "0" * 40, "LICENSE"),
            problems,
        )

        self.assertEqual(record["size"], None)
        self.assertEqual(record["sha256"], None)
        self.assertIn("unsupported tracked file mode: 120000 LICENSE", problems)


if __name__ == "__main__":
    unittest.main()
