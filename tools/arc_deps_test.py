from __future__ import annotations

import os
import re
import subprocess
import tempfile
import textwrap
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def cmake_features() -> set[str]:
    text = (ROOT / "cmake" / "arc-deps.cmake").read_text(encoding="utf-8")
    features = set(re.findall(r'feature STREQUAL "([^"]+)"', text))
    features.discard("core")
    return features


def readme_features() -> set[str]:
    text = (ROOT / "README.md").read_text(encoding="utf-8")
    marker = "Feature names map directly to hardware lanes:"
    section = text.split(marker, 1)[1].split("\n## ", 1)[0]
    return set(re.findall(r"^- `([^`]+)`", section, flags=re.MULTILINE))


class ArcDepsTest(unittest.TestCase):
    def run_cmake(self, script: str) -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory() as tmp:
            path = Path(tmp) / "deps.cmake"
            path.write_text(script, encoding="utf-8")
            env = os.environ.copy()
            env["PYTHONDONTWRITEBYTECODE"] = "1"
            return subprocess.run(
                ["cmake", "-P", str(path)],
                cwd=ROOT,
                env=env,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

    def test_uri_is_known_zero_dependency_feature(self) -> None:
        script = textwrap.dedent(
            f"""
            include("{(ROOT / "cmake" / "arc-deps.cmake").as_posix()}")
            arc_requires(req core uri)
            if(NOT "${{req}}" STREQUAL "arc")
                message(FATAL_ERROR "unexpected uri deps: ${{req}}")
            endif()
            """
        )

        result = self.run_cmake(script)

        self.assertEqual(result.returncode, 0, result.stderr)

    def test_network_features_keep_expected_deps(self) -> None:
        script = textwrap.dedent(
            f"""
            include("{(ROOT / "cmake" / "arc-deps.cmake").as_posix()}")
            arc_requires(req uri tcp tls http)
            foreach(dep IN ITEMS arc lwip esp-tls esp_http_client)
                list(FIND req "${{dep}}" found)
                if(found EQUAL -1)
                    message(FATAL_ERROR "missing ${{dep}} from ${{req}}")
                endif()
            endforeach()
            """
        )

        result = self.run_cmake(script)

        self.assertEqual(result.returncode, 0, result.stderr)

    def test_unknown_feature_fails(self) -> None:
        script = textwrap.dedent(
            f"""
            include("{(ROOT / "cmake" / "arc-deps.cmake").as_posix()}")
            arc_requires(req definitely_unknown_arc_feature)
            """
        )

        result = self.run_cmake(script)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Unknown Arc feature", result.stderr)

    def test_readme_feature_list_matches_cmake_features(self) -> None:
        self.assertEqual(readme_features(), cmake_features())


if __name__ == "__main__":
    unittest.main()
