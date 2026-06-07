from __future__ import annotations

import importlib.util
import json
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "release-evidence.py"

spec = importlib.util.spec_from_file_location("release_evidence", TOOL)
assert spec is not None and spec.loader is not None
release_evidence = importlib.util.module_from_spec(spec)
spec.loader.exec_module(release_evidence)


class ReleaseEvidenceTest(unittest.TestCase):
    def test_collect_records_policy_files_and_commands(self) -> None:
        evidence = release_evidence.collect()

        self.assertRegex(evidence["commit"], r"^[0-9a-f]{40}$")
        self.assertTrue(evidence["ok"], evidence["problems"])
        paths = {record["path"]: record for record in evidence["policy_files"]}
        self.assertTrue(paths["README.md"]["sha256"])
        self.assertTrue(paths["RELEASE.md"]["exists"])
        self.assertTrue(paths["SECURITY.md"]["exists"])
        self.assertTrue(paths["docs/governance.md"]["sha256"])
        self.assertTrue(paths["docs/security.md"]["sha256"])
        self.assertTrue(paths["docs/safety-case.md"]["sha256"])
        self.assertTrue(paths["docs/licensing.md"]["sha256"])
        self.assertTrue(paths["docs/hil-test-jig.md"]["sha256"])
        self.assertTrue(paths["docs/benchmarking.md"]["sha256"])
        self.assertTrue(paths["THIRD_PARTY_NOTICES.md"]["sha256"])
        self.assertTrue(paths["THIRD_PARTY_MANIFEST.json"]["sha256"])
        self.assertTrue(paths[".github/dependabot.yml"]["sha256"])
        self.assertIn("./tools/check-repo.sh", evidence["required_commands"])
        self.assertIn("./tools/format.sh --check", evidence["required_commands"])
        self.assertIn("./tools/tool-tests.sh", evidence["required_commands"])
        self.assertIn("./tools/profile-manifest-check.py", evidence["required_commands"])
        self.assertIn("./tools/topology-check.py --quiet", evidence["required_commands"])
        self.assertIn("python3 tools/compile-fail-check.py", evidence["required_commands"])
        self.assertIn("./tools/arc-prove.sh", evidence["required_commands"])
        self.assertIn("./tools/use-after-move-check.sh", evidence["required_commands"])
        self.assertIn("go run tools/arc-audit.go -root . -all", evidence["required_commands"])
        self.assertIn("./tools/firmware-manifest.py --format json --require-artifacts", evidence["required_commands"])
        self.assertIn("./tools/firmware-evidence-check.py .arc-artifacts", evidence["required_commands"])
        self.assertIn("./tools/source-manifest.py --format json --require-clean", evidence["required_commands"])
        self.assertIn("./tools/safety-case-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/third-party-manifest-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/workflow-pins-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/workflow-policy-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/dependabot-policy-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/evidence-workflow-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/npm-lock-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/license-policy-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/secret-scan-check.py --format json", evidence["required_commands"])
        self.assertIn("./tools/evidence-index.py --format json", evidence["required_commands"])
        self.assertIn("./tools/evidence-bundle-check.py .arc-artifacts", evidence["required_commands"])
        self.assertIn("./tools/sbom.py --format json", evidence["required_commands"])
        self.assertIn("./tools/provenance.py --format json", evidence["required_commands"])
        self.assertIn("./tools/release-evidence.py --format json --require-clean", evidence["required_commands"])
        self.assertIn("idf.py build", evidence["required_commands"])
        records = {record["command"]: record for record in evidence["required_command_records"]}
        self.assertTrue(records["./tools/check-repo.sh"]["exists"])
        self.assertTrue(records["./tools/check-repo.sh"]["executable"])
        self.assertEqual(
            records["./tools/check-repo.sh"]["sha256"], release_evidence.sha256(ROOT / "tools/check-repo.sh")
        )
        self.assertTrue(records["./tools/format.sh --check"]["executable"])
        self.assertTrue(records["python3 tools/compile-fail-check.py"]["executable"])
        self.assertEqual(records["go run tools/arc-audit.go -root . -all"]["kind"], "repo_source")
        self.assertTrue(records["go run tools/arc-audit.go -root . -all"]["exists"])
        self.assertEqual(
            records["go run tools/arc-audit.go -root . -all"]["sha256"],
            release_evidence.sha256(ROOT / "tools/arc-audit.go"),
        )
        self.assertEqual(records["npm run docs:build"]["kind"], "npm_script")
        self.assertTrue(records["npm run docs:build"]["exists"])
        self.assertEqual(records["npm run docs:build"]["sha256"], release_evidence.sha256(ROOT / "package.json"))
        self.assertEqual(records["idf.py build"]["kind"], "external")
        self.assertEqual(evidence["problems"], [])

    def test_json_format_is_machine_readable(self) -> None:
        result = subprocess.run(
            [str(TOOL), "--format", "json"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        payload = json.loads(result.stdout)
        self.assertIn("commit", payload)
        self.assertIn("policy_files", payload)
        self.assertIn("required_commands", payload)
        self.assertIn("required_command_records", payload)
        self.assertTrue(payload["ok"])
        self.assertNotIn("arc release evidence", result.stdout)

    def test_report_format_groups_release_metadata(self) -> None:
        result = subprocess.run(
            [str(TOOL), "--format", "report"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("arc release evidence", result.stdout)
        self.assertIn("- policy files:", result.stdout)
        self.assertIn("  - RELEASE.md: ok sha256=", result.stdout)
        self.assertIn("- required commands:", result.stdout)
        self.assertIn("  - ./tools/check-repo.sh", result.stdout)
        self.assertIn("- command checks:", result.stdout)
        self.assertIn("  - ./tools/check-repo.sh: ok path=tools/check-repo.sh", result.stdout)
        self.assertIn("  - npm run docs:build: ok script=docs:build", result.stdout)
        self.assertIn(" sha256=", result.stdout)

    def test_command_records_validate_repo_tools_and_npm_scripts(self) -> None:
        missing_tool = release_evidence.command_record("./tools/no-such-tool.py")
        missing_source = release_evidence.command_record("go run tools/no-such-tool.go")
        missing_script = release_evidence.command_record(
            "npm run no-such-script", {"docs:build": "vitepress build docs"}
        )
        external = release_evidence.command_record("idf.py build")

        self.assertEqual(
            release_evidence.command_problem(missing_tool),
            "./tools/no-such-tool.py: repo tool path missing: tools/no-such-tool.py",
        )
        self.assertEqual(
            release_evidence.command_problem(missing_source),
            "go run tools/no-such-tool.go: repo source path missing: tools/no-such-tool.go",
        )
        self.assertEqual(
            release_evidence.command_problem(missing_script),
            "npm run no-such-script: npm script missing from package.json: no-such-script",
        )
        self.assertIsNone(release_evidence.command_problem(external))


if __name__ == "__main__":
    unittest.main()
