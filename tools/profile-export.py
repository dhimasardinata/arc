#!/usr/bin/env python3

from __future__ import annotations

import argparse
import re
import shutil
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
INCLUDE_ROOT = ROOT / "components" / "arc" / "include"
DEFAULT_OUT = ROOT / "dump" / "profiles"
PROFILES = {
    "core": "arc/core.hpp",
    "crypto": "arc/crypto.hpp",
    "math": "arc/math.hpp",
    "memory": "arc/memory.hpp",
    "net_codecs": "arc/net_codecs.hpp",
    "robotics": "arc/robotics.hpp",
    "sandbox": "arc/sandbox.hpp",
}
PROFILE_REQUIRES = {
    "core": (),
    "crypto": (
        "bootloader_support",
        "esp-tls",
        "esp_adc",
        "esp_driver_dma",
        "esp_partition",
        "esp_security",
        "mbedtls",
        "nvs_flash",
        "spi_flash",
    ),
    "math": (),
    "memory": ("esp_driver_dma",),
    "net_codecs": (),
    "robotics": (
        "esp_adc",
        "esp_driver_cam",
        "esp_driver_dma",
        "esp_driver_i2s",
        "esp_driver_ledc",
        "esp_driver_mcpwm",
        "esp_driver_rmt",
        "esp_event",
        "esp_netif",
        "esp_wifi",
        "nvs_flash",
    ),
    "sandbox": (
        "esp_system",
        "hal",
        "soc",
    ),
}
INCLUDE_RE = re.compile(r'^\s*#\s*include\s+"(arc/[^"]+)"')


def parse_args(argv: list[str]) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Export one Arc profile header closure")
    parser.add_argument("profile", choices=sorted(PROFILES), help="profile header to export")
    parser.add_argument(
        "-o",
        "--output",
        type=Path,
        help="output directory; defaults to dump/profiles/<profile>",
    )
    return parser.parse_args(argv)


def safe_output(path: Path) -> Path:
    out = path.expanduser().resolve()
    if out == ROOT or ROOT not in out.parents:
        raise ValueError("profile export output must stay inside this repository")
    return out


def include_path(rel: str) -> Path:
    path = INCLUDE_ROOT / rel
    if not path.is_file():
        raise FileNotFoundError(f"missing Arc include: {rel}")
    return path


def include_deps(rel: str) -> list[str]:
    path = include_path(rel)
    deps: list[str] = []
    for line in path.read_text(encoding="utf-8").splitlines():
        match = INCLUDE_RE.match(line)
        if match:
            deps.append(match.group(1))
    return deps


def closure(root_header: str) -> list[str]:
    seen: set[str] = set()
    ordered: list[str] = []

    def visit(rel: str) -> None:
        if rel in seen:
            return
        seen.add(rel)
        for dep in include_deps(rel):
            visit(dep)
        ordered.append(rel)

    visit(root_header)
    return sorted(ordered)


def copy_headers(headers: list[str], out: Path) -> None:
    include_out = out / "include"
    for rel in headers:
        src = include_path(rel)
        dst = include_out / rel
        dst.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(src, dst)


def write_cmake(profile: str, out: Path) -> None:
    requires = PROFILE_REQUIRES[profile]
    if not requires:
        text = 'idf_component_register(INCLUDE_DIRS "include")\n'
    else:
        deps = "\n".join(f"        {dep}" for dep in requires)
        text = f'idf_component_register(\n    INCLUDE_DIRS "include"\n    REQUIRES\n{deps}\n)\n'
    (out / "CMakeLists.txt").write_text(text, encoding="utf-8")


def write_manifest(profile: str, headers: list[str], out: Path) -> None:
    manifest = "\n".join(headers)
    (out / "PROFILE.txt").write_text(
        f"profile\t{profile}\nheaders\t{len(headers)}\n\n{manifest}\n",
        encoding="utf-8",
    )


def write_readme(profile: str, out: Path) -> None:
    header = PROFILES[profile]
    requires = PROFILE_REQUIRES[profile]
    lines = [
        f"# Arc {profile} Profile",
        "",
        f"This is a header-only export rooted at `{header}`.",
        "Copy it as an ESP-IDF component and include the profile header directly.",
        "",
        "```cpp",
        f'#include "{header}"',
        "```",
        "",
    ]
    if requires:
        lines.extend(
            [
                "The generated component declares these ESP-IDF requirements:",
                "",
                ", ".join(f"`{dep}`" for dep in requires),
                "",
            ]
        )
    (out / "README.md").write_text(
        "\n".join(lines),
        encoding="utf-8",
    )


def export_profile(profile: str, out: Path) -> int:
    headers = closure(PROFILES[profile])
    shutil.rmtree(out, ignore_errors=True)
    out.mkdir(parents=True, exist_ok=True)
    copy_headers(headers, out)
    write_cmake(profile, out)
    write_manifest(profile, headers, out)
    write_readme(profile, out)
    print(f"exported {profile} profile with {len(headers)} headers into {out.relative_to(ROOT)}")
    return 0


def main(argv: list[str]) -> int:
    args = parse_args(argv)
    out = safe_output(args.output if args.output else DEFAULT_OUT / args.profile)
    return export_profile(args.profile, out)


if __name__ == "__main__":
    try:
        raise SystemExit(main(sys.argv[1:]))
    except (OSError, ValueError) as exc:
        print(exc, file=sys.stderr)
        raise SystemExit(2)
