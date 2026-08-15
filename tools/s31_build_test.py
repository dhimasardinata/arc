from __future__ import annotations

import io
import os
import tempfile
import unittest
from contextlib import redirect_stderr, redirect_stdout
from pathlib import Path
from unittest.mock import patch

import s31_build
from s31_manifest import S31_EXAMPLES


class S31BuildTest(unittest.TestCase):
    def test_selected_examples_defaults_to_manifest(self) -> None:
        self.assertEqual(s31_build.selected_examples(()), list(S31_EXAMPLES))

    def test_selected_examples_rejects_unknown_name(self) -> None:
        with self.assertRaisesRegex(ValueError, "unknown ESP32-S31 example: nope"):
            s31_build.selected_examples(("audio", "nope"))

    def test_dry_run_prints_preflight_and_selected_builds(self) -> None:
        lines = s31_build.dry_run_lines("/opt/preview-idf", ("audio", "usb"))

        self.assertEqual(
            lines[0],
            f"{os.sys.executable} {Path(s31_build.ROOT) / 'tools' / 's31-readiness.py'} --idf-path /opt/preview-idf --require-sdk --format report",
        )
        self.assertIn("ARC_IDF_PATH=/opt/preview-idf ARC_TARGET=esp32s31", lines[1])
        self.assertIn("idf.py -C examples/esp32s31/audio build", lines[1])
        self.assertIn("idf.py -C examples/esp32s31/usb build", lines[2])

    def test_monitor_implies_flash_and_uses_port(self) -> None:
        command = s31_build.build_steps(("audio",), monitor=True, port="/dev/ttyACM0")[0].command[2]

        self.assertEqual(command, ". ./env.sh && idf.py -C examples/esp32s31/audio -p /dev/ttyACM0 build flash monitor")

    def test_dry_run_prints_flash_monitor_command(self) -> None:
        lines = s31_build.dry_run_lines("/opt/preview-idf", ("audio",), monitor=True, port="/dev/ttyACM0")

        self.assertIn("idf.py -C examples/esp32s31/audio -p /dev/ttyACM0 build flash monitor", lines[1])

    def test_serial_ports_prefers_stable_by_id_then_usb_devices(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dev = Path(tmp)
            (dev / "serial" / "by-id").mkdir(parents=True)
            (dev / "serial" / "by-id" / "usb-esp32-s31-korvo").touch()
            (dev / "ttyUSB1").touch()
            (dev / "ttyACM0").touch()
            (dev / "ttyACM-not-dir").mkdir()

            ports = s31_build.serial_ports(dev)

        self.assertEqual(
            ports,
            [
                str(dev / "serial" / "by-id" / "usb-esp32-s31-korvo"),
                str(dev / "ttyACM0"),
                str(dev / "ttyUSB1"),
            ],
        )

    def test_serial_ports_deduplicates_by_id_aliases(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            dev = Path(tmp)
            (dev / "serial" / "by-id").mkdir(parents=True)
            (dev / "ttyACM0").touch()
            (dev / "serial" / "by-id" / "usb-esp32-s31-korvo").symlink_to(dev / "ttyACM0")

            ports = s31_build.serial_ports(dev)

        self.assertEqual(ports, [str(dev / "serial" / "by-id" / "usb-esp32-s31-korvo")])

    def test_auto_port_uses_by_id_alias_when_one_physical_device_exists(self) -> None:
        stdout = io.StringIO()

        with tempfile.TemporaryDirectory() as tmp:
            dev = Path(tmp)
            (dev / "serial" / "by-id").mkdir(parents=True)
            (dev / "ttyACM0").touch()
            (dev / "serial" / "by-id" / "usb-esp32-s31-korvo").symlink_to(dev / "ttyACM0")
            with patch("s31_build.DEV_ROOT", dev):
                with redirect_stdout(stdout):
                    code = s31_build.main(["--example", "audio", "--monitor", "--auto-port", "--dry-run"])

        self.assertEqual(code, 0)
        self.assertIn(f"-p {dev / 'serial' / 'by-id' / 'usb-esp32-s31-korvo'} build flash monitor", stdout.getvalue())

    def test_cli_list_honors_example_selection(self) -> None:
        stdout = io.StringIO()

        with redirect_stdout(stdout):
            code = s31_build.main(["--example", "audio", "--example", "usb", "--list"])

        self.assertEqual(code, 0)
        self.assertEqual(stdout.getvalue().splitlines(), ["audio", "usb"])

    def test_cli_dry_run_uses_env_preview_path(self) -> None:
        stdout = io.StringIO()

        with patch.dict(os.environ, {"S31_PREVIEW_IDF_PATH": "/opt/s31-idf"}, clear=False):
            with redirect_stdout(stdout):
                code = s31_build.main(["--example", "io", "--dry-run"])

        self.assertEqual(code, 0)
        self.assertIn("--idf-path /opt/s31-idf", stdout.getvalue())
        self.assertIn("idf.py -C examples/esp32s31/io build", stdout.getvalue())

    def test_cli_list_ports_prints_candidates_without_idf_path(self) -> None:
        stdout = io.StringIO()

        with tempfile.TemporaryDirectory() as tmp:
            dev = Path(tmp)
            (dev / "ttyACM0").touch()
            with patch("s31_build.DEV_ROOT", dev):
                with redirect_stdout(stdout):
                    code = s31_build.main(["--list-ports"])

        self.assertEqual(code, 0)
        self.assertEqual(stdout.getvalue().strip(), str(dev / "ttyACM0"))

    def test_cli_list_ports_reports_no_candidates(self) -> None:
        stderr = io.StringIO()

        with tempfile.TemporaryDirectory() as tmp:
            with patch("s31_build.DEV_ROOT", Path(tmp)):
                with redirect_stderr(stderr):
                    code = s31_build.main(["--list-ports"])

        self.assertEqual(code, 1)
        self.assertIn("no ESP32-S31/Korvo serial ports found", stderr.getvalue())

    def test_cli_auto_port_uses_single_candidate_for_monitor_dry_run(self) -> None:
        stdout = io.StringIO()

        with tempfile.TemporaryDirectory() as tmp:
            dev = Path(tmp)
            (dev / "ttyACM0").touch()
            with patch("s31_build.DEV_ROOT", dev):
                with redirect_stdout(stdout):
                    code = s31_build.main(["--example", "audio", "--monitor", "--auto-port", "--dry-run"])

        self.assertEqual(code, 0)
        self.assertIn(f"-p {dev / 'ttyACM0'} build flash monitor", stdout.getvalue())

    def test_cli_auto_port_rejects_missing_candidates(self) -> None:
        stderr = io.StringIO()

        with tempfile.TemporaryDirectory() as tmp:
            with patch("s31_build.DEV_ROOT", Path(tmp)):
                with redirect_stderr(stderr):
                    code = s31_build.main(["--example", "audio", "--flash", "--auto-port", "--dry-run"])

        self.assertEqual(code, 2)
        self.assertIn("no ESP32-S31/Korvo serial ports found", stderr.getvalue())

    def test_cli_auto_port_rejects_multiple_candidates(self) -> None:
        stderr = io.StringIO()

        with tempfile.TemporaryDirectory() as tmp:
            dev = Path(tmp)
            (dev / "ttyACM0").touch()
            (dev / "ttyUSB1").touch()
            with patch("s31_build.DEV_ROOT", dev):
                with redirect_stderr(stderr):
                    code = s31_build.main(["--example", "audio", "--flash", "--auto-port", "--dry-run"])

        self.assertEqual(code, 2)
        self.assertIn("multiple ESP32-S31/Korvo serial ports found", stderr.getvalue())
        self.assertIn(str(dev / "ttyACM0"), stderr.getvalue())
        self.assertIn(str(dev / "ttyUSB1"), stderr.getvalue())

    def test_cli_flash_rejects_multiple_examples(self) -> None:
        stderr = io.StringIO()

        with redirect_stderr(stderr):
            code = s31_build.main(["--example", "audio", "--example", "usb", "--flash", "--dry-run"])

        self.assertEqual(code, 2)
        self.assertIn("requires exactly one --example", stderr.getvalue())

    def test_cli_monitor_rejects_implicit_all_examples(self) -> None:
        stderr = io.StringIO()

        with redirect_stderr(stderr):
            code = s31_build.main(["--monitor", "--dry-run"])

        self.assertEqual(code, 2)
        self.assertIn("requires exactly one --example", stderr.getvalue())

    def test_cli_flash_requires_explicit_or_auto_port(self) -> None:
        stderr = io.StringIO()

        with redirect_stderr(stderr):
            code = s31_build.main(["--example", "audio", "--flash", "--dry-run"])

        self.assertEqual(code, 2)
        self.assertIn("requires --port or --auto-port", stderr.getvalue())

    def test_cli_monitor_requires_explicit_or_auto_port(self) -> None:
        stderr = io.StringIO()

        with redirect_stderr(stderr):
            code = s31_build.main(["--example", "audio", "--monitor", "--dry-run"])

        self.assertEqual(code, 2)
        self.assertIn("requires --port or --auto-port", stderr.getvalue())

    def test_cli_port_requires_flash_or_monitor(self) -> None:
        stderr = io.StringIO()

        with redirect_stderr(stderr):
            code = s31_build.main(["--example", "audio", "--port", "/dev/ttyACM0", "--dry-run"])

        self.assertEqual(code, 2)
        self.assertIn("--port is only used with --flash or --monitor", stderr.getvalue())

    def test_cli_auto_port_requires_flash_or_monitor(self) -> None:
        stderr = io.StringIO()

        with redirect_stderr(stderr):
            code = s31_build.main(["--example", "audio", "--auto-port", "--dry-run"])

        self.assertEqual(code, 2)
        self.assertIn("--auto-port is only used with --flash or --monitor", stderr.getvalue())

    def test_cli_port_and_auto_port_are_mutually_exclusive(self) -> None:
        stderr = io.StringIO()

        with redirect_stderr(stderr):
            code = s31_build.main(
                ["--example", "audio", "--port", "/dev/ttyACM0", "--auto-port", "--monitor", "--dry-run"]
            )

        self.assertEqual(code, 2)
        self.assertIn("--port and --auto-port are mutually exclusive", stderr.getvalue())

    def test_real_build_requires_explicit_preview_path(self) -> None:
        stderr = io.StringIO()

        with patch.dict(os.environ, {}, clear=True):
            with redirect_stderr(stderr):
                code = s31_build.main(["--example", "audio"])

        self.assertEqual(code, 2)
        self.assertIn("require a real preview ESP-IDF checkout", stderr.getvalue())

    def test_real_build_runs_preflight_then_selected_build(self) -> None:
        calls: list[tuple[tuple[str, ...], str | None]] = []

        class FakeProc:
            returncode = 0

        def fake_run(
            command: tuple[str, ...], cwd: Path, env: dict[str, str] | None = None, check: bool = False
        ) -> FakeProc:
            del cwd, check
            calls.append((tuple(command), None if env is None else env.get("ARC_TARGET")))
            return FakeProc()

        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            with patch("s31_build.subprocess.run", fake_run):
                with redirect_stdout(io.StringIO()):
                    code = s31_build.run(
                        type(
                            "Args",
                            (),
                            {
                                "example": ["audio"],
                                "idf_path": Path("/opt/preview-idf"),
                                "list": False,
                                "list_ports": False,
                                "dry_run": False,
                                "flash": False,
                                "monitor": False,
                                "port": None,
                                "auto_port": False,
                            },
                        )(),
                        root=root,
                    )

        self.assertEqual(code, 0)
        self.assertEqual(calls[0][0][:3], (os.sys.executable, str(root / "tools" / "s31-readiness.py"), "--idf-path"))
        self.assertIsNone(calls[0][1])
        self.assertEqual(calls[1][0], ("bash", "-lc", ". ./env.sh && idf.py -C examples/esp32s31/audio build"))
        self.assertEqual(calls[1][1], "esp32s31")


if __name__ == "__main__":
    unittest.main()
