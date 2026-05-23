from __future__ import annotations

import importlib.util
import json
import sys
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "compile-cost.py"


def load_module():
    spec = importlib.util.spec_from_file_location("compile_cost", SCRIPT)
    if spec is None or spec.loader is None:
        raise RuntimeError("failed to load compile-cost helper")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class CompileCostTest(unittest.TestCase):
    def test_aggregates_timed_complete_events_by_detail(self) -> None:
        module = load_module()
        events = [
            {"ph": "X", "cat": "template", "name": "InstantiateFunction", "dur": 4200, "args": {"detail": "arc::Flow"}},
            {"ph": "X", "cat": "template", "name": "InstantiateFunction", "dur": 800, "args": {"detail": "arc::Flow"}},
            {"ph": "X", "cat": "frontend", "name": "Source", "dur": 1200, "args": {"detail": "arc/pack.hpp"}},
            {"ph": "i", "cat": "ignored", "name": "Marker", "dur": 5000},
            {"ph": "X", "cat": "ignored", "name": "Zero", "dur": 0},
        ]

        rows, total_us, matched = module.aggregate_events(events)

        self.assertEqual(total_us, 6200)
        self.assertEqual(matched, 3)
        self.assertEqual(rows[0].key.detail, "arc::Flow")
        self.assertEqual(rows[0].total_us, 5000)
        self.assertEqual(rows[0].count, 2)

    def test_filters_category_and_min_duration(self) -> None:
        module = load_module()
        events = [
            {"ph": "X", "cat": "template", "name": "InstantiateClass", "dur": 900, "args": {"detail": "short"}},
            {"ph": "X", "cat": "template", "name": "InstantiateClass", "dur": 2500, "args": {"detail": "kept"}},
            {"ph": "X", "cat": "frontend", "name": "Source", "dur": 3000, "args": {"detail": "arc/fsm.hpp"}},
        ]

        rows, total_us, matched = module.aggregate_events(events, categories={"template"}, min_us=1000)

        self.assertEqual(total_us, 2500)
        self.assertEqual(matched, 1)
        self.assertEqual(rows[0].key.detail, "kept")

    def test_loads_trace_file_and_renders_report(self) -> None:
        module = load_module()
        with tempfile.TemporaryDirectory() as tmp:
            trace = Path(tmp) / "compile.json"
            trace.write_text(
                json.dumps(
                    {
                        "traceEvents": [
                            {
                                "ph": "X",
                                "cat": "template",
                                "name": "InstantiateClass",
                                "dur": 1500,
                                "args": {"detail": "arc::Pack"},
                            }
                        ]
                    }
                ),
                encoding="utf-8",
            )

            events = module.load_trace_events(trace)
            rows, total_us, matched = module.aggregate_events(events)
            report = module.render(rows, traces=1, events=matched, total_us=total_us, top=5)

        self.assertIn("1 trace(s)", report)
        self.assertIn("1.500", report)
        self.assertIn("arc::Pack", report)


if __name__ == "__main__":
    unittest.main()
