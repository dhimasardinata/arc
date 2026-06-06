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
        self.assertEqual(
            jobs[(".github/workflows/pages.yml", "deploy")]["permissions"], {"id-token": "write", "pages": "write"}
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
    runs-on: ubuntu-latest
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
    runs-on: ubuntu-latest
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
    runs-on: ubuntu-latest
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


if __name__ == "__main__":
    unittest.main()
