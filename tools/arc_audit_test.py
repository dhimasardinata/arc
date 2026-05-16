from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class ArcAuditTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if env_bin := os.environ.get("ARC_AUDIT_TEST_BIN"):
            cls._bin_dir = None
            cls.arc_audit = Path(env_bin)
            return

        cls._bin_dir = tempfile.TemporaryDirectory()
        cls.arc_audit = Path(cls._bin_dir.name) / "arc-audit"
        result = subprocess.run(
            ["go", "build", "-o", str(cls.arc_audit), "tools/arc-audit.go"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr)

    @classmethod
    def tearDownClass(cls) -> None:
        if cls._bin_dir is not None:
            cls._bin_dir.cleanup()

    def run_arc_audit(self, root: Path, *args: str) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env["PYTHONDONTWRITEBYTECODE"] = "1"
        return subprocess.run(
            [str(self.arc_audit), "-root", str(root), *args],
            cwd=ROOT,
            env=env,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def test_realtime_call_graph_reports_forbidden_call(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "void slow() { malloc(4); }",
                        "void loop() { slow(); }",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_audit(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("realtime entry loop reaches forbidden token", result.stderr)
        self.assertIn('"malloc"', result.stderr)

    def test_realtime_call_graph_follows_template_calls(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "template <typename T>",
                        "void slow() { malloc(4); }",
                        "void loop() { slow<int>(); }",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_audit(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("realtime entry loop reaches forbidden token", result.stderr)
        self.assertIn('"malloc"', result.stderr)

    def test_realtime_call_graph_follows_helpers_with_default_arguments(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "void slow(int bytes = 4) { malloc(bytes); }",
                        "void loop() { slow(); }",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_audit(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("realtime entry loop reaches forbidden token", result.stderr)
        self.assertIn('"malloc"', result.stderr)

    def test_realtime_call_graph_checks_overloaded_helpers_conservatively(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "void slow(int bytes) { malloc(bytes); }",
                        "void slow() {}",
                        "void loop() { slow(4); }",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_audit(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("realtime entry loop reaches forbidden token", result.stderr)
        self.assertIn('"malloc"', result.stderr)

    def test_realtime_forbidden_token_matches_template_call_syntax(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "void loop() { heap_caps_malloc<int>(4); }",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_audit(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("realtime entry loop reaches forbidden token", result.stderr)
        self.assertIn('"heap_caps_malloc"', result.stderr)

    def test_realtime_safe_source_passes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "void tick() {}",
                        "void loop() { tick(); }",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_audit(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("checked 1 realtime entries, 0 violations", result.stdout)

    def test_block_comments_do_not_leak_forbidden_tokens(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "void loop() {",
                        "  /* malloc(4); } void slow() { heap_caps_malloc(4); */",
                        "}",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_audit(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("checked 1 realtime entries, 0 violations", result.stdout)

    def test_build_prefixed_directories_are_skipped(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "void loop() {}",
                    ]
                ),
                encoding="utf-8",
            )
            generated = root / "build-cache"
            generated.mkdir()
            (generated / "stale.cpp").write_text(
                "\n".join(
                    [
                        "#define ARC_REALTIME_PROOF 1",
                        "void loop() { malloc(4); }",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_audit(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("checked 1 realtime entries, 0 violations", result.stdout)

    def test_all_mode_scans_unannotated_loop_entries(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "app.cpp").write_text(
                "\n".join(
                    [
                        "void slow() { malloc(4); }",
                        "void step() { slow(); }",
                    ]
                ),
                encoding="utf-8",
            )

            default_result = self.run_arc_audit(root)
            all_result = self.run_arc_audit(root, "-all")

        self.assertEqual(default_result.returncode, 0, default_result.stderr)
        self.assertIn("checked 0 realtime entries, 0 violations", default_result.stdout)
        self.assertNotEqual(all_result.returncode, 0)
        self.assertIn("realtime entry step reaches forbidden token", all_result.stderr)
        self.assertIn('"malloc"', all_result.stderr)


if __name__ == "__main__":
    unittest.main()
