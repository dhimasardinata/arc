from __future__ import annotations

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "profile-export.py"


class ProfileExportTest(unittest.TestCase):
    def make_out(self) -> Path:
        dump = ROOT / "dump"
        dump.mkdir(exist_ok=True)
        return Path(tempfile.mkdtemp(prefix="profile-export-test-", dir=dump))

    def run_export(self, profile: str, out: Path) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [str(TOOL), profile, "-o", str(out)],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def test_core_export_copies_dependency_closure(self) -> None:
        out = self.make_out()
        try:
            result = self.run_export("core", out)

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue((out / "include" / "arc" / "core.hpp").is_file())
            self.assertTrue((out / "include" / "arc" / "spsc.hpp").is_file())
            self.assertFalse((out / "include" / "arc" / "dsp.hpp").exists())
            self.assertIn("arc/core.hpp", (out / "PROFILE.txt").read_text(encoding="utf-8"))
        finally:
            shutil.rmtree(out, ignore_errors=True)

    def test_memory_export_includes_guarded_copy_header(self) -> None:
        out = self.make_out()
        try:
            result = self.run_export("memory", out)

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertTrue((out / "include" / "arc" / "memory.hpp").is_file())
            self.assertTrue((out / "include" / "arc" / "copy.hpp").is_file())
            cmake = (out / "CMakeLists.txt").read_text(encoding="utf-8")
            self.assertIn("esp_driver_dma", cmake)
        finally:
            shutil.rmtree(out, ignore_errors=True)

    def test_domain_exports_have_profile_roots_and_requires(self) -> None:
        cases = {
            "crypto": ("crypto.hpp", "kyber.hpp", "mbedtls"),
            "robotics": ("robotics.hpp", "acoustic_slam.hpp", "esp_driver_i2s"),
            "sandbox": ("sandbox.hpp", "wasm_aot.hpp", "esp_system"),
        }

        for profile, (root, included, dependency) in cases.items():
            out = self.make_out()
            try:
                result = self.run_export(profile, out)

                self.assertEqual(result.returncode, 0, result.stderr)
                self.assertTrue((out / "include" / "arc" / root).is_file(), profile)
                self.assertTrue((out / "include" / "arc" / included).is_file(), profile)
                self.assertIn(dependency, (out / "CMakeLists.txt").read_text(encoding="utf-8"), profile)
                self.assertIn(f"arc/{root}", (out / "PROFILE.txt").read_text(encoding="utf-8"), profile)
            finally:
                shutil.rmtree(out, ignore_errors=True)

    def test_output_must_stay_in_repo(self) -> None:
        result = self.run_export("core", Path(tempfile.gettempdir()) / "arc-profile-out")

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("must stay inside this repository", result.stderr)


if __name__ == "__main__":
    unittest.main()
