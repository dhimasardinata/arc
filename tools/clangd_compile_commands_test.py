from __future__ import annotations

import importlib.util
import shlex
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "tools" / "clangd-compile-commands.py"


def load_module():
    spec = importlib.util.spec_from_file_location("clangd_compile_commands", SCRIPT)
    if spec is None or spec.loader is None:
        raise RuntimeError("failed to load clangd compile command helper")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


class ClangdCompileCommandsTest(unittest.TestCase):
    def test_fast_command_split_matches_shell_split_for_idf_flags(self) -> None:
        module = load_module()
        command = (
            r"xtensa-esp32s3-elf-g++ -DIDF_VER=\"v6.0.1\" "
            r'@"/tmp/arc/build/toolchain/cflags" "-DMESSAGE=hello world" -Ifoo\ bar -c main/app_main.cpp'
        )

        self.assertEqual(module.split_compile_command(command), shlex.split(command))

    def test_fast_command_split_uses_whitespace_path_for_idf_response_files(self) -> None:
        module = load_module()
        command = r'g++ -DARC_TARGET_NAME=\"esp32s3\" @"/tmp/arc/build/toolchain/cflags" -c main.cpp'

        self.assertTrue(module.fast_whitespace_split_safe(command))
        self.assertEqual(module.split_compile_command(command), shlex.split(command))

    def test_normalized_entry_converts_command_once(self) -> None:
        module = load_module()
        entry = {
            "directory": str(ROOT),
            "command": r'g++ "-DNAME=arc fw" -Ibuild/config -c main/app_main.cpp',
            "file": "main/app_main.cpp",
        }

        normalized = module.normalized_entry(entry)

        self.assertNotIn("command", normalized)
        self.assertEqual(normalized["arguments"], ["g++", "-DNAME=arc fw", "-Ibuild/config", "-c", "main/app_main.cpp"])

    def test_auto_validate_batch_size_keeps_changed_sets_parallel(self) -> None:
        module = load_module()

        self.assertEqual(module.auto_validate_batch_size(None, 4), 1)
        self.assertEqual(module.auto_validate_batch_size(None, 202), 8)
        self.assertEqual(module.auto_validate_batch_size(3, 202), 3)

    def test_feature_rich_rank_prefers_project_commands_over_vendor_commands(self) -> None:
        module = load_module()
        project_entry = {
            "directory": str(ROOT),
            "arguments": ["g++", "-Icomponents/arc/include", "-c", str(ROOT / "main" / "app_main.cpp")],
            "file": str(ROOT / "main" / "app_main.cpp"),
        }
        vendor_entry = {
            "directory": str(ROOT),
            "arguments": [
                "g++",
                "-Ione",
                "-Itwo",
                "-Ithree",
                "-c",
                str(ROOT / "examples" / "esp32s3" / "bench" / "managed_components" / "esp-dsp" / "dsp.cpp"),
            ],
            "file": str(ROOT / "examples" / "esp32s3" / "bench" / "managed_components" / "esp-dsp" / "dsp.cpp"),
        }

        project_rank = module.feature_rich_rank((0, project_entry, (ROOT / "components" / "arc" / "include",)))
        vendor_rank = module.feature_rich_rank((1, vendor_entry, (ROOT / "one", ROOT / "two", ROOT / "three")))

        self.assertLess(project_rank, vendor_rank)

    def test_validate_header_groups_batches_clean_headers(self) -> None:
        module = load_module()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            headers = [root / "one.hpp", root / "two.hpp"]
            base_entry = {"directory": str(root), "arguments": ["c++", "-c", "main.cpp"], "file": "main.cpp"}
            calls: list[tuple[Path, ...]] = []

            original_set = module.validate_header_set
            try:
                module.validate_header_set = lambda _entry, group, _extra, _timeout: calls.append(tuple(group)) or None

                failures = module.validate_header_groups(
                    [(header, module.command_for_file(base_entry, header)) for header in headers],
                    (),
                    1.0,
                    1,
                    8,
                )
            finally:
                module.validate_header_set = original_set

        self.assertEqual(failures, [])
        self.assertEqual(calls, [tuple(headers)])

    def test_validate_header_groups_isolates_failed_batches(self) -> None:
        module = load_module()
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            clean = root / "clean.hpp"
            broken = root / "broken.hpp"
            entry = {"directory": str(root), "arguments": ["c++", "-c", "main.cpp"], "file": "main.cpp"}

            original_set = module.validate_header_set
            original_command = module.validate_header_command
            try:
                module.validate_header_set = lambda _entry, _group, _extra, _timeout: "batch failed"
                module.validate_header_command = lambda _entry, header, _extra, _timeout: (
                    "fatal error: missing.hpp" if header == broken else None
                )

                failures = module.validate_header_groups([(clean, entry), (broken, entry)], (), 1.0, 1, 8)
            finally:
                module.validate_header_set = original_set
                module.validate_header_command = original_command

        self.assertEqual(failures, [f"{broken}: fatal error: missing.hpp"])


if __name__ == "__main__":
    unittest.main()
