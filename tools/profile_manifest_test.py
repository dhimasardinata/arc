from __future__ import annotations

import importlib.util
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "profile-manifest-check.py"

spec = importlib.util.spec_from_file_location("profile_manifest_check", TOOL)
assert spec is not None and spec.loader is not None
profile_manifest_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(profile_manifest_check)


class ProfileManifestTest(unittest.TestCase):
    def test_current_profile_manifest_passes(self) -> None:
        result = subprocess.run(
            [str(TOOL)],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc profile manifest check: OK", result.stdout)

    def test_json_report_lists_profile_boundaries(self) -> None:
        profiles = profile_manifest_check.load_profiles()
        payload = profile_manifest_check.profile_report(profiles, [])

        self.assertTrue(payload["ok"])
        self.assertEqual(payload["summary"]["profiles"], 7)
        self.assertEqual(payload["summary"]["substrate"], 4)
        self.assertEqual(payload["summary"]["domain"], 3)
        names = {profile["name"] for profile in payload["profiles"]}
        self.assertIn("core", names)
        self.assertIn("robotics", names)
        robotics = next(profile for profile in payload["profiles"] if profile["name"] == "robotics")
        self.assertEqual(robotics["kind"], "domain")
        self.assertEqual(robotics["header"], "arc/robotics.hpp")
        self.assertIn("esp_driver_i2s", robotics["requires"])
        self.assertGreater(robotics["headers"], 1)

    def test_report_format_groups_profiles_for_humans(self) -> None:
        result = subprocess.run(
            [str(TOOL), "--format", "report"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc profile manifest report", result.stdout)
        self.assertIn("- profiles: 7", result.stdout)
        self.assertIn("- substrate profiles: 4", result.stdout)
        self.assertIn("- domain profiles: 3", result.stdout)
        self.assertIn("  - core (substrate)", result.stdout)
        self.assertIn("    header: arc/core.hpp", result.stdout)
        self.assertIn("  - robotics (domain)", result.stdout)
        self.assertIn("    requires: esp_adc, esp_driver_cam", result.stdout)
        self.assertNotIn("arc profile manifest check: OK", result.stdout)

    def test_cmake_parser_extracts_profile_requires(self) -> None:
        cmake = """
if(feature STREQUAL "core")
elseif(feature STREQUAL "crypto")
    list(APPEND result esp_security mbedtls)
elseif(feature STREQUAL "timer" OR feature STREQUAL "gptimer")
    list(APPEND result esp_driver_gptimer)
elseif(feature STREQUAL "sandbox")
    list(APPEND result esp_system hal soc)
endif()
"""

        parsed = profile_manifest_check.cmake_requires(cmake)

        self.assertEqual(parsed["core"], ())
        self.assertEqual(parsed["crypto"], ("esp_security", "mbedtls"))
        self.assertEqual(parsed["sandbox"], ("esp_system", "hal", "soc"))
        self.assertNotIn("timer", parsed)

    def test_manifest_rejects_cmake_drift(self) -> None:
        profiles = {
            "core": {
                "kind": "substrate",
                "header": "arc/core.hpp",
                "requires": ["unexpected_dep"],
            }
        }

        problems = profile_manifest_check.validate_profiles(profiles)

        self.assertTrue(any("requires drift" in problem for problem in problems))

    def test_substrate_profiles_cannot_include_domain_roots(self) -> None:
        profiles = profile_manifest_check.load_profiles()
        profiles["core"] = {
            **profiles["core"],
            "header": profiles["crypto"]["header"],
        }

        problems = profile_manifest_check.validate_profiles(profiles)

        self.assertTrue(any("substrate profile core includes domain profile header" in problem for problem in problems))


if __name__ == "__main__":
    unittest.main()
