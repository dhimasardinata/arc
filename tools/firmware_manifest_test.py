from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "firmware-manifest.py"

spec = importlib.util.spec_from_file_location("firmware_manifest", TOOL)
assert spec is not None and spec.loader is not None
firmware_manifest = importlib.util.module_from_spec(spec)
spec.loader.exec_module(firmware_manifest)


class FirmwareManifestTest(unittest.TestCase):
    def test_collect_hashes_firmware_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build = root / "build"
            build.mkdir()
            (build / "app.bin").write_bytes(b"app")
            (build / "app.elf").write_bytes(b"elf")
            (build / "ignored.txt").write_text("ignored", encoding="utf-8")

            evidence = firmware_manifest.collect(root, [Path("build")])

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["artifact_count"], 2)
        artifacts = {record["path"]: record for record in evidence["artifacts"]}
        self.assertEqual(artifacts["build/app.bin"]["kind"], "firmware-bin")
        self.assertEqual(artifacts["build/app.elf"]["kind"], "elf")
        self.assertRegex(artifacts["build/app.bin"]["sha256"], r"^[0-9a-f]{64}$")

    def test_known_binary_kinds_are_classified(self) -> None:
        self.assertEqual(firmware_manifest.artifact_kind(Path("bootloader.bin")), "bootloader")
        self.assertEqual(firmware_manifest.artifact_kind(Path("partition-table.bin")), "partition-table")
        self.assertEqual(firmware_manifest.artifact_kind(Path("ota_data_initial.bin")), "ota-data")

    def test_default_scan_matches_uploaded_build_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            app_build = root / "examples" / "esp32s3" / "app" / "build"
            cmake_probe = app_build / "CMakeFiles" / "probe"
            vendored = root / "examples" / "esp32s3" / "app" / "managed_components" / "dep" / "bin"
            app_build.mkdir(parents=True)
            cmake_probe.mkdir(parents=True)
            vendored.mkdir(parents=True)
            (app_build / "app.bin").write_bytes(b"app")
            (app_build / "app.elf").write_bytes(b"elf")
            (cmake_probe / "CMakeDetermineCompilerABI_C.bin").write_bytes(b"probe")
            (vendored / "testfs.bin").write_bytes(b"vendored")

            evidence = firmware_manifest.collect(root, [Path("examples")])

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(
            [record["path"] for record in evidence["artifacts"]],
            [
                "examples/esp32s3/app/build/app.bin",
                "examples/esp32s3/app/build/app.elf",
            ],
        )

    def test_json_format_is_machine_readable(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build = root / "build"
            build.mkdir()
            (build / "app.bin").write_bytes(b"app")
            result = subprocess.run(
                [str(TOOL), "--root", str(root), "--format", "json", "--require-artifacts"],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertEqual(result.returncode, 0, result.stderr)
        payload = json.loads(result.stdout)
        self.assertEqual(payload["artifact_count"], 1)
        self.assertNotIn("arc firmware manifest", result.stdout)

    def test_output_file_is_created(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            build = root / "build"
            out = root / ".arc-artifacts" / "firmware-manifest.json"
            build.mkdir()
            (build / "app.bin").write_bytes(b"app")
            result = subprocess.run(
                [str(TOOL), "--root", str(root), "--format", "json", "--output", str(out), "--require-artifacts"],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, "")
            payload = json.loads(out.read_text(encoding="utf-8"))
            self.assertEqual(payload["artifacts"][0]["path"], "build/app.bin")

    def test_require_artifacts_fails_empty_scan(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            result = subprocess.run(
                [str(TOOL), "--root", tmp, "--format", "json", "--require-artifacts"],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertEqual(result.returncode, 1)
        self.assertIn("no firmware artifacts found", result.stderr)

    def test_rejects_search_root_outside_repository(self) -> None:
        with tempfile.TemporaryDirectory() as root_tmp, tempfile.TemporaryDirectory() as outside_tmp:
            root = Path(root_tmp)
            outside = Path(outside_tmp) / "outside.bin"
            outside.write_bytes(b"firmware")
            evidence = firmware_manifest.collect(root, [outside])

        self.assertFalse(evidence["ok"])
        self.assertEqual(evidence["artifact_count"], 0)
        self.assertIn(
            f"firmware search root must stay inside repository: {outside}",
            evidence["problems"],
        )

    def test_cli_fails_for_search_root_outside_repository(self) -> None:
        with tempfile.TemporaryDirectory() as root_tmp, tempfile.TemporaryDirectory() as outside_tmp:
            outside = Path(outside_tmp) / "outside.bin"
            outside.write_bytes(b"firmware")
            result = subprocess.run(
                [str(TOOL), "--root", root_tmp, "--format", "json", str(outside)],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertEqual(result.returncode, 1)
        payload = json.loads(result.stdout)
        self.assertFalse(payload["ok"])
        self.assertIn("outside repository scope", result.stderr)


if __name__ == "__main__":
    unittest.main()
