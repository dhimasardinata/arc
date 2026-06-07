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


def source_payload(commit: str) -> dict[str, object]:
    output = subprocess.check_output(["git", "ls-files", "--stage", "-z"], cwd=ROOT)
    records: list[dict[str, object]] = []
    for raw in output.split(b"\0"):
        if not raw:
            continue
        meta, _, path = raw.partition(b"\t")
        parts = meta.decode().split()
        path_text = path.decode()
        if parts[0] not in evidence_bundle_check.REGULAR_FILE_MODES:
            continue
        source_path = ROOT / path_text
        records.append(
            {
                "path": path_text,
                "mode": parts[0],
                "git_object": parts[1],
                "size": source_path.stat().st_size,
                "sha256": evidence_bundle_check.sha256(source_path),
            }
        )
    records.sort(key=lambda item: str(item["path"]))
    return {
        "commit": commit,
        "branch": "main",
        "dirty": False,
        "status": [],
        "file_count": len(records),
        "tree_sha256": evidence_bundle_check.source_tree_sha256(records),
        "files": records,
        "ok": True,
        "problems": [],
    }


def release_payload(commit: str, release_commit: str | None = None) -> dict[str, object]:
    commands = [
        "./tools/check-repo.sh",
        "go run tools/arc-audit.go -root . -all",
        "npm run docs:build",
        "idf.py build",
    ]
    return {
        "commit": release_commit or commit,
        "branch": "main",
        "dirty": False,
        "ok": True,
        "problems": [],
        "status": [],
        "required_commands": commands,
        "required_command_records": [
            {
                "command": commands[0],
                "kind": "repo_tool",
                "path": "tools/check-repo.sh",
                "exists": True,
                "executable": True,
                "sha256": evidence_bundle_check.sha256(ROOT / "tools/check-repo.sh"),
            },
            {
                "command": commands[1],
                "kind": "repo_source",
                "path": "tools/arc-audit.go",
                "exists": True,
                "sha256": evidence_bundle_check.sha256(ROOT / "tools/arc-audit.go"),
            },
            {
                "command": commands[2],
                "kind": "npm_script",
                "path": "package.json",
                "script": "docs:build",
                "exists": True,
                "sha256": evidence_bundle_check.sha256(ROOT / "package.json"),
            },
            {
                "command": commands[3],
                "kind": "external",
                "tool": "idf.py",
            },
        ],
    }


def make_bundle(base: Path, *, commit: str = "a" * 40, release_commit: str | None = None) -> Path:
    base.mkdir(parents=True, exist_ok=True)
    payloads: dict[str, object] = {
        "source-manifest.json": source_payload(commit),
        "third-party-manifest.json": {"ok": True, "problems": []},
        "safety-case.json": {"ok": True, "problems": []},
        "release-evidence.json": release_payload(commit, release_commit),
        "workflow-pins.json": {"ok": True, "problems": []},
        "workflow-policy.json": {"ok": True, "problems": []},
        "evidence-workflow.json": {"ok": True, "problems": []},
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
                    "buildType": "local://arc/tools/provenance.py",
                    "externalParameters": {
                        "branch": "main",
                        "commit": commit,
                        "subjects": [item["name"] for item in subjects],
                    },
                    "internalParameters": {
                        "tool": "tools/provenance.py",
                        "predicateType": "https://slsa.dev/provenance/v1",
                    },
                    "resolvedDependencies": [
                        {
                            "name": evidence_bundle_check.EXPECTED_SOURCE_DEPENDENCY,
                            "uri": evidence_bundle_check.EXPECTED_SOURCE_URI,
                            "digest": {"gitCommit": commit},
                        },
                    ],
                },
                "runDetails": {
                    "builder": {
                        "id": "local://arc/tools/provenance.py",
                    },
                    "metadata": {
                        "invocationId": "local:/tmp/arc:" + commit,
                        "startedOn": "2026-01-01T00:00:00+00:00",
                        "finishedOn": "2026-01-01T00:00:00+00:00",
                    },
                    "byproducts": [
                        {
                            "name": name,
                            "digest": {
                                "sha256": evidence_bundle_check.sha256(ROOT / name),
                            },
                        }
                        for name in evidence_bundle_check.EXPECTED_PROVENANCE_BYPRODUCTS
                    ],
                },
            },
        },
    )

    records = [file_record(base / name) for name in evidence_bundle_check.EXPECTED_INDEXED]
    write_json(
        base / "evidence-index.json",
        {
            "commit": commit,
            "branch": "main",
            "ok": True,
            "file_count": len(records),
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
        self.assertEqual(evidence["source_file_count"], len(evidence_bundle_check.git_tracked_regular_files([])))
        self.assertEqual(evidence["provenance_subject_count"], len(evidence_bundle_check.EXPECTED_PROVENANCE_SUBJECTS))
        self.assertEqual(
            evidence["provenance_byproduct_count"], len(evidence_bundle_check.EXPECTED_PROVENANCE_BYPRODUCTS)
        )
        self.assertEqual(evidence["release_command_count"], 4)

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

    def test_rejects_malformed_commit_identity(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp), commit="not-a-full-git-sha")
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "evidence-index.json: commit must be a full lowercase 40-character hex git commit",
            evidence["problems"],
        )

    def test_rejects_branch_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            release = json.loads((bundle / "release-evidence.json").read_text(encoding="utf-8"))
            release["branch"] = "release"
            write_json(bundle / "release-evidence.json", release)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("release-evidence.json: branch must match evidence-index branch", evidence["problems"])

    def test_rejects_provenance_branch_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["predicate"]["buildDefinition"]["externalParameters"]["branch"] = "release"
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("provenance.intoto.json: branch must match evidence-index branch", evidence["problems"])

    def test_rejects_index_file_count_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            index = json.loads((bundle / "evidence-index.json").read_text(encoding="utf-8"))
            index["file_count"] = 99
            write_json(bundle / "evidence-index.json", index)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("evidence-index.json: file_count must match files length", evidence["problems"])

    def test_rejects_index_size_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            index = json.loads((bundle / "evidence-index.json").read_text(encoding="utf-8"))
            index["files"][0]["size"] = 99
            name = Path(index["files"][0]["path"]).name
            write_json(bundle / "evidence-index.json", index)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(f"evidence-index.json: {name} size mismatch", evidence["problems"])

    def test_rejects_missing_index_json_state(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            index = json.loads((bundle / "evidence-index.json").read_text(encoding="utf-8"))
            index["files"][0].pop("json")
            name = Path(index["files"][0]["path"]).name
            write_json(bundle / "evidence-index.json", index)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(f"evidence-index.json: {name} JSON state must be an object", evidence["problems"])

    def test_rejects_source_manifest_record_drift(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            source = json.loads((bundle / "source-manifest.json").read_text(encoding="utf-8"))
            source["files"][0]["sha256"] = "0" * 64
            write_json(bundle / "source-manifest.json", source)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "source-manifest.json: " + source["files"][0]["path"] + " sha256 mismatch",
            evidence["problems"],
        )
        self.assertIn("source-manifest.json: tree_sha256 mismatch", evidence["problems"])

    def test_rejects_missing_source_manifest_record(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            source = json.loads((bundle / "source-manifest.json").read_text(encoding="utf-8"))
            removed = source["files"].pop(0)
            source["file_count"] = len(source["files"])
            source["tree_sha256"] = evidence_bundle_check.source_tree_sha256(source["files"])
            write_json(bundle / "source-manifest.json", source)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "source-manifest.json: missing tracked file record: " + removed["path"],
            evidence["problems"],
        )

    def test_rejects_source_manifest_summary_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            source = json.loads((bundle / "source-manifest.json").read_text(encoding="utf-8"))
            source["file_count"] = 99
            source["tree_sha256"] = "0" * 64
            write_json(bundle / "source-manifest.json", source)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("source-manifest.json: file_count must match files length", evidence["problems"])
        self.assertIn("source-manifest.json: tree_sha256 mismatch", evidence["problems"])

    def test_rejects_release_command_record_problem(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            release = json.loads((bundle / "release-evidence.json").read_text(encoding="utf-8"))
            release["required_command_records"][0]["executable"] = False
            release["required_command_records"][1]["exists"] = False
            release["required_command_records"][2]["sha256"] = "0" * 64
            release["required_command_records"].append(
                {
                    "command": "./tools/extra.py",
                    "kind": "repo_tool",
                    "path": "tools/extra.py",
                    "exists": True,
                    "executable": True,
                    "sha256": "0" * 64,
                }
            )
            write_json(bundle / "release-evidence.json", release)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("release-evidence.json: ./tools/check-repo.sh repo tool must be executable", evidence["problems"])
        self.assertIn(
            "release-evidence.json: go run tools/arc-audit.go -root . -all repo source must exist",
            evidence["problems"],
        )
        self.assertIn("release-evidence.json: npm run docs:build npm script sha256 mismatch", evidence["problems"])
        self.assertIn("release-evidence.json: command records must match required_commands order", evidence["problems"])

    def test_rejects_incomplete_provenance_predicate(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            predicate = provenance["predicate"]
            predicate["buildDefinition"]["buildType"] = ""
            predicate["buildDefinition"]["externalParameters"]["subjects"] = []
            predicate["runDetails"]["builder"]["id"] = ""
            predicate["runDetails"]["metadata"]["invocationId"] = ""
            predicate["runDetails"]["byproducts"][0]["digest"]["sha256"] = "0" * 64
            bad_byproduct = predicate["runDetails"]["byproducts"][0]["name"]
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("provenance.intoto.json: buildType must be present", evidence["problems"])
        self.assertIn(
            "provenance.intoto.json: externalParameters subjects must match provenance subjects",
            evidence["problems"],
        )
        self.assertIn("provenance.intoto.json: builder id must be present", evidence["problems"])
        self.assertIn("provenance.intoto.json: metadata invocationId must be present", evidence["problems"])
        self.assertIn(f"provenance.intoto.json: {bad_byproduct} byproduct sha256 mismatch", evidence["problems"])

    def test_rejects_invalid_provenance_metadata_time(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["predicate"]["runDetails"]["metadata"]["startedOn"] = "2026-01-01T00:00:00"
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("provenance.intoto.json: metadata startedOn must include timezone", evidence["problems"])

    def test_rejects_reversed_provenance_metadata_time(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            metadata = provenance["predicate"]["runDetails"]["metadata"]
            metadata["startedOn"] = "2026-01-02T00:00:00+00:00"
            metadata["finishedOn"] = "2026-01-01T00:00:00+00:00"
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("provenance.intoto.json: metadata finishedOn must not be before startedOn", evidence["problems"])

    def test_rejects_missing_provenance_byproduct(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["predicate"]["runDetails"]["byproducts"] = [
                item for item in provenance["predicate"]["runDetails"]["byproducts"] if item["name"] != "tools/sbom.py"
            ]
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn("provenance.intoto.json: missing required byproduct: tools/sbom.py", evidence["problems"])

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

    def test_rejects_extra_provenance_dependency(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["predicate"]["buildDefinition"]["resolvedDependencies"].append(
                {
                    "name": "unreviewed-source",
                    "uri": "git+https://github.com/example/source.git",
                    "digest": {"gitCommit": "b" * 40},
                }
            )
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "provenance.intoto.json: resolvedDependencies must contain only arc-source",
            evidence["problems"],
        )

    def test_rejects_provenance_dependency_uri_mismatch(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            bundle = make_bundle(Path(tmp))
            provenance = json.loads((bundle / "provenance.intoto.json").read_text(encoding="utf-8"))
            provenance["predicate"]["buildDefinition"]["resolvedDependencies"][0]["uri"] = (
                "git+https://github.com/example/fork.git"
            )
            write_json(bundle / "provenance.intoto.json", provenance)
            evidence = evidence_bundle_check.collect(bundle)

        self.assertFalse(evidence["ok"])
        self.assertIn(
            "provenance.intoto.json: resolved dependency uri must be " + evidence_bundle_check.EXPECTED_SOURCE_URI,
            evidence["problems"],
        )

    def test_rejects_artifact_dir_outside_repository(self) -> None:
        with tempfile.TemporaryDirectory() as outside_tmp:
            evidence = evidence_bundle_check.collect(Path(outside_tmp))

        self.assertFalse(evidence["ok"])
        self.assertEqual(evidence["indexed_count"], 0)
        self.assertIn(f"artifact directory must stay inside repository: {outside_tmp}", evidence["problems"])

    def test_cli_reports_missing_artifact_dir_as_json(self) -> None:
        with tempfile.TemporaryDirectory(dir=ROOT) as tmp:
            missing = Path(tmp) / "missing-bundle"
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
        self.assertEqual(payload["artifact_dir"], str(missing.resolve()))
        self.assertIn(f"artifact directory is missing: {missing.relative_to(ROOT).as_posix()}", payload["problems"])
        self.assertIn("arc evidence bundle check failed", result.stderr)

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
