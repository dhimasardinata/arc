from __future__ import annotations

import importlib.util
import json
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
TOOL = ROOT / "tools" / "firmware-evidence-check.py"

spec = importlib.util.spec_from_file_location("firmware_evidence_check", TOOL)
assert spec is not None and spec.loader is not None
firmware_evidence_check = importlib.util.module_from_spec(spec)
spec.loader.exec_module(firmware_evidence_check)


def write_json(path: Path, payload: object) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, sort_keys=True), encoding="utf-8")


def rel(path: Path) -> str:
    return path.resolve().relative_to(ROOT).as_posix()


def evidence_file_record(path: Path) -> dict[str, object]:
    return {
        "path": rel(path),
        "size": path.stat().st_size,
        "sha256": firmware_evidence_check.sha256(path),
        "json": {
            "valid": True,
            "ok": None,
            "problem_count": None,
        },
    }


def make_bundle(base: Path, *, commit: str = "b" * 40) -> Path:
    artifacts = base / "build"
    bundle = base / ".arc-artifacts"
    artifacts.mkdir(parents=True, exist_ok=True)
    bundle.mkdir(parents=True, exist_ok=True)
    app = artifacts / "app.bin"
    elf = artifacts / "app.elf"
    app.write_bytes(b"app")
    elf.write_bytes(b"elf")

    manifest = {
        "commit": commit,
        "branch": "main",
        "dirty": False,
        "status": [],
        "artifact_count": 2,
        "ok": True,
        "problems": [],
        "artifacts": [
            {
                "path": rel(app),
                "kind": "firmware-bin",
                "size": app.stat().st_size,
                "sha256": firmware_evidence_check.sha256(app),
            },
            {
                "path": rel(elf),
                "kind": "elf",
                "size": elf.stat().st_size,
                "sha256": firmware_evidence_check.sha256(elf),
            },
        ],
    }
    write_json(bundle / "firmware-manifest.json", manifest)

    subject = bundle / "firmware-manifest.json"
    write_json(
        bundle / "firmware-provenance.intoto.json",
        {
            "_type": "https://in-toto.io/Statement/v1",
            "predicateType": "https://slsa.dev/provenance/v1",
            "subject": [
                {
                    "name": rel(subject),
                    "digest": {
                        "sha256": firmware_evidence_check.sha256(subject),
                    },
                }
            ],
            "predicate": {
                "buildDefinition": {
                    "buildType": "local://arc/tools/provenance.py",
                    "externalParameters": {
                        "commit": commit,
                        "subjects": [rel(subject)],
                    },
                    "resolvedDependencies": [
                        {"name": "arc-source", "digest": {"gitCommit": commit}},
                    ],
                },
                "runDetails": {
                    "builder": {"id": "local://arc/tools/provenance.py"},
                    "metadata": {
                        "invocationId": "local:/tmp/arc:" + commit,
                        "startedOn": "2026-01-01T00:00:00+00:00",
                        "finishedOn": "2026-01-01T00:00:00+00:00",
                    },
                    "byproducts": [
                        {
                            "name": name,
                            "digest": {
                                "sha256": firmware_evidence_check.sha256(ROOT / name),
                            },
                        }
                        for name in firmware_evidence_check.EXPECTED_PROVENANCE_BYPRODUCTS
                    ],
                },
            },
        },
    )

    records = [
        evidence_file_record(bundle / "firmware-manifest.json"),
        evidence_file_record(bundle / "firmware-provenance.intoto.json"),
    ]
    write_json(
        bundle / "firmware-evidence-index.json",
        {
            "commit": commit,
            "branch": "main",
            "ok": True,
            "file_count": len(records),
            "files": records,
            "problems": [],
        },
    )
    return bundle


class FirmwareEvidenceCheckTest(unittest.TestCase):
    def test_accepts_coherent_firmware_bundle(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            evidence = firmware_evidence_check.collect(bundle)

        self.assertTrue(evidence["ok"], evidence["problems"])
        self.assertEqual(evidence["indexed_count"], 2)
        self.assertEqual(evidence["provenance_subject_count"], 1)
        self.assertEqual(
            evidence["provenance_byproduct_count"],
            len(firmware_evidence_check.EXPECTED_PROVENANCE_BYPRODUCTS),
        )
        self.assertEqual(evidence["firmware_artifact_count"], 2)

    def test_rejects_stale_firmware_artifact_hash(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            (Path(tmp) / "build" / "app.bin").write_bytes(b"changed")
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware-manifest.json: " + rel(Path(tmp) / "build" / "app.bin") + " size mismatch", evidence["problems"]
        )
        self.assertIn(
            "firmware-manifest.json: " + rel(Path(tmp) / "build" / "app.bin") + " sha256 mismatch", evidence["problems"]
        )

    def test_rejects_missing_provenance_subject(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "firmware-provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["subject"] = []
            write_json(bundle / "firmware-provenance.intoto.json", provenance)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware-provenance.intoto.json: missing required subject: firmware-manifest.json",
            evidence["problems"],
        )

    def test_rejects_missing_indexed_provenance(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            index = json.loads((bundle / "firmware-evidence-index.json").read_text(encoding="utf-8"))
            index["files"] = [
                item for item in index["files"] if not item["path"].endswith("firmware-provenance.intoto.json")
            ]
            write_json(bundle / "firmware-evidence-index.json", index)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware-evidence-index.json: missing required indexed evidence: firmware-provenance.intoto.json",
            evidence["problems"],
        )

    def test_rejects_incomplete_provenance_predicate(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "firmware-provenance.intoto.json").read_text(encoding="utf-8"))
            predicate = provenance["predicate"]
            predicate["buildDefinition"]["buildType"] = ""
            predicate["buildDefinition"]["externalParameters"]["subjects"] = []
            predicate["runDetails"]["builder"]["id"] = ""
            predicate["runDetails"]["metadata"]["invocationId"] = ""
            write_json(bundle / "firmware-provenance.intoto.json", provenance)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("firmware-provenance.intoto.json: buildType must be present", evidence["problems"])
        self.assertIn(
            "firmware-provenance.intoto.json: externalParameters subjects must match provenance subjects",
            evidence["problems"],
        )
        self.assertIn("firmware-provenance.intoto.json: builder id must be present", evidence["problems"])
        self.assertIn("firmware-provenance.intoto.json: metadata invocationId must be present", evidence["problems"])

    def test_rejects_invalid_provenance_metadata_time(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "firmware-provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["predicate"]["runDetails"]["metadata"]["startedOn"] = "2026-01-01T00:00:00"
            write_json(bundle / "firmware-provenance.intoto.json", provenance)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("firmware-provenance.intoto.json: metadata startedOn must include timezone", evidence["problems"])

    def test_rejects_reversed_provenance_metadata_time(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "firmware-provenance.intoto.json").read_text(encoding="utf-8"))
            metadata = provenance["predicate"]["runDetails"]["metadata"]
            metadata["startedOn"] = "2026-01-02T00:00:00+00:00"
            metadata["finishedOn"] = "2026-01-01T00:00:00+00:00"
            write_json(bundle / "firmware-provenance.intoto.json", provenance)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware-provenance.intoto.json: metadata finishedOn must not be before startedOn",
            evidence["problems"],
        )

    def test_rejects_stale_summary_counts(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            index = json.loads((bundle / "firmware-evidence-index.json").read_text(encoding="utf-8"))
            index["file_count"] = 99
            write_json(bundle / "firmware-evidence-index.json", index)
            manifest = json.loads((bundle / "firmware-manifest.json").read_text(encoding="utf-8"))
            manifest["artifact_count"] = 99
            write_json(bundle / "firmware-manifest.json", manifest)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("firmware-evidence-index.json: file_count must match files length", evidence["problems"])
        self.assertIn("firmware-manifest.json: artifact_count must match artifacts length", evidence["problems"])

    def test_rejects_branch_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            manifest = json.loads((bundle / "firmware-manifest.json").read_text(encoding="utf-8"))
            manifest["branch"] = "release"
            write_json(bundle / "firmware-manifest.json", manifest)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "firmware-manifest.json: branch must match firmware-evidence-index.json branch", evidence["problems"]
        )

    def test_rejects_index_size_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            index = json.loads((bundle / "firmware-evidence-index.json").read_text(encoding="utf-8"))
            index["files"][0]["size"] = 99
            name = Path(index["files"][0]["path"]).name
            write_json(bundle / "firmware-evidence-index.json", index)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(f"firmware-evidence-index.json: {name} size mismatch", evidence["problems"])

    def test_rejects_missing_index_json_state(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            index = json.loads((bundle / "firmware-evidence-index.json").read_text(encoding="utf-8"))
            index["files"][0].pop("json")
            name = Path(index["files"][0]["path"]).name
            write_json(bundle / "firmware-evidence-index.json", index)
            evidence = firmware_evidence_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(f"firmware-evidence-index.json: {name} JSON state must be an object", evidence["problems"])

    def test_cli_reports_missing_bundle_as_json(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            missing = Path(tmp) / ".arc-artifacts"
            result = subprocess.run(
                [str(TOOL), "--format", "json", str(missing)],
                cwd=ROOT,
                check=False,
                text=True,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
            )

        self.assertEqual(result.returncode, 1)
        payload = json.loads(result.stdout)
        self.assertFalse(payload["ok"])
        self.assertIn(f"artifact directory is missing: {missing.relative_to(ROOT).as_posix()}", payload["problems"])
        self.assertIn("arc firmware evidence check failed", result.stderr)


if __name__ == "__main__":
    unittest.main()
