from __future__ import annotations

import os
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class GoToolsTest(unittest.TestCase):
    def run_tool_help(self, *args: str) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env["PYTHONDONTWRITEBYTECODE"] = "1"
        return subprocess.run(
            ["go", "run", *args, "--help"],
            cwd=ROOT,
            env=env,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def test_go_entrypoints_compile_for_help(self) -> None:
        tools = [
            ("tools/arc-audit.go", "forbidden realtime tokens"),
            ("tools/arc-gen.go", "optional compile database"),
            ("tools/clangd-compile-commands.go", "clangd-compile-commands.py"),
            ("tools/dump-source.go", "output directory"),
            ("tools/bridge/main.go", "UDP listen address"),
        ]

        for source, help_text in tools:
            with self.subTest(source=source):
                result = self.run_tool_help(source)
                output = result.stdout + result.stderr
                self.assertEqual(result.returncode, 0, output)
                self.assertIn(help_text, output)

    def test_go_tools_avoid_trimmed_local_stdlib_imports(self) -> None:
        unsupported = {'"encoding/json"', '"regexp"'}
        for source in (ROOT / "tools").glob("*.go"):
            text = source.read_text(encoding="utf-8")
            used = sorted(token for token in unsupported if token in text)
            self.assertEqual(used, [], f"{source.relative_to(ROOT)} imports {used}")


if __name__ == "__main__":
    unittest.main()
