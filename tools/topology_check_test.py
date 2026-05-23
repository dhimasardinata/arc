from __future__ import annotations

import json
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

    def test_accepts_cpp_and_esp_idf_pin_literals(self) -> None:
        result = self.run_tool("using Pins = arc::Pins<GPIO_NUM_4, GPIO_NUM_NC, 0x0CU, 1'3, 016>;\n")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc::Pins<4, -1, 12, 13, 14>", result.stdout)

    def test_rejects_duplicate_esp_idf_pin_literals(self) -> None:
        result = self.run_tool("using Pins = arc::Pins<GPIO_NUM_4, 4>;\n")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("duplicate GPIO4", result.stderr)

    def test_strict_unresolved_tokens_fail(self) -> None:
        result = self.run_tool("using Pins = arc::Pins<CONFIG_LED, 4>;\n", "--strict-unresolved")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("unresolved pin token 'CONFIG_LED'", result.stderr)
        self.assertIn("GPIO_NUM_<n>", result.stderr)

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

    def test_mermaid_format_draws_markdown_graph(self) -> None:
        result = self.run_tool("struct Board { using pins = arc::Pins<4, -1, CONFIG_LED>; };\n", "--format", "mermaid")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("flowchart LR", result.stdout)
        self.assertIn('gpio_4["GPIO4"]', result.stdout)
        self.assertIn('pack_0_sentinel_2["optional -1"]', result.stdout)
        self.assertIn('pack_0_unresolved_1["CONFIG_LED"]', result.stdout)
        self.assertIn("pack_0 -->|1| gpio_4", result.stdout)
        self.assertIn("pack_0 -. unresolved .-> pack_0_unresolved_1", result.stdout)
        self.assertNotIn("arc topology check: OK", result.stdout)

    def test_json_format_emits_machine_readable_evidence(self) -> None:
        result = self.run_tool("struct Board { using pins = arc::Pins<4, -1, CONFIG_LED>; };\n", "--format", "json")

        self.assertEqual(result.returncode, 0, result.stderr)
        payload = json.loads(result.stdout)
        self.assertEqual(payload["summary"], {"gpio": 1, "packs": 1, "sentinels": 1, "unresolved": 1})
        self.assertEqual(payload["packs"][0]["line"], 1)
        self.assertEqual(payload["packs"][0]["pins"], [4, -1])
        self.assertEqual(payload["packs"][0]["gpio"], [4])
        self.assertEqual(payload["packs"][0]["sentinels"], [-1])
        self.assertEqual(payload["packs"][0]["unresolved"], ["CONFIG_LED"])
        self.assertNotIn("arc topology check: OK", result.stdout)


if __name__ == "__main__":
    unittest.main()
