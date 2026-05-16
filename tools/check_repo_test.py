from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class CheckRepoToolTest(unittest.TestCase):
    def test_docs_policy_scans_untracked_nonignored_files(self) -> None:
        text = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        self.assertIn("git ls-files --cached --others --exclude-standard", text)
        self.assertIn("mapfile -t docs < <(worktree_files README.md AGENTS.md docs .github examples)", text)

    def test_whitespace_policy_scans_untracked_nonignored_files(self) -> None:
        text = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        self.assertIn("mapfile -t whitespace_files < <(worktree_files)", text)
        self.assertIn("tracked or untracked non-ignored files contain trailing whitespace", text)

    def test_ci_guards_cover_strict_audit_and_header_validation(self) -> None:
        text = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        self.assertIn("go run tools/arc-audit.go -root . -all", text)
        self.assertIn("go run tools/clangd-compile-commands\\.go --validate-arc-headers", text)
        self.assertIn('workflow_step_before "name: Host benchmarks" "name: Build all"', text)


if __name__ == "__main__":
    unittest.main()
