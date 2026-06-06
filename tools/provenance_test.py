from __future__ import annotations

import importlib.util
import json
import os
import subprocess
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "provenance.py"

spec = importlib.util.spec_from_file_location("provenance", TOOL)
assert spec is not None and spec.loader is not None
provenance = importlib.util.module_from_spec(spec)
spec.loader.exec_module(provenance)


class ProvenanceTest(unittest.TestCase):
    def test_collect_emits_in_toto_slsa_statement(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            root = Path(tmp)
            subject = root / "source-manifest.json"
            subject.write_text('{"ok": true}', encoding="utf-8")
            statement, problems, stats = provenance.collect(ROOT, [subject])

        self.assertEqual(problems, [])
        self.assertEqual(stats["subject_count"], 1)
        self.assertEqual(statement["_type"], "https://in-toto.io/Statement/v1")
        self.assertEqual(statement["predicateType"], "https://slsa.dev/provenance/v1")
        self.assertEqual(statement["subject"][0]["name"], provenance.relpath(subject, ROOT))
        self.assertRegex(statement["subject"][0]["digest"]["sha256"], r"^[0-9a-f]{64}$")
        resolved = statement["predicate"]["buildDefinition"]["resolvedDependencies"][0]
        self.assertEqual(resolved["name"], "arc-source")
        self.assertRegex(resolved["digest"]["gitCommit"], r"^[0-9a-f]{40}$")

    def test_rejects_missing_subject(self) -> None:
        statement, problems, stats = provenance.collect(ROOT, [ROOT / "missing-evidence.json"])

        self.assertEqual(statement["subject"], [])
        self.assertEqual(stats["problem_count"], 2)
        self.assertIn("missing subject file: missing-evidence.json", problems)
        self.assertIn("at least one provenance subject file is required", problems)

    def test_cli_writes_json_statement(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            root = Path(tmp)
            subject = root / "sbom.spdx.json"
            output = root / "provenance.intoto.json"
            subject.write_text('{"spdxVersion": "SPDX-2.3"}', encoding="utf-8")
            result = subprocess.run(
                [
                    str(TOOL),
                    "--format",
                    "json",
                    "--output",
                    str(output),
                    str(subject),
                ],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertEqual(result.stdout, "")
            payload = json.loads(output.read_text(encoding="utf-8"))
            self.assertEqual(payload["predicateType"], "https://slsa.dev/provenance/v1")
            self.assertEqual(payload["subject"][0]["name"], provenance.relpath(subject, ROOT))

    def test_github_actions_context_shapes_builder_and_invocation(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            subject = Path(tmp) / "evidence-index.json"
            subject.write_text("{}", encoding="utf-8")
            env = {
                "GITHUB_ACTIONS": "true",
                "GITHUB_SERVER_URL": "https://github.com",
                "GITHUB_REPOSITORY": "dhimasardinata/arc",
                "GITHUB_WORKFLOW": "build",
                "GITHUB_RUN_ID": "1234",
                "GITHUB_RUN_ATTEMPT": "2",
                "GITHUB_WORKFLOW_REF": "dhimasardinata/arc/.github/workflows/build.yml@refs/heads/main",
            }
            with patch.dict(os.environ, env, clear=False):
                statement, problems, _ = provenance.collect(ROOT, [subject])

        self.assertEqual(problems, [])
        run_details = statement["predicate"]["runDetails"]
        build_definition = statement["predicate"]["buildDefinition"]
        self.assertEqual(run_details["builder"]["id"], "https://github.com/dhimasardinata/arc/actions/workflows/build")
        self.assertEqual(
            run_details["metadata"]["invocationId"],
            "https://github.com/dhimasardinata/arc/actions/runs/1234/attempts/2",
        )
        self.assertEqual(
            build_definition["buildType"],
            "https://github.com/dhimasardinata/arc/.github/workflows/build.yml@refs/heads/main",
        )

    def test_cli_fails_for_missing_subject(self) -> None:
        result = subprocess.run(
            [str(TOOL), "missing-evidence.json"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("missing subject file: missing-evidence.json", result.stdout)
        self.assertIn("arc provenance failed", result.stderr)


if __name__ == "__main__":
    unittest.main()
