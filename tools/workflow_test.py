from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def line_of(text: str, needle: str) -> int:
    for number, line in enumerate(text.splitlines(), start=1):
        if needle in line:
            return number
    return 0


class WorkflowTest(unittest.TestCase):
    def test_host_benchmarks_run_before_firmware_builds(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        host_benchmarks = line_of(workflow, "name: Host benchmarks")
        build_firmware = line_of(workflow, "name: Build firmware")

        self.assertGreater(host_benchmarks, 0)
        self.assertGreater(build_firmware, 0)
        self.assertLess(host_benchmarks, build_firmware)

    def test_realtime_audit_is_strict_before_firmware_builds(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        audit = line_of(workflow, "go run tools/arc-audit.go -root . -all")
        build_firmware = line_of(workflow, "name: Build firmware")

        self.assertGreater(audit, 0)
        self.assertGreater(build_firmware, 0)
        self.assertLess(audit, build_firmware)

    def test_changed_project_plan_gates_firmware_builds(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        plan = line_of(workflow, "name: Plan firmware builds")
        build_firmware = line_of(workflow, "name: Build firmware")

        self.assertGreater(plan, 0)
        self.assertGreater(build_firmware, 0)
        self.assertLess(plan, build_firmware)
        self.assertIn("./tools/ci-build-plan.py --buildable", workflow)
        self.assertIn("cat .arc-build-projects", workflow)
        self.assertIn("steps.firmware-plan.outputs.count != '0'", workflow)

    def test_header_validation_uses_go_helper_with_public_header_gate(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        self.assertIn("go run tools/clangd-compile-commands.go --validate-arc-headers", workflow)
        self.assertIn("--changed-arc-headers", workflow)


if __name__ == "__main__":
    unittest.main()
