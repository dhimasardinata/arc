from __future__ import annotations

import argparse
import os
import shlex
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Sequence

from s31_manifest import S31_EXAMPLES, S31_PREVIEW_IDF_PATH, S31_TARGET

ROOT = Path(__file__).resolve().parents[1]
DEV_ROOT = Path("/dev")
SERIAL_PORT_PATTERNS = (
    "serial/by-id/*",
    "ttyACM*",
    "ttyUSB*",
    "cu.usbmodem*",
    "cu.usbserial*",
    "cu.SLAB_USBtoUART*",
)


@dataclass(frozen=True)
class S31BuildStep:
    example: str
    project: str
    command: tuple[str, ...]


def default_idf_path() -> str:
    return os.environ.get("S31_PREVIEW_IDF_PATH") or os.environ.get("ARC_IDF_PATH") or S31_PREVIEW_IDF_PATH


def selected_examples(requested: Sequence[str]) -> list[str]:
    if not requested:
        return list(S31_EXAMPLES)
    unknown = sorted(set(requested).difference(S31_EXAMPLES))
    if unknown:
        valid = ", ".join(S31_EXAMPLES)
        raise ValueError(f"unknown ESP32-S31 example: {', '.join(unknown)}; valid examples: {valid}")
    return list(dict.fromkeys(requested))


def readiness_command(idf_path: str, root: Path = ROOT) -> tuple[str, ...]:
    return (
        sys.executable,
        str(root / "tools" / "s31-readiness.py"),
        "--idf-path",
        idf_path,
        "--require-sdk",
        "--format",
        "report",
    )


def build_env(idf_path: str) -> dict[str, str]:
    env = os.environ.copy()
    env.update(
        {
            "ARC_IDF_PATH": idf_path,
            "ARC_TARGET": S31_TARGET,
            "ARC_EXPERIMENTAL_ESP32S31": "ON",
            "IDF_TARGET": S31_TARGET,
        }
    )
    return env


def serial_port_key(path: Path) -> str:
    return path.resolve().as_posix()


def serial_ports(dev_root: Path | None = None) -> list[str]:
    if dev_root is None:
        dev_root = DEV_ROOT
    if not dev_root.exists():
        return []

    found: list[str] = []
    seen_devices: set[str] = set()
    for pattern in SERIAL_PORT_PATTERNS:
        for path in sorted(dev_root.glob(pattern)):
            if path.is_dir():
                continue
            key = serial_port_key(path)
            if key in seen_devices:
                continue
            seen_devices.add(key)
            name = path.as_posix()
            found.append(name)
    return found


def idf_actions(*, flash: bool = False, monitor: bool = False) -> tuple[str, ...]:
    actions = ["build"]
    if flash or monitor:
        actions.append("flash")
    if monitor:
        actions.append("monitor")
    return tuple(actions)


def idf_command(project: str, *, flash: bool = False, monitor: bool = False, port: str | None = None) -> str:
    parts = ["idf.py", "-C", project]
    if port is not None:
        parts.extend(("-p", port))
    parts.extend(idf_actions(flash=flash, monitor=monitor))
    return " ".join(shlex.quote(part) for part in parts)


def build_steps(
    examples: Sequence[str],
    root: Path = ROOT,
    *,
    flash: bool = False,
    monitor: bool = False,
    port: str | None = None,
) -> list[S31BuildStep]:
    del root
    steps: list[S31BuildStep] = []
    for example in examples:
        project = f"examples/esp32s31/{example}"
        command = idf_command(project, flash=flash, monitor=monitor, port=port)
        steps.append(
            S31BuildStep(
                example=example,
                project=project,
                command=("bash", "-lc", f". ./env.sh && {command}"),
            )
        )
    return steps


def shell_env_prefix(idf_path: str) -> str:
    return " ".join(
        (
            f"ARC_IDF_PATH={shlex.quote(idf_path)}",
            f"ARC_TARGET={S31_TARGET}",
            "ARC_EXPERIMENTAL_ESP32S31=ON",
            f"IDF_TARGET={S31_TARGET}",
        )
    )


def dry_run_lines(
    idf_path: str,
    examples: Sequence[str],
    root: Path = ROOT,
    *,
    flash: bool = False,
    monitor: bool = False,
    port: str | None = None,
) -> list[str]:
    lines = [
        " ".join(shlex.quote(part) for part in readiness_command(idf_path, root)),
    ]
    env_prefix = shell_env_prefix(idf_path)
    for step in build_steps(examples, root, flash=flash, monitor=monitor, port=port):
        command = step.command[2]
        lines.append(f"{env_prefix} bash -lc {shlex.quote(command)}")
    return lines


def validate_board_action(
    examples: Sequence[str],
    *,
    flash: bool,
    monitor: bool,
    port: str | None,
    auto_port: bool,
) -> str | None:
    if port is not None and auto_port:
        return "--port and --auto-port are mutually exclusive"
    if port is not None and not (flash or monitor):
        return "--port is only used with --flash or --monitor"
    if auto_port and not (flash or monitor):
        return "--auto-port is only used with --flash or --monitor"
    if (flash or monitor) and len(examples) != 1:
        return "ESP32-S31 flash/monitor requires exactly one --example selection"
    if (flash or monitor) and port is None and not auto_port:
        return "ESP32-S31 flash/monitor requires --port or --auto-port"
    return None


def resolve_port(port: str | None, *, auto_port: bool) -> tuple[str | None, str | None]:
    if not auto_port:
        return port, None

    ports = serial_ports()
    if len(ports) == 1:
        return ports[0], None
    if not ports:
        return None, "no ESP32-S31/Korvo serial ports found under /dev"
    return None, "multiple ESP32-S31/Korvo serial ports found; pass --port explicitly: " + ", ".join(ports)


def run(args: argparse.Namespace, root: Path = ROOT) -> int:
    try:
        examples = selected_examples(args.example)
    except ValueError as exc:
        print(str(exc), file=sys.stderr)
        return 2

    if args.list_ports:
        ports = serial_ports()
        if not ports:
            print("no ESP32-S31/Korvo serial ports found under /dev", file=sys.stderr)
            return 1
        print("\n".join(ports))
        return 0

    validation_error = validate_board_action(
        examples,
        flash=args.flash,
        monitor=args.monitor,
        port=args.port,
        auto_port=args.auto_port,
    )
    if validation_error is not None:
        print(validation_error, file=sys.stderr)
        return 2

    port, port_error = resolve_port(args.port, auto_port=args.auto_port)
    if port_error is not None:
        print(port_error, file=sys.stderr)
        return 2

    idf_path = str(args.idf_path) if args.idf_path is not None else default_idf_path()
    if args.list:
        print("\n".join(examples))
        return 0

    if args.dry_run:
        print("\n".join(dry_run_lines(idf_path, examples, root, flash=args.flash, monitor=args.monitor, port=port)))
        return 0

    if idf_path == S31_PREVIEW_IDF_PATH:
        print(
            "ESP32-S31 builds require a real preview ESP-IDF checkout; pass --idf-path or set S31_PREVIEW_IDF_PATH.",
            file=sys.stderr,
        )
        return 2

    preflight = subprocess.run(readiness_command(idf_path, root), cwd=root, check=False)
    if preflight.returncode != 0:
        return preflight.returncode

    env = build_env(idf_path)
    for step in build_steps(examples, root, flash=args.flash, monitor=args.monitor, port=port):
        action = "monitor" if args.monitor else "flash" if args.flash else "build"
        print(f"arc s31 {action}: {step.project}", flush=True)
        proc = subprocess.run(step.command, cwd=root, env=env, check=False)
        if proc.returncode != 0:
            return proc.returncode
    return 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description="Build Arc ESP32-S31/Korvo examples with a preview ESP-IDF checkout.")
    parser.add_argument(
        "--idf-path",
        type=Path,
        default=None,
        help="Preview ESP-IDF checkout with complete ESP32-S31 target metadata.",
    )
    parser.add_argument(
        "--example",
        action="append",
        choices=S31_EXAMPLES,
        default=[],
        help="Build one ESP32-S31 example. Repeat to build a subset. Defaults to all S31 examples.",
    )
    parser.add_argument("--list", action="store_true", help="List selected ESP32-S31 examples without building.")
    parser.add_argument(
        "--dry-run", action="store_true", help="Print the preflight/build commands without running them."
    )
    parser.add_argument("--port", help="Serial port for flashing or monitoring a selected Korvo board.")
    parser.add_argument(
        "--auto-port",
        action="store_true",
        help="Use the only detected Korvo serial port for flashing or monitoring; fail when zero or multiple ports are found.",
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List likely USB serial ports for a connected ESP32-S31-Korvo board without building.",
    )
    parser.add_argument("--flash", action="store_true", help="Build and flash exactly one selected ESP32-S31 example.")
    parser.add_argument(
        "--monitor",
        action="store_true",
        help="Build, flash, and open the ESP-IDF monitor for exactly one selected ESP32-S31 example.",
    )
    args = parser.parse_args(argv)
    return run(args)


if __name__ == "__main__":
    raise SystemExit(main())
