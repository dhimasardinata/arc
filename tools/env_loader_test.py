from __future__ import annotations

import os
import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class EnvLoaderTest(unittest.TestCase):
    def make_fake_idf(self, tmp: Path, *, s31: bool = False) -> Path:
        idf = tmp / "esp-idf"
        idf.mkdir()
        (idf / "export.sh").write_text("export ARC_FAKE_IDF_LOADED=1\n", encoding="utf-8")
        (idf / "export.fish").write_text("set -gx ARC_FAKE_IDF_LOADED 1\n", encoding="utf-8")
        if s31:
            for rel in (
                "components/soc/esp32s31",
                "components/hal/esp32s31",
                "components/esp_rom/esp32s31",
                "components/esp_system/ld/esp32s31",
                "tools/cmake",
                "tools/idf_py_actions",
            ):
                (idf / rel).mkdir(parents=True, exist_ok=True)
            (idf / "tools" / "cmake" / "toolchain-esp32s31.cmake").write_text("", encoding="utf-8")
            (idf / "tools" / "idf_py_actions" / "constants.py").write_text(
                "SUPPORTED_TARGETS = ['esp32s31']\n", encoding="utf-8"
            )
        return idf

    def bash_source(
        self, idf: Path, *, target: str | None = None, s31: str | None = None
    ) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env["ARC_IDF_PATH"] = str(idf)
        env.pop("ARC_TARGET", None)
        env.pop("ARC_EXPERIMENTAL_ESP32S31", None)
        env.pop("IDF_TARGET", None)
        if target is not None:
            env["ARC_TARGET"] = target
        if s31 is not None:
            env["ARC_EXPERIMENTAL_ESP32S31"] = s31
        return subprocess.run(
            [
                "bash",
                "-c",
                'source ./env.sh >/dev/null && printf \'%s %s\\n\' "$IDF_TARGET" "$ARC_FAKE_IDF_LOADED"',
            ],
            cwd=ROOT,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            check=False,
        )

    def test_bash_defaults_to_esp32s3(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.bash_source(self.make_fake_idf(Path(tmp)))

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "esp32s3 1")

    def test_bash_rejects_s31_without_gate_before_loading_idf(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.bash_source(self.make_fake_idf(Path(tmp)), target="esp32s31")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("ARC_TARGET=esp32s31 requires ARC_EXPERIMENTAL_ESP32S31=ON", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_bash_rejects_s31_with_gate_when_idf_lacks_target(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.bash_source(self.make_fake_idf(Path(tmp)), target="esp32s31", s31="ON")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("complete esp32s31 target metadata", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_bash_rejects_s31_with_partial_target_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            idf = self.make_fake_idf(Path(tmp))
            (idf / "components" / "soc" / "esp32s31").mkdir(parents=True)
            result = self.bash_source(idf, target="esp32s31", s31="ON")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("complete esp32s31 target metadata", result.stderr)
        self.assertIn("components/hal/esp32s31", result.stderr)
        self.assertEqual(result.stdout, "")

    def test_bash_accepts_s31_with_gate_and_target_metadata(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.bash_source(self.make_fake_idf(Path(tmp), s31=True), target="esp32s31", s31="ON")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "esp32s31 1")

    def test_bash_rejects_unknown_target(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = self.bash_source(self.make_fake_idf(Path(tmp)), target="esp32c6")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Unsupported ARC_TARGET='esp32c6'", result.stderr)

    @unittest.skipIf(shutil.which("fish") is None, "fish is not installed")
    def test_fish_rejects_s31_with_gate_when_idf_lacks_target(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            idf = self.make_fake_idf(Path(tmp))
            env = os.environ.copy()
            env["ARC_IDF_PATH"] = str(idf)
            env["ARC_TARGET"] = "esp32s31"
            env["ARC_EXPERIMENTAL_ESP32S31"] = "ON"
            env.pop("IDF_TARGET", None)
            result = subprocess.run(
                ["fish", "-c", "source ./env.fish >/dev/null; and printf '%s %s\\n' $IDF_TARGET $ARC_FAKE_IDF_LOADED"],
                cwd=ROOT,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("complete esp32s31 target metadata", result.stderr)
        self.assertEqual(result.stdout, "")

    @unittest.skipIf(shutil.which("fish") is None, "fish is not installed")
    def test_fish_accepts_s31_with_gate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            idf = self.make_fake_idf(Path(tmp), s31=True)
            env = os.environ.copy()
            env["ARC_IDF_PATH"] = str(idf)
            env["ARC_TARGET"] = "esp32s31"
            env["ARC_EXPERIMENTAL_ESP32S31"] = "ON"
            env.pop("IDF_TARGET", None)
            result = subprocess.run(
                ["fish", "-c", "source ./env.fish >/dev/null; and printf '%s %s\\n' $IDF_TARGET $ARC_FAKE_IDF_LOADED"],
                cwd=ROOT,
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(result.stdout.strip(), "esp32s31 1")


if __name__ == "__main__":
    unittest.main()
