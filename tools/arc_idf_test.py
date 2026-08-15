from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
ARC_IDF = ROOT / "cmake" / "arc-idf.cmake"


class ArcIdfCmakeTest(unittest.TestCase):
    def run_cmake(
        self,
        script: str,
        *,
        arc_target: str | None = None,
        s31: str | None = None,
        s31_metadata: bool = False,
    ) -> subprocess.CompletedProcess[str]:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "probe.cmake"
            path.write_text(script, encoding="utf-8")
            idf_path = root / "esp-idf"
            if s31_metadata:
                for rel in (
                    "components/soc/esp32s31",
                    "components/hal/esp32s31",
                    "components/esp_rom/esp32s31",
                    "components/esp_system/ld/esp32s31",
                    "tools/cmake",
                    "tools/idf_py_actions",
                ):
                    (idf_path / rel).mkdir(parents=True, exist_ok=True)
                (idf_path / "tools" / "cmake" / "toolchain-esp32s31.cmake").write_text("", encoding="utf-8")
                (idf_path / "tools" / "idf_py_actions" / "constants.py").write_text(
                    "PREVIEW_TARGETS = ['esp32s31']\n", encoding="utf-8"
                )
            env = os.environ.copy()
            env["IDF_PATH"] = str(idf_path)
            env.pop("ARC_TARGET", None)
            env.pop("ARC_EXPERIMENTAL_ESP32S31", None)
            env.pop("IDF_TARGET", None)
            if arc_target is not None:
                env["ARC_TARGET"] = arc_target
            if s31 is not None:
                env["ARC_EXPERIMENTAL_ESP32S31"] = s31
            return subprocess.run(
                ["cmake", "-P", str(path)],
                cwd=ROOT,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )

    def test_defaults_to_esp32s3(self) -> None:
        result = self.run_cmake(
            f"""
include("{ARC_IDF.as_posix()}")
if(NOT ARC_TARGET STREQUAL "esp32s3")
    message(FATAL_ERROR "unexpected ARC_TARGET=${{ARC_TARGET}}")
endif()
if(NOT IDF_TARGET STREQUAL "esp32s3")
    message(FATAL_ERROR "unexpected IDF_TARGET=${{IDF_TARGET}}")
endif()
arc_target(esp32s3)
"""
        )

        self.assertEqual(result.returncode, 0, result.stderr)

    def test_rejects_esp32s31_without_experimental_gate(self) -> None:
        result = self.run_cmake(
            f'include("{ARC_IDF.as_posix()}")\n',
            arc_target="esp32s31",
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ARC_TARGET=esp32s31 requires -DARC_EXPERIMENTAL_ESP32S31=ON", result.stderr)

    def test_rejects_esp32s31_when_idf_lacks_target_metadata(self) -> None:
        result = self.run_cmake(
            f'include("{ARC_IDF.as_posix()}")\n',
            arc_target="esp32s31",
            s31="ON",
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("complete esp32s31", result.stderr)
        self.assertIn("target metadata", result.stderr)

    def test_rejects_esp32s31_when_idf_has_only_soc_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            path = root / "probe.cmake"
            path.write_text(f'include("{ARC_IDF.as_posix()}")\n', encoding="utf-8")
            idf_path = root / "esp-idf"
            (idf_path / "components" / "soc" / "esp32s31").mkdir(parents=True)
            env = os.environ.copy()
            env["IDF_PATH"] = str(idf_path)
            env["ARC_TARGET"] = "esp32s31"
            env["ARC_EXPERIMENTAL_ESP32S31"] = "ON"
            env.pop("IDF_TARGET", None)
            result = subprocess.run(
                ["cmake", "-P", str(path)],
                cwd=ROOT,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("complete esp32s31", result.stderr)
        self.assertIn("target metadata", result.stderr)
        self.assertIn("components/hal/esp32s31", result.stderr)

    def test_accepts_esp32s31_env_gate_with_target_metadata(self) -> None:
        result = self.run_cmake(
            f"""
include("{ARC_IDF.as_posix()}")
if(NOT ARC_TARGET STREQUAL "esp32s31")
    message(FATAL_ERROR "unexpected ARC_TARGET=${{ARC_TARGET}}")
endif()
if(NOT IDF_TARGET STREQUAL "esp32s31")
    message(FATAL_ERROR "unexpected IDF_TARGET=${{IDF_TARGET}}")
endif()
if(NOT ARC_EXPERIMENTAL_ESP32S31)
    message(FATAL_ERROR "S31 gate did not propagate from env")
endif()
arc_target(esp32s31)
""",
            arc_target="esp32s31",
            s31="ON",
            s31_metadata=True,
        )

        self.assertEqual(result.returncode, 0, result.stderr)

    def test_arc_target_rejects_project_mismatch(self) -> None:
        result = self.run_cmake(
            f"""
include("{ARC_IDF.as_posix()}")
arc_target(esp32s3)
""",
            arc_target="esp32s31",
            s31="ON",
            s31_metadata=True,
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("This Arc project requires ARC_TARGET=esp32s3; current ARC_TARGET=esp32s31", result.stderr)


if __name__ == "__main__":
    unittest.main()
