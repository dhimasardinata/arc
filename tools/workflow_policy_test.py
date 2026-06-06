from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "workflow-policy-check.py"

spec = importlib.util.spec_from_file_location("workflow_policy_check", TOOL)
assert spec and spec.loader
workflow_policy_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(workflow_policy_check)


def write_workflow(root: Path, name: str, body: str) -> None:
    path = root / ".github" / "workflows" / name
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(body, encoding="utf-8")


class WorkflowPolicyTest(unittest.TestCase):
    def test_accepts_current_workflow_policies(self) -> None:
        evidence = workflow_policy_check.collect(ROOT)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["workflow_count"], 3)
        jobs = {(workflow["path"], job["id"]): job for workflow in evidence["workflows"] for job in workflow["jobs"]}
        for job in jobs.values():
            self.assertEqual(job["runs_on"], workflow_policy_check.RUNNER_IMAGE)
        self.assertEqual(
            jobs[(".github/workflows/pages.yml", "deploy")]["permissions"], {"id-token": "write", "pages": "write"}
        )
        steps = {
            (workflow["path"], job["id"], step["name"]): step
            for workflow in evidence["workflows"]
            for job in workflow["jobs"]
            for step in job["steps"]
        }
        self.assertEqual(
            steps[(".github/workflows/build.yml", "esp32s3", "Ensure format tools")]["ruff_install_spec"],
            workflow_policy_check.RUFF_SPEC,
        )
        self.assertEqual(
            steps[(".github/workflows/build.yml", "esp32s3", "Restore ccache")]["uses"],
            "actions/cache/restore@27d5ce7f107fe9357f9df03efb73ab90386fccae",
        )
        for step_name in ("Save ESP-IDF and tools cache", "Save ccache", "Save firmware build cache"):
            self.assertIn(
                "github.event_name == 'push'",
                steps[(".github/workflows/build.yml", "esp32s3", step_name)]["if"],
            )

    def test_rejects_pull_request_target(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  pull_request_target:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-24.04
    timeout-minutes: 10
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(".github/workflows/build.yml: pull_request_target is not allowed", evidence["problems"])

    def test_rejects_top_level_write_permissions(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  push:
permissions:
  contents: write
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-latest
    timeout-minutes: 10
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml: top-level permissions must be {'contents': 'read'}",
            evidence["problems"],
        )

    def test_rejects_missing_job_timeout(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  push:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-latest
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(".github/workflows/build.yml:esp32s3: job must define timeout-minutes", evidence["problems"])

    def test_rejects_mutable_runner_image(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  push:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-latest
    timeout-minutes: 10
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml:esp32s3: runs-on must pin ubuntu-24.04",
            evidence["problems"],
        )

    def test_rejects_unapproved_job_permissions(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "pages.yml",
                """name: pages
on:
  push:
permissions:
  contents: read
concurrency:
  group: pages
  cancel-in-progress: true
jobs:
  build:
    runs-on: ubuntu-latest
    timeout-minutes: 10
    permissions:
      contents: write
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(".github/workflows/pages.yml:build: job permissions must be {}", evidence["problems"])

    def test_rejects_github_expression_in_shell_run(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  pull_request:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-24.04
    timeout-minutes: 10
    steps:
      - name: unsafe shell
        run: echo "${{ github.event.pull_request.title }}"
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml:esp32s3:unsafe shell: run block must not interpolate GitHub expressions",
            evidence["problems"],
        )

    def test_rejects_unreviewed_arc_base_sha_expression(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  pull_request:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-latest
    timeout-minutes: 10
    steps:
      - name: plan
        env:
          ARC_BASE_SHA: ${{ github.head_ref }}
        run: |
          git fetch --no-tags --depth=1 origin "$ARC_BASE_SHA"
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml:esp32s3:plan: ARC_BASE_SHA expression must stay reviewed",
            evidence["problems"],
        )
        self.assertIn(
            ".github/workflows/build.yml:esp32s3:plan: ARC_BASE_SHA must be guarded as a 40-hex commit",
            evidence["problems"],
        )

    def test_accepts_guarded_arc_base_sha_expression(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  pull_request:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-24.04
    timeout-minutes: 10
    steps:
      - name: plan
        env:
          ARC_BASE_SHA: ${{ github.event.pull_request.base.sha || github.event.before }}
        run: |
          if [[ ! "$ARC_BASE_SHA" =~ ^[0-9a-fA-F]{40}$ ]]; then
            echo "base SHA is not a 40-hex commit"
          fi
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertTrue(evidence["ok"], evidence["problems"])

    def test_rejects_mutable_ruff_install(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  push:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-latest
    timeout-minutes: 10
    steps:
      - name: Ensure format tools
        run: python3 -m pip install 'ruff>=0.8,<1'
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml:esp32s3:Ensure format tools: Ruff install must pin ruff==0.15.16",
            evidence["problems"],
        )

    def test_rejects_combined_cache_action(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  push:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-24.04
    timeout-minutes: 10
    steps:
      - name: Cache ccache
        uses: actions/cache@27d5ce7f107fe9357f9df03efb73ab90386fccae
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml:esp32s3:Cache ccache: cache use must split restore and push-gated save",
            evidence["problems"],
        )

    def test_rejects_cache_save_without_push_gate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                "build.yml",
                """name: build
on:
  push:
permissions:
  contents: read
concurrency:
  group: build
  cancel-in-progress: true
jobs:
  esp32s3:
    runs-on: ubuntu-24.04
    timeout-minutes: 10
    steps:
      - name: Save ccache
        if: steps.firmware-plan.outputs.count != '0'
        uses: actions/cache/save@27d5ce7f107fe9357f9df03efb73ab90386fccae
""",
            )

            evidence = workflow_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml:esp32s3:Save ccache: cache save must be gated to push events",
            evidence["problems"],
        )


if __name__ == "__main__":
    unittest.main()
