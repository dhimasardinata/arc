from __future__ import annotations

import importlib.util
import tempfile
import textwrap
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "evidence-workflow-check.py"

spec = importlib.util.spec_from_file_location("evidence_workflow_check", TOOL)
assert spec and spec.loader
evidence_workflow_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evidence_workflow_check)


def write_workflow(root: Path, body: str) -> None:
    path = root / ".github" / "workflows" / "build.yml"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(textwrap.dedent(body).lstrip(), encoding="utf-8")


def workflow_body() -> str:
    return """
    name: build
    jobs:
      esp32s3:
        steps:
          - name: Repository evidence bundle
            shell: bash
            run: |
              mkdir -p .arc-artifacts
              ./tools/source-manifest.py --format json --require-clean > .arc-artifacts/source-manifest.json
              ./tools/third-party-manifest-check.py --format json > .arc-artifacts/third-party-manifest.json
              ./tools/safety-case-check.py --format json > .arc-artifacts/safety-case.json
              ./tools/release-evidence.py --format json --require-clean > .arc-artifacts/release-evidence.json
              ./tools/workflow-pins-check.py --format json > .arc-artifacts/workflow-pins.json
              ./tools/workflow-policy-check.py --format json > .arc-artifacts/workflow-policy.json
              ./tools/evidence-workflow-check.py --format json > .arc-artifacts/evidence-workflow.json
              ./tools/dependabot-policy-check.py --format json > .arc-artifacts/dependabot-policy.json
              ./tools/npm-lock-check.py --format json > .arc-artifacts/npm-lock.json
              ./tools/license-policy-check.py --format json > .arc-artifacts/license-policy.json
              ./tools/secret-scan-check.py --format json > .arc-artifacts/secret-scan.json
              ./tools/sbom.py --format json --output .arc-artifacts/sbom.spdx.json
              ./tools/provenance.py --format json --output .arc-artifacts/provenance.intoto.json \\
                .arc-artifacts/source-manifest.json \\
                .arc-artifacts/third-party-manifest.json \\
                .arc-artifacts/safety-case.json \\
                .arc-artifacts/release-evidence.json \\
                .arc-artifacts/workflow-pins.json \\
                .arc-artifacts/workflow-policy.json \\
                .arc-artifacts/evidence-workflow.json \\
                .arc-artifacts/dependabot-policy.json \\
                .arc-artifacts/npm-lock.json \\
                .arc-artifacts/license-policy.json \\
                .arc-artifacts/secret-scan.json \\
                .arc-artifacts/sbom.spdx.json
              ./tools/evidence-index.py --format json --output .arc-artifacts/evidence-index.json \\
                --require .arc-artifacts/source-manifest.json \\
                --require .arc-artifacts/third-party-manifest.json \\
                --require .arc-artifacts/safety-case.json \\
                --require .arc-artifacts/release-evidence.json \\
                --require .arc-artifacts/workflow-pins.json \\
                --require .arc-artifacts/workflow-policy.json \\
                --require .arc-artifacts/evidence-workflow.json \\
                --require .arc-artifacts/dependabot-policy.json \\
                --require .arc-artifacts/npm-lock.json \\
                --require .arc-artifacts/license-policy.json \\
                --require .arc-artifacts/secret-scan.json \\
                --require .arc-artifacts/sbom.spdx.json \\
                --require .arc-artifacts/provenance.intoto.json \\
                .arc-artifacts/source-manifest.json \\
                .arc-artifacts/third-party-manifest.json \\
                .arc-artifacts/safety-case.json \\
                .arc-artifacts/release-evidence.json \\
                .arc-artifacts/workflow-pins.json \\
                .arc-artifacts/workflow-policy.json \\
                .arc-artifacts/evidence-workflow.json \\
                .arc-artifacts/dependabot-policy.json \\
                .arc-artifacts/npm-lock.json \\
                .arc-artifacts/license-policy.json \\
                .arc-artifacts/secret-scan.json \\
                .arc-artifacts/sbom.spdx.json \\
                .arc-artifacts/provenance.intoto.json
              ./tools/evidence-bundle-check.py .arc-artifacts

          - name: Upload repository evidence
            uses: actions/upload-artifact@043fb46d1a93c77aae656e7c1c64a875d1fc6a0a
            with:
              name: arc-evidence
              path: |
                .arc-artifacts/evidence-index.json
                .arc-artifacts/source-manifest.json
                .arc-artifacts/third-party-manifest.json
                .arc-artifacts/safety-case.json
                .arc-artifacts/release-evidence.json
                .arc-artifacts/workflow-pins.json
                .arc-artifacts/workflow-policy.json
                .arc-artifacts/evidence-workflow.json
                .arc-artifacts/dependabot-policy.json
                .arc-artifacts/npm-lock.json
                .arc-artifacts/license-policy.json
                .arc-artifacts/secret-scan.json
                .arc-artifacts/sbom.spdx.json
                .arc-artifacts/provenance.intoto.json
              if-no-files-found: error
              include-hidden-files: true
              retention-days: 30

          - name: Firmware artifact manifest
            if: steps.firmware-plan.outputs.count != '0'
            shell: bash
            run: |
              ./tools/firmware-manifest.py --format json --require-artifacts --output .arc-artifacts/firmware-manifest.json
              ./tools/evidence-index.py --format json --output .arc-artifacts/firmware-evidence-index.json \\
                --require .arc-artifacts/firmware-manifest.json \\
                .arc-artifacts/firmware-manifest.json

          - name: Upload binaries
            if: steps.firmware-plan.outputs.count != '0'
            uses: actions/upload-artifact@043fb46d1a93c77aae656e7c1c64a875d1fc6a0a
            with:
              name: arc-binaries
              path: |
                build/*.bin
                build/*.elf
                examples/**/build/*.bin
                examples/**/build/*.elf
                .arc-artifacts/firmware-evidence-index.json
                .arc-artifacts/firmware-manifest.json
              if-no-files-found: error
              include-hidden-files: true
              retention-days: 30
    """


def move_step_before(body: str, step_name: str, before_name: str) -> str:
    lines = textwrap.dedent(body).lstrip().splitlines(keepends=True)

    def bounds(name: str) -> tuple[int, int]:
        marker = f"- name: {name}"
        start = next(index for index, line in enumerate(lines) if line.strip() == marker)
        indent = len(lines[start]) - len(lines[start].lstrip(" "))
        end = len(lines)
        for index in range(start + 1, len(lines)):
            child_indent = len(lines[index]) - len(lines[index].lstrip(" "))
            if child_indent == indent and lines[index].lstrip().startswith("- name: "):
                end = index
                break
        return start, end

    start, end = bounds(step_name)
    block = lines[start:end]
    del lines[start:end]
    before, _ = bounds(before_name)
    lines[before:before] = block
    return "".join(lines)


class EvidenceWorkflowCheckTest(unittest.TestCase):
    def test_accepts_current_workflow_contract(self) -> None:
        evidence = evidence_workflow_check.collect(ROOT)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["generated"], list(evidence_workflow_check.EXPECTED_GENERATED))
        self.assertEqual(evidence["uploaded"], list(evidence_workflow_check.EXPECTED_UPLOADED))
        self.assertEqual(evidence["firmware_generated"], list(evidence_workflow_check.EXPECTED_FIRMWARE_GENERATED))
        self.assertEqual(evidence["firmware_uploaded"], list(evidence_workflow_check.EXPECTED_FIRMWARE_UPLOADED))
        self.assertEqual(
            evidence["firmware_binary_patterns"],
            list(evidence_workflow_check.EXPECTED_FIRMWARE_BINARY_PATTERNS),
        )

    def test_accepts_synthetic_workflow_contract(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(root, workflow_body())

            evidence = evidence_workflow_check.collect(root)

        self.assertTrue(evidence["ok"], evidence["problems"])

    def test_rejects_missing_uploaded_artifact(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(root, workflow_body().replace("                .arc-artifacts/secret-scan.json\n", "", 1))

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "repository evidence upload paths: missing required artifact: secret-scan.json",
            evidence["problems"],
        )

    def test_rejects_missing_provenance_subject(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(root, workflow_body().replace("                .arc-artifacts/npm-lock.json \\\n", "", 1))

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "repository evidence provenance subjects: missing required artifact: npm-lock.json",
            evidence["problems"],
        )

    def test_rejects_missing_index_requirement(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                workflow_body().replace("                --require .arc-artifacts/license-policy.json \\\n", "", 1),
            )

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "repository evidence index requirements: missing required artifact: license-policy.json",
            evidence["problems"],
        )

    def test_rejects_bundle_check_before_index(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            body = workflow_body()
            body = body.replace("              ./tools/evidence-bundle-check.py .arc-artifacts\n\n", "\n", 1)
            body = body.replace(
                "              ./tools/evidence-index.py --format json --output .arc-artifacts/evidence-index.json",
                "              ./tools/evidence-bundle-check.py .arc-artifacts\n"
                "              ./tools/evidence-index.py --format json --output .arc-artifacts/evidence-index.json",
                1,
            )
            write_workflow(root, body)

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "repository evidence bundle check: must run after evidence-index generation",
            evidence["problems"],
        )

    def test_rejects_repository_upload_without_hidden_artifact_paths(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(root, workflow_body().replace("              include-hidden-files: true\n", "", 1))

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "repository evidence upload: must include hidden .arc-artifacts paths",
            evidence["problems"],
        )

    def test_rejects_repository_upload_before_bundle_generation(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root, move_step_before(workflow_body(), "Upload repository evidence", "Repository evidence bundle")
            )

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "repository evidence upload: must run after Repository evidence bundle step",
            evidence["problems"],
        )

    def test_rejects_missing_firmware_upload_artifact(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                workflow_body().replace(
                    "                .arc-artifacts/firmware-manifest.json\n              if-no-files-found: error\n",
                    "              if-no-files-found: error\n",
                    1,
                ),
            )

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware evidence upload paths: missing required artifact: firmware-manifest.json",
            evidence["problems"],
        )

    def test_rejects_missing_firmware_binary_pattern(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(root, workflow_body().replace("                examples/**/build/*.elf\n", "", 1))

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware binary upload patterns: missing required artifact: examples/**/build/*.elf",
            evidence["problems"],
        )

    def test_rejects_firmware_upload_without_build_gate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                workflow_body().replace(
                    "          - name: Upload binaries\n            if: steps.firmware-plan.outputs.count != '0'\n",
                    "          - name: Upload binaries\n",
                    1,
                ),
            )

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware evidence upload: must be gated on selected firmware builds",
            evidence["problems"],
        )

    def test_rejects_firmware_upload_without_hidden_artifact_paths(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(root, workflow_body().replace("              include-hidden-files: true\n", "", 2))

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware evidence upload: must include hidden .arc-artifacts paths",
            evidence["problems"],
        )

    def test_rejects_firmware_upload_before_manifest_generation(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(root, move_step_before(workflow_body(), "Upload binaries", "Firmware artifact manifest"))

            evidence = evidence_workflow_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware evidence upload: must run after Firmware artifact manifest step",
            evidence["problems"],
        )


if __name__ == "__main__":
    unittest.main()
