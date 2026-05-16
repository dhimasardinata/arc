from __future__ import annotations

import importlib.util
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "hil-evidence-check.py"

spec = importlib.util.spec_from_file_location("hil_evidence_check", TOOL)
assert spec is not None and spec.loader is not None
hil_evidence_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(hil_evidence_check)


class HilEvidenceCheckTest(unittest.TestCase):
    def test_valid_artifact_passes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            artifact = Path(tmp) / "hil.jsonl"
            artifact.write_text(
                "\n".join(
                    [
                        "# serial capture from jig",
                        '{"case":"i2c_fault_recovery","status":"pass","node":"A"}',
                        '{"case":"spi_fault_recovery","status":"pass","node":"A"}',
                        '{"case":"can_priority_spam","status":"pass","node":"A"}',
                        '{"case":"espnow_stale_recover","status":"pass","node":"A"}',
                    ]
                ),
                encoding="utf-8",
            )

            result = subprocess.run(
                [str(TOOL), str(artifact)],
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc hil evidence check: OK", result.stdout)

    def test_failed_or_missing_case_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            artifact = Path(tmp) / "hil.jsonl"
            artifact.write_text(
                "\n".join(
                    [
                        '{"case":"i2c_fault_recovery","status":"pass"}',
                        '{"case":"spi_fault_recovery","status":"fail"}',
                        '{"case":"can_priority_spam","status":"pass"}',
                    ]
                ),
                encoding="utf-8",
            )

            result = subprocess.run(
                [str(TOOL), str(artifact)],
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("case spi_fault_recovery: status is 'fail'", result.stderr)
        self.assertIn("missing required HIL case: espnow_stale_recover", result.stderr)

    def test_loader_reports_bad_records(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            artifact = Path(tmp) / "hil.jsonl"
            artifact.write_text(
                "\n".join(
                    [
                        "not-json",
                        '["not", "object"]',
                        '{"status":"pass"}',
                    ]
                ),
                encoding="utf-8",
            )

            cases, problems = hil_evidence_check.load_records(artifact)

        self.assertEqual(cases, {})
        self.assertEqual(len(problems), 3)

    def test_list_defaults_does_not_require_artifact(self) -> None:
        result = subprocess.run(
            [str(TOOL), "--list-defaults"],
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("i2c_fault_recovery", result.stdout)


if __name__ == "__main__":
    unittest.main()
