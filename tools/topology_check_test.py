from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "topology-check.py"


class TopologyCheckTest(unittest.TestCase):
    def run_tool(self, source: str, *args: str) -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory(prefix="topology-check-test-") as tmp:
            path = Path(tmp) / "board.cpp"
            path.write_text(source, encoding="utf-8")
            return subprocess.run(
                [str(TOOL), *args, str(path)],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )

    def test_accepts_unique_literal_pins_and_sentinel(self) -> None:
        result = self.run_tool("struct Board { using pins = arc::Pins<4, 5, -1, 12>; };\n")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc topology check: OK (1 pin packs)", result.stdout)

    def test_rejects_duplicate_literal_pins(self) -> None:
        result = self.run_tool("using Pins = arc::Pins<4, 5, 4>;\n")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("duplicate GPIO4", result.stderr)

    def test_rejects_out_of_range_literal_pin(self) -> None:
        result = self.run_tool("using Pins = arc::Pins<49>;\n")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("GPIO49 exceeds GPIO48", result.stderr)

    def test_reports_unresolved_tokens_without_failing(self) -> None:
        result = self.run_tool("using Pins = arc::Pins<CONFIG_LED, 4>;\n")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("unresolved: CONFIG_LED", result.stdout)

    def test_report_format_groups_pins_for_humans(self) -> None:
        result = self.run_tool("struct Board { using pins = arc::Pins<4, -1, CONFIG_LED>; };\n", "--format", "report")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc topology report", result.stdout)
        self.assertIn("pins: GPIO4", result.stdout)
        self.assertIn("optional sentinels: -1", result.stdout)
        self.assertIn("unresolved tokens: CONFIG_LED", result.stdout)

    def test_dot_format_draws_pin_pack_graph(self) -> None:
        result = self.run_tool("struct Board { using pins = arc::Pins<4, -1, CONFIG_LED>; };\n", "--format", "dot")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("digraph arc_topology", result.stdout)
        self.assertIn('label="GPIO4"', result.stdout)
        self.assertIn('label="optional -1"', result.stdout)
        self.assertIn('label="CONFIG_LED"', result.stdout)
        self.assertNotIn("arc topology check: OK", result.stdout)


if __name__ == "__main__":
    unittest.main()
