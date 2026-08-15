from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class SdkCacheLineTest(unittest.TestCase):
    def compile_with_config(self, config: str, expected: int) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            (tmp_path / "sdkconfig.h").write_text(config, encoding="utf-8")
            source = tmp_path / "probe.cpp"
            source.write_text(
                f"""
#include "arc/sdk.hpp"

static_assert(arc::cache_line == {expected}U);
static_assert((arc::cache_line & (arc::cache_line - 1U)) == 0U);
""",
                encoding="utf-8",
            )

            result = subprocess.run(
                [
                    os.environ.get("CXX", "c++"),
                    "-std=gnu++23",
                    "-fsyntax-only",
                    "-I",
                    str(tmp_path),
                    "-I",
                    str(ROOT / "components" / "arc" / "include"),
                    str(source),
                ],
                cwd=ROOT,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                check=False,
            )

        self.assertEqual(result.returncode, 0, result.stderr)

    def test_generic_cache_line_macro_wins(self) -> None:
        self.compile_with_config(
            """
#define CONFIG_CACHE_L1_CACHE_LINE_SIZE 128
#define CONFIG_ESP32S31_DATA_CACHE_LINE_SIZE 64
""",
            128,
        )

    def test_esp32s31_cache_line_fallback(self) -> None:
        self.compile_with_config("#define CONFIG_ESP32S31_DATA_CACHE_LINE_SIZE 32\n", 32)

    def test_esp32p4_cache_line_fallback(self) -> None:
        self.compile_with_config("#define CONFIG_ESP32P4_DATA_CACHE_LINE_SIZE 64\n", 64)

    def test_esp32s3_cache_line_fallback(self) -> None:
        self.compile_with_config("#define CONFIG_ESP32S3_DATA_CACHE_LINE_SIZE 32\n", 32)

    def test_default_cache_line_fallback(self) -> None:
        self.compile_with_config("", 64)


if __name__ == "__main__":
    unittest.main()
