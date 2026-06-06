from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "secret-scan-check.py"

spec = importlib.util.spec_from_file_location("secret_scan_check", TOOL)
assert spec and spec.loader
secret_scan_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(secret_scan_check)


class SecretScanTest(unittest.TestCase):
    def test_accepts_plain_source_text(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "README.md").write_text("Use SECURITY.md for private reports.\n", encoding="utf-8")

            evidence = secret_scan_check.collect(root)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["finding_count"], 0)
        self.assertEqual(evidence["scanned_files"], 1)

    def test_rejects_private_key_material(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            marker = "-----BEGIN " + "PRIVATE KEY-----"
            (root / "key.pem").write_text(f"{marker}\nabc\n", encoding="utf-8")

            evidence = secret_scan_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn("key.pem:1: private-key matched private key material", evidence["problems"])

    def test_rejects_github_tokens(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            token = "ghp_" + ("A" * 36)
            (root / "token.txt").write_text(f"{token}\n", encoding="utf-8")

            evidence = secret_scan_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn("token.txt:1: github-token matched GitHub access token", evidence["problems"])

    def test_rejects_aws_access_keys(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            token = "AKIA" + ("A" * 16)
            (root / "aws.txt").write_text(f"{token}\n", encoding="utf-8")

            evidence = secret_scan_check.collect(root)

        self.assertFalse(evidence["ok"])
        self.assertIn("aws.txt:1: aws-access-key matched AWS access key id", evidence["problems"])

    def test_skips_dependency_directories(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            dependency = root / "node_modules" / "package" / "token.txt"
            dependency.parent.mkdir(parents=True)
            dependency.write_text("ghp_" + ("A" * 36), encoding="utf-8")

            evidence = secret_scan_check.collect(root)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["scanned_files"], 0)


if __name__ == "__main__":
    unittest.main()
