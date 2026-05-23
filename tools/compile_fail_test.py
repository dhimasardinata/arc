from __future__ import annotations

import importlib.util
import json
import sys
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "compile-fail-check.py"

spec = importlib.util.spec_from_file_location("compile_fail_check", TOOL)
assert spec is not None and spec.loader is not None
compile_fail_check = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = compile_fail_check
spec.loader.exec_module(compile_fail_check)


class CompileFailCheckTest(unittest.TestCase):
    def test_report_lists_machine_readable_case_results(self) -> None:
        results = [
            compile_fail_check.CaseResult(
                name="scoped_borrow_returns_pointer",
                must_contain="callback cannot return a reference or pointer",
                problem=None,
            ),
            compile_fail_check.CaseResult(
                name="wrong_core_static_read",
                must_contain=None,
                problem="wrong_core_static_read: expected compile failure, but compilation succeeded",
            ),
        ]

        payload = compile_fail_check.report(results)

        self.assertFalse(payload["ok"])
        self.assertEqual(payload["summary"]["cases"], 2)
        self.assertEqual(payload["summary"]["passed"], 1)
        self.assertEqual(payload["summary"]["failed"], 1)
        self.assertEqual(payload["summary"]["groups"]["scoped_borrow_result"], {"cases": 1, "passed": 1, "failed": 0})
        self.assertEqual(payload["summary"]["groups"]["static_borrow_access"], {"cases": 1, "passed": 0, "failed": 1})
        self.assertEqual(payload["cases"][0]["status"], "failed_as_expected")
        self.assertEqual(payload["cases"][0]["group"], "scoped_borrow_result")
        self.assertEqual(payload["cases"][0]["must_contain"], "callback cannot return a reference or pointer")
        self.assertEqual(payload["cases"][1]["status"], "problem")
        self.assertEqual(payload["cases"][1]["group"], "static_borrow_access")
        self.assertIn("expected compile failure", payload["cases"][1]["problem"])
        json.dumps(payload)

    def test_case_result_passed_tracks_problem(self) -> None:
        self.assertTrue(compile_fail_check.CaseResult("ok_case", None, None).passed)
        self.assertFalse(compile_fail_check.CaseResult("bad_case", None, "failed").passed)

    def test_case_group_names_main_contract_areas(self) -> None:
        self.assertEqual(compile_fail_check.case_group("wrong_core_loan_snapshot"), "static_borrow_access")
        self.assertEqual(compile_fail_check.case_group("core_msg_returns_reference"), "core_msg")
        self.assertEqual(compile_fail_check.case_group("flow_pointer_payload"), "payload_boundary")
        self.assertEqual(compile_fail_check.case_group("scoped_rpc_returns_other_endpoint"), "role_scope")
        self.assertEqual(compile_fail_check.case_group("proof_zero_subject_fact"), "proof_metadata")


if __name__ == "__main__":
    unittest.main()
