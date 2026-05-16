from __future__ import annotations

import importlib.util
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "safety-case-check.py"

spec = importlib.util.spec_from_file_location("safety_case_check", TOOL)
assert spec is not None and spec.loader is not None
safety_case_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(safety_case_check)


class SafetyCaseCheckTest(unittest.TestCase):
    def test_current_safety_case_passes(self) -> None:
        result = subprocess.run(
            [str(TOOL)],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc safety-case check: OK", result.stdout)

    def test_claim_rows_require_evidence_paths(self) -> None:
        rows = safety_case_check.claim_rows(
            "\n".join(
                [
                    "| Claim | Evidence |",
                    "| --- | --- |",
                    "| Good | `README.md` backs this. |",
                    "| Bad | no source reference. |",
                    "",
                ]
            )
        )

        problems = safety_case_check.check_paths(rows, [])

        self.assertIn("claim evidence has no source path: no source reference.", problems)

    def test_overclaim_patterns_do_not_match_non_claim(self) -> None:
        text = "Arc is not certified to MISRA C++:2023."

        self.assertFalse(any(pattern.search(text) for pattern in safety_case_check.OVERCLAIMS))


if __name__ == "__main__":
    unittest.main()
