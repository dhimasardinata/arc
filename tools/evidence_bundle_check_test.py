from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "evidence-bundle-check.py"

spec = importlib.util.spec_from_file_location("evidence_bundle_check", TOOL)
assert spec is not None and spec.loader is not None
evidence_bundle_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(evidence_bundle_check)


def write_json(path: Path, payload: object) -> None:
    path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")


def rel(path: Path) -> str:
    return path.resolve().relative_to(ROOT).as_posix()


def file_record(path: Path) -> dict[str, object]:
    return {
        "path": rel(path),
        "size": path.stat().st_size,
        "sha256": evidence_bundle_check.sha256(path),
        "json": {
            "valid": True,
            "ok": None,
            "problem_count": None,
        },
    }


def make_bundle(base: Path, *, commit: str = "a" * 40, release_commit: str | None = None) -> Path:
    base.mkdir(parents=True, exist_ok=True)
    payloads: dict[str, object] = {
        "source-manifest.json": {"commit": commit, "dirty": False, "ok": True, "problems": []},
        "third-party-manifest.json": {"ok": True, "problems": []},
        "safety-case.json": {"ok": True, "problems": []},
        "release-evidence.json": {"commit": release_commit or commit, "dirty": False, "status": []},
        "workflow-pins.json": {"ok": True, "problems": []},
        "workflow-policy.json": {"ok": True, "problems": []},
        "dependabot-policy.json": {"ok": True, "problems": []},
        "npm-lock.json": {"ok": True, "problems": []},
        "license-policy.json": {"ok": True, "problems": []},
        "secret-scan.json": {"ok": True, "problems": []},
        "sbom.spdx.json": {
            "spdxVersion": "SPDX-2.3",
            "packages": [{"name": "Arc", "versionInfo": commit}],
        },
    }
    for name, payload in payloads.items():
        write_json(base / name, payload)

    subjects = []
    for name in evidence_bundle_check.EXPECTED_PROVENANCE_SUBJECTS:
        path = base / name
        subjects.append({"name": rel(path), "digest": {"sha256": evidence_bundle_check.sha256(path)}})
    write_json(
        base / "provenance.intoto.json",
        {
            "_type": "https://in-toto.io/Statement/v1",
            "predicateType": "https://slsa.dev/provenance/v1",
            "subject": subjects,
            "predicate": {
                "buildDefinition": {
                    "externalParameters": {"commit": commit},
                    "resolvedDependencies": [
                        {"name": "arc-source", "digest": {"gitCommit": commit}},
                    ],
                }
            },
        },
    )

    records = [file_record(base / name) for name in evidence_bundle_check.EXPECTED_INDEXED]
    write_json(
        base / "evidence-index.json",
        {
            "commit": commit,
            "ok": True,
            "problems": [],
            "files": records,
        },
    )
    return base


class EvidenceBundleCheckTest(unittest.TestCase):
    def test_accepts_coherent_bundle(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            evidence = evidence_bundle_check.collect(bundle)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["indexed_count"], len(evidence_bundle_check.EXPECTED_INDEXED))
        self.assertEqual(evidence["provenance_subject_count"], len(evidence_bundle_check.EXPECTED_PROVENANCE_SUBJECTS))

    def test_rejects_hash_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            (bundle / "npm-lock.json").write_text('{"ok": true, "changed": true}', encoding="utf-8")
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("evidence-index.json: npm-lock.json sha256 mismatch", evidence["problems"])
        self.assertIn("provenance.intoto.json: npm-lock.json subject sha256 mismatch", evidence["problems"])

    def test_rejects_missing_provenance_subject(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["subject"] = [
                item for item in provenance["subject"] if not item["name"].endswith("source-manifest.json")
            ]
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("provenance.intoto.json: missing required subject: source-manifest.json", evidence["problems"])

    def test_rejects_commit_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp), release_commit="b" * 40)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("release-evidence.json: commit must match evidence-index commit", evidence["problems"])

    def test_rejects_missing_provenance_dependency_commit(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["predicate"]["buildDefinition"]["resolvedDependencies"] = []
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "provenance.intoto.json resolved dependency: commit must match evidence-index commit",
            evidence["problems"],
        )

    def test_cli_fails_for_incomplete_bundle(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            (bundle / "sbom.spdx.json").unlink()
            result = subprocess.run(
                [str(TOOL), str(bundle)],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("missing evidence file: sbom.spdx.json", result.stdout)
        self.assertIn("arc evidence bundle check failed", result.stderr)


if __name__ == "__main__":
    unittest.main()
