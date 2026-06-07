from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "dependabot-policy-check.py"

spec = importlib.util.spec_from_file_location("dependabot_policy_check", TOOL)
assert spec and spec.loader
dependabot_policy_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(dependabot_policy_check)


def write_dependabot(root: Path, body: str) -> None:
    path = root / ".github" / "dependabot.yml"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(body, encoding="utf-8")


def dependabot_body() -> str:
    return """version: 2
updates:
  - package-ecosystem: "github-actions"
    directory: "/"
    target-branch: "main"
    schedule:
      interval: "weekly"
      day: "monday"
      time: "04:00"
      timezone: "Asia/Jakarta"
    open-pull-requests-limit: 5
    groups:
      github-actions:
        patterns:
          - "*"

  - package-ecosystem: "npm"
    directory: "/"
    target-branch: "main"
    schedule:
      interval: "weekly"
      day: "monday"
      time: "04:30"
      timezone: "Asia/Jakarta"
    open-pull-requests-limit: 5
    groups:
      docs-dependencies:
        patterns:
          - "*"
"""


class DependabotPolicyTest(unittest.TestCase):
    def test_accepts_current_dependabot_policy(self) -> None:
        evidence = dependabot_policy_check.collect(ROOT)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["version"], "2")
        self.assertEqual(len(evidence["updates"]), 2)

    def test_accepts_synthetic_dependabot_policy(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_dependabot(root, dependabot_body())

            evidence = dependabot_policy_check.collect(root)

        self.assertTrue(evidence["ok"], evidence["problems"])

    def test_rejects_missing_npm_updates(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            body = dependabot_body().split('  - package-ecosystem: "npm"', 1)[0]
            write_dependabot(root, body)

            evidence = dependabot_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(".github/dependabot.yml: missing update entry for npm", evidence["problems"])

    def test_rejects_wrong_schedule(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_dependabot(root, dependabot_body().replace('time: "04:30"', 'time: "12:00"', 1))

            evidence = dependabot_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/dependabot.yml: npm time must be '04:30', got '12:00'",
            evidence["problems"],
        )

    def test_rejects_ungrouped_ecosystem(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_dependabot(root, dependabot_body().replace("      docs-dependencies:", "      misc:", 1))

            evidence = dependabot_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/dependabot.yml: npm group must be 'docs-dependencies', got 'misc'",
            evidence["problems"],
        )

    def test_rejects_missing_target_branch(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            write_dependabot(root, dependabot_body().replace('    target-branch: "main"\n', "", 1))

            evidence = dependabot_policy_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            ".github/dependabot.yml: github-actions target-branch must be 'main', got None",
            evidence["problems"],
        )


if __name__ == "__main__":
    unittest.main()
