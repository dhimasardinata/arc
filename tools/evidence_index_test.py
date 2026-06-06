from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "evidence-index.py"

spec = importlib.util.spec_from_file_location("evidence_index", TOOL)
assert spec is not None and spec.loader is not None
evidence_index = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evidence_index)


class EvidenceIndexTest(unittest.TestCase):
    def test_collect_hashes_required_evidence_files(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            base = Path(tmp)
            first = base / "source-manifest.json"
            second = base / "release-evidence.json"
            first.write_text('{"ok": true}', encoding="utf-8")
            second.write_text('{"dirty": false}', encoding="utf-8")
            evidence = evidence_index.collect(
                [first, second],
                [evidence_index.relpath(first), evidence_index.relpath(second)],
            )

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["file_count"], 2)
        self.assertRegex(evidence["files"][0]["sha256"], r"^[0-9a-f]{64}$")

    def test_rejects_missing_required_path(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            path = Path(tmp) / "source-manifest.json"
            path.write_text("{}", encoding="utf-8")
            evidence = evidence_index.collect([path], ["missing.json"])

        self.assertFalse(evidence["ok"])
        self.assertIn("required evidence file not indexed: missing.json", evidence["problems"])

    def test_json_format_and_output_file(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            base = Path(tmp)
            evidence_file = base / "source-manifest.json"
            output = base / "index.json"
            evidence_file.write_text('{"ok": true}', encoding="utf-8")
            rel = evidence_index.relpath(evidence_file)
            result = subprocess.run(
                [
                    str(TOOL),
                    "--format",
                    "json",
                    "--require",
                    rel,
                    "--output",
                    str(output),
                    str(evidence_file),
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
            self.assertTrue(payload["ok"])
            self.assertEqual(payload["files"][0]["path"], rel)


if __name__ == "__main__":
    unittest.main()
