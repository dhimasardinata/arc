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

        self.assertIn("root SECURITY.md must define the GitHub-facing disclosure policy", text)
        self.assertIn("private vulnerability reporting", text)
        self.assertIn("CONTRIBUTING.md must define the repository contribution path", text)
        self.assertIn("CONTRIBUTING.md must cover validation, API naming, security", text)
        self.assertIn("RELEASE.md must define release evidence requirements", text)
        self.assertIn("RELEASE.md must cover repository gates, firmware evidence", text)
        self.assertIn("python3 tools/release-evidence.py --format json", text)
        self.assertIn("release evidence tool must stay executable", text)
        self.assertIn("THIRD_PARTY_NOTICES.md must define third-party notice handling", text)
        self.assertIn("THIRD_PARTY_NOTICES.md must cover firmware, docs", text)
        self.assertIn("docs/governance.md must expose repository governance controls", text)
        self.assertIn("docs/governance.md must link contribution, release, security", text)
        self.assertIn("pull request template must keep review evidence consistent", text)
        self.assertIn("Target hardware or host-only scope", text)
        self.assertIn(".github/ISSUE_TEMPLATE/bug_report.yml", text)
        self.assertIn("firmware issue template must request SDK version", text)
        self.assertIn("CODEOWNERS must route critical Arc changes to the maintainer", text)
        self.assertIn("CODEOWNERS must cover $owned_path", text)
        self.assertIn("go run tools/arc-audit.go -root . -all", text)
        self.assertIn("./tools/arc-prove.sh", text)
        self.assertIn("./tools/topology-check.py --quiet", text)
        self.assertIn("python3 tools/compile-fail-check.py", text)
        self.assertIn("./tools/use-after-move-check.sh", text)
        self.assertIn("./tools/safety-case-check.py", text)
        self.assertIn("./tools/profile-manifest-check.py", text)
        self.assertIn("go run tools/clangd-compile-commands\\.go --validate-arc-headers", text)
        self.assertIn('workflow_step_before "name: Plan firmware builds" "name: Ensure host tools"', text)
        self.assertIn(
            'workflow_step_before "name: Plan firmware builds" "name: Restore ESP-IDF and tools cache"',
            text,
        )
        self.assertIn('workflow_step_before "name: Host benchmarks" "name: Build firmware"', text)
        self.assertIn('workflow_step_before "name: Plan firmware builds" "name: Build firmware"', text)
        self.assertIn("Restore firmware build cache", text)
        self.assertIn("\\./tools/ci-build-plan\\.py --buildable", text)

    def test_formal_gate_requires_role_model(self) -> None:
        text = (ROOT / "tools" / "arc-prove.sh").read_text(encoding="utf-8")

        self.assertIn("Roles", text)

    def test_formal_gate_links_spsc_model_to_cpp(self) -> None:
        text = (ROOT / "tools" / "arc-prove.sh").read_text(encoding="utf-8")

        self.assertIn("Spsc model push must advance head", text)
        self.assertIn("Spsc model pop must advance tail", text)
        self.assertIn("store_release(&head_, next);", text)
        self.assertIn("store_release(&tail_, increment(tail));", text)

    def test_repo_policy_runs_use_after_move_lint(self) -> None:
        text = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        self.assertIn("use-after-move lint gate", text)
        self.assertIn("tools/use-after-move-check.sh", text)

    def test_use_after_move_lint_probes_expected_support(self) -> None:
        text = (ROOT / "tools" / "use-after-move-check.sh").read_text(encoding="utf-8")

        self.assertIn("std_expected_probe.cpp", text)
        self.assertIn("clang-tidy cannot parse Arc C++23 std::expected headers", text)
        self.assertIn("no (template|member) named 'expected' in namespace 'std'", text)

    def test_repo_policy_keeps_hil_evidence_checker_executable(self) -> None:
        text = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        self.assertIn("tools/hil-evidence-check.py", text)

    def test_repo_policy_runs_topology_gate(self) -> None:
        text = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        self.assertIn("topology source gate", text)
        self.assertIn("tools/topology-check.py", text)

    def test_repo_policy_runs_compile_fail_contract_gate(self) -> None:
        text = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        self.assertIn("negative compile contract gate", text)
        self.assertIn("tools/compile-fail-check.py", text)

    def test_queue_static_asserts_include_actionable_arc_errors(self) -> None:
        text = "\n".join(
            [
                (ROOT / "components" / "arc" / "include" / "arc" / "spsc.hpp").read_text(encoding="utf-8"),
                (ROOT / "components" / "arc" / "include" / "arc" / "mpsc.hpp").read_text(encoding="utf-8"),
                (ROOT / "components" / "arc" / "include" / "arc" / "fanin.hpp").read_text(encoding="utf-8"),
                (ROOT / "components" / "arc" / "include" / "arc" / "rpc.hpp").read_text(encoding="utf-8"),
            ]
        )

        self.assertIn("[ARC ERROR] arc::Spsc payload must be trivially copyable", text)
        self.assertIn("[ARC ERROR] arc::Mpsc payload must be trivially copyable", text)
        self.assertIn("[ARC ERROR] arc::Fanin payload must be trivially copyable", text)
        self.assertIn("[ARC ERROR] arc::RpcLane request payload must be trivially copyable", text)
        self.assertIn("Action:", text)

    def test_new_static_asserts_include_actionable_arc_errors(self) -> None:
        text = "\n".join(
            [
                (ROOT / "components" / "arc" / "include" / "arc" / "matrix.hpp").read_text(encoding="utf-8"),
                (ROOT / "components" / "arc" / "include" / "arc" / "task.hpp").read_text(encoding="utf-8"),
            ]
        )

        self.assertIn("[ARC ERROR] arc::dsp::Matrix dimensions must be non-zero", text)
        self.assertIn("[ARC ERROR] arc::dsp::Lqr state count must be non-zero", text)
        self.assertIn("[ARC ERROR] arc::CoreLocal owner must be a concrete core", text)
        self.assertIn("[ARC ERROR] arc::CoreMsg source must be a concrete core", text)
        self.assertIn("Action:", text)

    def test_tool_tests_runner_parallelizes_test_files(self) -> None:
        text = (ROOT / "tools" / "tool-tests.sh").read_text(encoding="utf-8")

        self.assertIn("ARC_TOOL_TEST_JOBS", text)
        self.assertIn("ARC_AUDIT_TEST_BIN", text)
        self.assertIn("ARC_GEN_TEST_BIN", text)
        self.assertIn("find tools -maxdepth 1 -type f -name '*_test.py'", text)
        self.assertIn('PYTHONDONTWRITEBYTECODE=1 python3 "$test"', text)


if __name__ == "__main__":
    unittest.main()
