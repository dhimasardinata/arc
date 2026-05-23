from __future__ import annotations

import argparse
import io
import json
import tempfile
import unittest
from contextlib import redirect_stdout
from pathlib import Path

import arc_projects


def add_project(root: Path, rel: str) -> None:
    project = root / rel
    (project / "main").mkdir(parents=True)
    (project / "CMakeLists.txt").write_text("project(test)\n", encoding="utf-8")
    (project / "main" / "CMakeLists.txt").write_text("idf_component_register()\n", encoding="utf-8")


def add_root_project(root: Path) -> None:
    (root / "main").mkdir(parents=True)
    (root / "CMakeLists.txt").write_text("project(arc)\n", encoding="utf-8")
    (root / "main" / "CMakeLists.txt").write_text("idf_component_register()\n", encoding="utf-8")


class ArcProjectsTest(unittest.TestCase):
    def test_discovers_examples_and_skips_build_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_project(root, "examples/esp32s3/udp")
            add_project(root, "examples/esp32s31/cam")
            add_project(root, "examples/portable/pack")
            add_project(root, "examples/esp32s3/build-junk/ignored")
            add_project(root, "examples/esp32s3/managed_components/ignored")
            (root / "examples" / "esp32s3" / "missing_main").mkdir(parents=True)
            (root / "examples" / "esp32s3" / "missing_main" / "CMakeLists.txt").write_text(
                "project(no_main)\n",
                encoding="utf-8",
            )

            found = arc_projects.example_projects(root)

        self.assertEqual(
            {(project.rel, project.kind, project.target, project.experimental) for project in found},
            {
                ("examples/esp32s3/udp", "example", "esp32s3", False),
                ("examples/esp32s31/cam", "example", "esp32s31", True),
                ("examples/portable/pack", "example", "portable", False),
            },
        )

    def test_buildable_filter_omits_experimental_examples(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_root_project(root)
            add_project(root, "examples/esp32s3/udp")
            add_project(root, "examples/esp32s31/cam")
            args = argparse.Namespace(examples=False, buildable=True, experimental=False, target=None)

            found = arc_projects.selected_projects(args, root=root)

        self.assertEqual([project.rel for project in found], [".", "examples/esp32s3/udp"])

    def test_cli_json_honors_root_and_target_filter(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_project(root, "examples/esp32s3/udp")
            add_project(root, "examples/portable/pack")
            stdout = io.StringIO()

            with redirect_stdout(stdout):
                code = arc_projects.main(
                    [
                        "--root",
                        str(root),
                        "--examples",
                        "--target",
                        "portable",
                        "--json",
                    ]
                )

        self.assertEqual(code, 0)
        self.assertEqual(
            json.loads(stdout.getvalue()),
            [
                {
                    "path": "examples/portable/pack",
                    "kind": "example",
                    "target": "portable",
                    "experimental": False,
                }
            ],
        )

    def test_report_includes_build_commands_and_target_summary(self) -> None:
        projects = [
            arc_projects.ArcProject(Path("/repo"), ".", "root", "esp32s3", False),
            arc_projects.ArcProject(
                Path("/repo/examples/esp32s3/udp"),
                "examples/esp32s3/udp",
                "example",
                "esp32s3",
                False,
            ),
            arc_projects.ArcProject(
                Path("/repo/examples/esp32s31/amp"),
                "examples/esp32s31/amp",
                "example",
                "esp32s31",
                True,
            ),
        ]

        payload = arc_projects.report(projects)

        self.assertEqual(payload["summary"]["projects"], 3)
        self.assertEqual(payload["summary"]["targets"], {"esp32s3": 2, "esp32s31": 1})
        self.assertEqual(payload["projects"][0]["build_command"], "idf.py build")
        self.assertEqual(payload["projects"][1]["build_command"], "idf.py -C examples/esp32s3/udp build")
        self.assertTrue(payload["projects"][2]["experimental"])

    def test_cli_report_groups_projects_for_humans(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_root_project(root)
            add_project(root, "examples/esp32s3/udp")
            add_project(root, "examples/esp32s31/amp")
            stdout = io.StringIO()

            with redirect_stdout(stdout):
                code = arc_projects.main(["--root", str(root), "--format", "report"])

        text = stdout.getvalue()
        self.assertEqual(code, 0)
        self.assertIn("arc projects report", text)
        self.assertIn("- projects: 3", text)
        self.assertIn("  - esp32s3: 2", text)
        self.assertIn("  - esp32s31: 1", text)
        self.assertIn("  - . (root, esp32s3)", text)
        self.assertIn("    build: idf.py build", text)
        self.assertIn("  - examples/esp32s31/amp (example, esp32s31 experimental)", text)

    def test_cli_format_json_includes_summary(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_project(root, "examples/esp32s3/udp")
            stdout = io.StringIO()

            with redirect_stdout(stdout):
                code = arc_projects.main(["--root", str(root), "--examples", "--format", "json"])

        payload = json.loads(stdout.getvalue())
        self.assertEqual(code, 0)
        self.assertEqual(payload["summary"]["projects"], 1)
        self.assertEqual(payload["projects"][0]["build_command"], "idf.py -C examples/esp32s3/udp build")


if __name__ == "__main__":
    unittest.main()
