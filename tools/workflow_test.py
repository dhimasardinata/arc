from __future__ import annotations

import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def line_of(text: str, needle: str) -> int:
    for number, line in enumerate(text.splitlines(), start=1):
        if needle in line:
            return number
    return 0


class WorkflowTest(unittest.TestCase):
    def test_build_workflow_uses_least_privilege_and_bounded_runs(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        self.assertIn("permissions:\n  contents: read", workflow)
        self.assertIn("concurrency:\n  group: build-${{ github.ref }}", workflow)
        self.assertIn("cancel-in-progress: ${{ github.event_name == 'pull_request' }}", workflow)
        self.assertIn("timeout-minutes: 90", workflow)

    def test_workflow_action_refs_have_policy_gate(self) -> None:
        check_repo = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        self.assertIn("./tools/workflow-pins-check.py --format json", check_repo)
        self.assertIn("workflow action pin check failed", check_repo)

    def test_changed_project_plan_runs_before_expensive_setup(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        plan = line_of(workflow, "name: Plan firmware builds")
        host_tools = line_of(workflow, "name: Ensure host tools")
        idf_cache = line_of(workflow, "name: Restore ESP-IDF and tools cache")

        self.assertGreater(plan, 0)
        self.assertGreater(host_tools, 0)
        self.assertGreater(idf_cache, 0)
        self.assertLess(plan, host_tools)
        self.assertLess(plan, idf_cache)

    def test_host_benchmarks_run_before_firmware_builds(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        host_benchmarks = line_of(workflow, "name: Host benchmarks")
        build_firmware = line_of(workflow, "name: Build firmware")

        self.assertGreater(host_benchmarks, 0)
        self.assertGreater(build_firmware, 0)
        self.assertLess(host_benchmarks, build_firmware)

    def test_repo_sanity_carries_strict_audit_before_firmware_builds(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")
        check_repo = (ROOT / "tools" / "check-repo.sh").read_text(encoding="utf-8")

        sanity = line_of(workflow, "name: Repo sanity")
        build_firmware = line_of(workflow, "name: Build firmware")

        self.assertIn("go run tools/arc-audit.go -root . -all", check_repo)
        self.assertGreater(sanity, 0)
        self.assertGreater(build_firmware, 0)
        self.assertLess(sanity, build_firmware)
        self.assertNotIn("name: Host realtime audit", workflow)

    def test_repository_evidence_bundle_is_uploaded(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        sanity = line_of(workflow, "name: Repo sanity")
        evidence = line_of(workflow, "name: Repository evidence bundle")
        upload = line_of(workflow, "name: Upload repository evidence")
        host_tests = line_of(workflow, "name: Host tests")

        self.assertGreater(sanity, 0)
        self.assertGreater(evidence, 0)
        self.assertGreater(upload, 0)
        self.assertGreater(host_tests, 0)
        self.assertLess(sanity, evidence)
        self.assertLess(evidence, upload)
        self.assertLess(evidence, host_tests)
        self.assertIn("name: arc-evidence", workflow)
        self.assertIn("./tools/evidence-index.py --format json --output .arc-artifacts/evidence-index.json", workflow)
        self.assertIn(".arc-artifacts/evidence-index.json", workflow)
        self.assertIn(".arc-artifacts/source-manifest.json", workflow)
        self.assertIn(".arc-artifacts/third-party-manifest.json", workflow)
        self.assertIn(".arc-artifacts/safety-case.json", workflow)
        self.assertIn(".arc-artifacts/release-evidence.json", workflow)
        self.assertIn("./tools/workflow-pins-check.py --format json > .arc-artifacts/workflow-pins.json", workflow)
        self.assertIn(".arc-artifacts/workflow-pins.json", workflow)
        self.assertIn("if-no-files-found: error", workflow)
        self.assertIn("retention-days: 30", workflow)

    def test_changed_project_plan_gates_firmware_builds(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        plan = line_of(workflow, "name: Plan firmware builds")
        build_firmware = line_of(workflow, "name: Build firmware")

        self.assertGreater(plan, 0)
        self.assertGreater(build_firmware, 0)
        self.assertLess(plan, build_firmware)
        self.assertIn("./tools/ci-build-plan.py --buildable", workflow)
        self.assertIn("cat .arc-build-projects", workflow)
        self.assertIn("steps.firmware-plan.outputs.count != '0'", workflow)

    def test_firmware_build_cache_is_selected_project_scoped(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        restore_cache = line_of(workflow, "name: Restore firmware build cache")
        build_firmware = line_of(workflow, "name: Build firmware")
        save_cache = line_of(workflow, "name: Save firmware build cache")

        self.assertGreater(restore_cache, 0)
        self.assertGreater(build_firmware, 0)
        self.assertGreater(save_cache, 0)
        self.assertLess(restore_cache, build_firmware)
        self.assertLess(build_firmware, save_cache)
        self.assertIn(".arc-build-cache", workflow)
        self.assertIn("ARC_BUILD_CACHE_MAX_PROJECTS", workflow)
        self.assertIn("cache_ready", workflow)

    def test_firmware_artifact_manifest_is_uploaded_with_binaries(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        build_firmware = line_of(workflow, "name: Build firmware")
        manifest = line_of(workflow, "name: Firmware artifact manifest")
        upload = line_of(workflow, "name: Upload binaries")

        self.assertGreater(build_firmware, 0)
        self.assertGreater(manifest, 0)
        self.assertGreater(upload, 0)
        self.assertLess(build_firmware, manifest)
        self.assertLess(manifest, upload)
        self.assertIn("./tools/firmware-manifest.py --format json --require-artifacts", workflow)
        self.assertIn("--output .arc-artifacts/firmware-evidence-index.json", workflow)
        self.assertIn(".arc-artifacts/firmware-evidence-index.json", workflow)
        self.assertIn("--output .arc-artifacts/firmware-manifest.json", workflow)
        self.assertIn(".arc-artifacts/firmware-manifest.json", workflow)
        self.assertIn("if-no-files-found: error", workflow)
        self.assertIn("retention-days: 30", workflow)

    def test_header_validation_uses_go_helper_with_public_header_gate(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "build.yml").read_text(encoding="utf-8")

        self.assertIn("go run tools/clangd-compile-commands.go --validate-arc-headers", workflow)
        self.assertIn("--changed-arc-headers", workflow)

    def test_pages_workflow_uses_official_node24_actions(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "pages.yml").read_text(encoding="utf-8")

        self.assertIn("npm ci", workflow)
        self.assertIn("npm run docs:build", workflow)
        self.assertIn("timeout-minutes: 20", workflow)
        self.assertIn("timeout-minutes: 10", workflow)
        self.assertIn("actions/setup-node@48b55a011bda9f5d6aeb4c2d9c7362e8dae4041e", workflow)
        self.assertIn("actions/configure-pages@45bfe0192ca1faeb007ade9deae92b16b8254a0d", workflow)
        self.assertIn("actions/upload-pages-artifact@fc324d3547104276b827a68afc52ff2a11cc49c9", workflow)
        self.assertIn("actions/deploy-pages@cd2ce8fcbc39b97be8ca5fce6e763baed58fa128", workflow)

    def test_codeql_workflow_scans_source_without_firmware_build(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "codeql.yml").read_text(encoding="utf-8")

        self.assertIn("security-events: write", workflow)
        self.assertIn("concurrency:\n  group: codeql-${{ github.ref }}", workflow)
        self.assertIn("cancel-in-progress: ${{ github.event_name == 'pull_request' }}", workflow)
        self.assertIn("language: [c-cpp, python]", workflow)
        self.assertIn("timeout-minutes: 30", workflow)
        self.assertIn("build-mode: none", workflow)
        self.assertIn("queries: +security-extended,security-and-quality", workflow)
        self.assertIn("github/codeql-action/init@dc73d59c2d7bd4f8194098a91219eeee6d8a1719", workflow)
        self.assertIn("github/codeql-action/analyze@dc73d59c2d7bd4f8194098a91219eeee6d8a1719", workflow)
        self.assertNotIn("idf.py build", workflow)

    def test_dependabot_covers_actions_and_docs_dependencies(self) -> None:
        config = (ROOT / ".github" / "dependabot.yml").read_text(encoding="utf-8")

        self.assertIn('package-ecosystem: "github-actions"', config)
        self.assertIn('package-ecosystem: "npm"', config)
        self.assertIn('directory: "/"', config)
        self.assertIn('interval: "weekly"', config)
        self.assertIn("docs-dependencies", config)


if __name__ == "__main__":
    unittest.main()
