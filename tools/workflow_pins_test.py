from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "workflow-pins-check.py"
SHA = "de0fac2e4500dabe0009e67214ff5f5447ce83dd"

spec = importlib.util.spec_from_file_location("workflow_pins_check", TOOL)
assert spec and spec.loader
workflow_pins_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(workflow_pins_check)


def write_workflow(root: Path, body: str) -> None:
    path = root / ".github" / "workflows" / "build.yml"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(body, encoding="utf-8")


class WorkflowPinsTest(unittest.TestCase):
    def test_accepts_sha_pinned_remote_actions(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                f"""name: build
jobs:
  test:
    steps:
      - uses: actions/checkout@{SHA} # v6.0.2
        with:
          persist-credentials: false
      - name: codeql
        uses: github/codeql-action/init@{SHA} # v4
      - uses: ./.github/actions/local
""",
            )

            evidence = workflow_pins_check.collect(root)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["workflow_count"], 1)
        self.assertEqual(evidence["ref_count"], 3)
        self.assertEqual(evidence["refs"][0]["persist_credentials"], "false")

    def test_rejects_mutable_action_tags(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                """name: build
jobs:
  test:
    steps:
      - uses: actions/checkout@v6 # v6.0.2
        with:
          persist-credentials: false
""",
            )

            evidence = workflow_pins_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml:5: uses actions/checkout@v6 must pin to a full 40-character commit SHA",
            evidence["problems"],
        )

    def test_rejects_missing_version_comments(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                f"""name: build
jobs:
  test:
    steps:
      - uses: actions/checkout@{SHA}
        with:
          persist-credentials: false
""",
            )

            evidence = workflow_pins_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            f".github/workflows/build.yml:5: uses actions/checkout@{SHA} must include a trailing version comment",
            evidence["problems"],
        )

    def test_rejects_unpinned_docker_actions(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                """name: build
jobs:
  test:
    steps:
      - uses: docker://alpine:3.20
""",
            )

            evidence = workflow_pins_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/workflows/build.yml:5: uses docker://alpine:3.20 must pin docker actions by sha256 digest",
            evidence["problems"],
        )

    def test_rejects_checkout_with_persisted_credentials(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                f"""name: build
jobs:
  test:
    steps:
      - uses: actions/checkout@{SHA} # v6.0.2
        with:
          persist-credentials: true
""",
            )

            evidence = workflow_pins_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            f".github/workflows/build.yml:5: uses actions/checkout@{SHA} must set persist-credentials: false",
            evidence["problems"],
        )

    def test_rejects_checkout_without_persisted_credentials_policy(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_workflow(
                root,
                f"""name: build
jobs:
  test:
    steps:
      - uses: actions/checkout@{SHA} # v6.0.2
""",
            )

            evidence = workflow_pins_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            f".github/workflows/build.yml:5: uses actions/checkout@{SHA} must set persist-credentials: false",
            evidence["problems"],
        )


if __name__ == "__main__":
    unittest.main()
