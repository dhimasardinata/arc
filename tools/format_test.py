from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class FormatToolTest(unittest.TestCase):
    def test_default_check_includes_untracked_sources(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            repo = Path(tmp)
            tools = repo / "tools"
            tools.mkdir()
            (tools / "format.sh").write_text(
                (ROOT / "tools" / "format.sh").read_text(encoding="utf-8"),
                encoding="utf-8",
            )
            (tools / "format.sh").chmod(0o755)
            (tools / "probe.go").write_text('package main\nfunc main(){println("probe")}\n', encoding="utf-8")
            subprocess.run(["git", "init"], cwd=repo, check=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)

            bin_dir = repo / "bin"
            bin_dir.mkdir()
            ruff = bin_dir / "ruff"
            ruff.write_text("#!/usr/bin/env sh\nexit 0\n", encoding="utf-8")
            ruff.chmod(0o755)

            env = os.environ.copy()
            env["PATH"] = f"{bin_dir}{os.pathsep}{env['PATH']}"
            result = subprocess.run(
                ["./tools/format.sh", "--check"],
                cwd=repo,
                env=env,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        output = result.stdout + result.stderr
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("tools/probe.go", output)


if __name__ == "__main__":
    unittest.main()
