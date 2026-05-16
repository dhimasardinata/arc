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


if __name__ == "__main__":
    unittest.main()
