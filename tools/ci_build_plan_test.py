from __future__ import annotations

import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def add_project(root: Path, rel: str) -> None:
    project = root / rel
    (project / "main").mkdir(parents=True)
    (project / "CMakeLists.txt").write_text("project(test)\n", encoding="utf-8")
    (project / "main" / "CMakeLists.txt").write_text("idf_component_register()\n", encoding="utf-8")


def add_root_project(root: Path) -> None:
    (root / "main").mkdir(parents=True)
    (root / "CMakeLists.txt").write_text("project(arc)\n", encoding="utf-8")
    (root / "main" / "CMakeLists.txt").write_text("idf_component_register()\n", encoding="utf-8")


class CiBuildPlanTest(unittest.TestCase):
    def run_plan(self, root: Path, *changed: str) -> list[str]:
        args = ["python3", "tools/ci-build-plan.py", "--root", str(root), "--buildable"]
        for path in changed:
            args.extend(("--changed-file", path))
        result = subprocess.run(
            args,
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        return result.stdout.splitlines()

    def test_docs_only_change_skips_firmware_builds(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_root_project(root)
            add_project(root, "examples/esp32s3/udp")

            planned = self.run_plan(root, "README.md", "docs/architecture.md", "package.json", "package-lock.json")

        self.assertEqual(planned, [])

    def test_example_change_selects_only_that_example(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_root_project(root)
            add_project(root, "examples/esp32s3/udp")
            add_project(root, "examples/esp32s3/can")

            planned = self.run_plan(root, "examples/esp32s3/udp/main/app_main.cpp")

        self.assertEqual(planned, ["examples/esp32s3/udp"])

    def test_root_app_change_selects_root_project(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_root_project(root)
            add_project(root, "examples/esp32s3/udp")

            planned = self.run_plan(root, "main/app_main.cpp")

        self.assertEqual(planned, ["."])

    def test_shared_component_change_selects_all_buildable_projects(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_root_project(root)
            add_project(root, "examples/esp32s3/udp")
            add_project(root, "examples/esp32s31/experimental")

            planned = self.run_plan(root, "components/arc/include/arc/udp.hpp")

        self.assertEqual(planned, [".", "examples/esp32s3/udp"])

    def test_unknown_example_scope_falls_back_to_all_buildable_projects(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            add_root_project(root)
            add_project(root, "examples/esp32s3/udp")

            planned = self.run_plan(root, "examples/esp32s3/shared.txt")

        self.assertEqual(planned, [".", "examples/esp32s3/udp"])


if __name__ == "__main__":
    unittest.main()
