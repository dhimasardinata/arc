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


if __name__ == "__main__":
    unittest.main()
