from __future__ import annotations

import os
import subprocess
import tempfile
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


class ArcGenTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls) -> None:
        if env_bin := os.environ.get("ARC_GEN_TEST_BIN"):
            cls._bin_dir = None
            cls.arc_gen = Path(env_bin)
            return

        cls._bin_dir = tempfile.TemporaryDirectory()
        cls.arc_gen = Path(cls._bin_dir.name) / "arc-gen"
        result = subprocess.run(
            ["go", "build", "-o", str(cls.arc_gen), "tools/arc-gen.go"],
            cwd=ROOT,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )
        if result.returncode != 0:
            raise RuntimeError(result.stderr)

    @classmethod
    def tearDownClass(cls) -> None:
        if cls._bin_dir is not None:
            cls._bin_dir.cleanup()

    def run_arc_gen(self, root: Path, *args: str) -> subprocess.CompletedProcess[str]:
        env = os.environ.copy()
        env["PYTHONDONTWRITEBYTECODE"] = "1"
        return subprocess.run(
            [str(self.arc_gen), "-root", str(root), *args],
            cwd=ROOT,
            env=env,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
        )

    def test_compile_db_uses_directory_for_relative_source_files(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "src"
            src.mkdir()
            (src / "probe space.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct Probe { std::uint8_t id{}; };",
                        "ARC_PACK_REFLECT(Probe, &Probe::id);",
                    ]
                ),
                encoding="utf-8",
            )
            escaped_root = str(root).replace("/", "\\/")
            (root / "compile_commands.json").write_text(
                f'[{{"directory":"{escaped_root}","file":"src/probe\\u0020space.cpp","arguments":["c++"]}}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Probe u8:id", result.stdout)

    def test_compile_db_flag_accepts_absolute_paths(self) -> None:
        with tempfile.TemporaryDirectory() as root_tmp, tempfile.TemporaryDirectory() as src_tmp:
            root = Path(root_tmp)
            source_root = Path(src_tmp)
            src = source_root / "source"
            src.mkdir()
            (src / "external.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct External { std::uint16_t seq{}; };",
                        "ARC_PACK_REFLECT(External, &External::seq);",
                    ]
                ),
                encoding="utf-8",
            )
            compile_db = source_root / "compile_commands.json"
            compile_db.write_text(
                f'[{{"directory":"{source_root}","file":"source/external.cpp"}}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root, "-compile-db", str(compile_db))

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("External u16:seq", result.stdout)

    def test_compile_db_resolves_relative_directory_from_database_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            project = root / "project"
            src = project / "src"
            build = project / "build" / "nested"
            src.mkdir(parents=True)
            build.mkdir(parents=True)
            (src / "relative.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct Relative { std::uint32_t seq{}; };",
                        "ARC_PACK_REFLECT(Relative, &Relative::seq);",
                    ]
                ),
                encoding="utf-8",
            )
            compile_db = build / "compile_commands.json"
            compile_db.write_text(
                '[{"directory":"../..","file":"src/relative.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root, "-compile-db", str(compile_db))

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Relative u32:seq", result.stdout)

    def test_compile_db_resolves_relative_file_without_directory_from_database_path(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            db_dir = root / "db"
            db_dir.mkdir()
            (db_dir / "solo.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct Solo { std::uint64_t seq{}; };",
                        "ARC_PACK_REFLECT(Solo, &Solo::seq);",
                    ]
                ),
                encoding="utf-8",
            )
            compile_db = db_dir / "compile_commands.json"
            compile_db.write_text('[{"file":"solo.cpp"}]\n', encoding="utf-8")

            result = self.run_arc_gen(root, "-compile-db", str(compile_db))

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Solo u64:seq", result.stdout)

    def test_compile_db_skips_generated_relative_source_paths(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            for rel, name in [
                ("build/generated.cpp", "BuildGenerated"),
                ("build-cache/generated.cpp", "BuildCacheGenerated"),
                ("managed_components/generated.cpp", "ManagedGenerated"),
            ]:
                path = root / rel
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(
                    "\n".join(
                        [
                            "#include <cstdint>",
                            f"struct {name} {{ std::uint8_t id{{}}; }};",
                            f"ARC_PACK_REFLECT({name}, &{name}::id);",
                        ]
                    ),
                    encoding="utf-8",
                )
            (root / "compile_commands.json").write_text(
                "\n".join(
                    [
                        "[",
                        '{"directory":".","file":"build/generated.cpp"},',
                        '{"directory":".","file":"build-cache/generated.cpp"},',
                        '{"directory":".","file":"managed_components/generated.cpp"}',
                        "]",
                    ]
                ),
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertNotIn("Generated", result.stdout)

    def test_compile_db_keeps_source_files_with_build_prefixed_basenames(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "src"
            src.mkdir()
            (src / "build-helper.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct BuildHelper { std::uint8_t id{}; };",
                        "ARC_PACK_REFLECT(BuildHelper, &BuildHelper::id);",
                    ]
                ),
                encoding="utf-8",
            )
            (root / "compile_commands.json").write_text(
                '[{"directory":".","file":"src/build-helper.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("BuildHelper u8:id", result.stdout)

    def test_ignores_reflect_markers_inside_comments_and_strings(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "src"
            src.mkdir()
            (src / "comments.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct Live { std::uint8_t id{}; };",
                        "// ARC_PACK_REFLECT(Commented, &Commented::id);",
                        "/* ARC_PACK_REFLECT(Blocked, &Blocked::id); */",
                        'const char* text = "ARC_PACK_REFLECT(Stringed, &Stringed::id);";',
                        "ARC_PACK_REFLECT(Live, &Live::id);",
                    ]
                ),
                encoding="utf-8",
            )
            (root / "compile_commands.json").write_text(
                '[{"directory":".","file":"src/comments.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Live u8:id", result.stdout)
        self.assertNotIn("Commented", result.stdout)
        self.assertNotIn("Blocked", result.stdout)
        self.assertNotIn("Stringed", result.stdout)

    def test_writes_typescript_and_go_schema_outputs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "src"
            out = root / "out"
            src.mkdir()
            (src / "telemetry.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct Telemetry { std::uint16_t seq{}; float temp{}; };",
                        "ARC_PACK_REFLECT(Telemetry, &Telemetry::seq, &Telemetry::temp);",
                    ]
                ),
                encoding="utf-8",
            )
            (root / "compile_commands.json").write_text(
                '[{"directory":".","file":"src/telemetry.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(
                root,
                "-api-ts",
                str(out / "api.ts"),
                "-schema-go",
                str(out / "schema" / "schema.go"),
            )

            api = (out / "api.ts").read_text(encoding="utf-8")
            schema = (out / "schema" / "schema.go").read_text(encoding="utf-8")

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("export interface Telemetry", api)
        self.assertIn("seq: number;", api)
        self.assertIn('TelemetrySchema = "u16:seq,f32:temp"', api)
        self.assertIn("package schema", schema)
        self.assertIn('Name: "Telemetry"', schema)
        self.assertIn('{Name: "seq", Kind: "u16"}', schema)
        self.assertIn('{Name: "temp", Kind: "f32"}', schema)

    def test_struct_fields_from_multi_member_declaration(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "src"
            src.mkdir()
            (src / "multi.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct Multi { std::uint8_t a{}, b{}; std::uint16_t c{}, d{}; };",
                        "ARC_PACK_REFLECT(Multi, &Multi::b, &Multi::d);",
                    ]
                ),
                encoding="utf-8",
            )
            (root / "compile_commands.json").write_text(
                '[{"directory":".","file":"src/multi.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Multi u8:b,u16:d", result.stdout)

    def test_reflect_missing_field_fails_instead_of_raw_schema(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "src"
            src.mkdir()
            (src / "missing.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct Missing { std::uint8_t actual{}; };",
                        "ARC_PACK_REFLECT(Missing, &Missing::typo);",
                    ]
                ),
                encoding="utf-8",
            )
            (root / "compile_commands.json").write_text(
                '[{"directory":".","file":"src/missing.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("reflected field Missing::typo not found", result.stderr)

    def test_compile_db_rejects_malformed_json(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "compile_commands.json").write_text("[{", encoding="utf-8")

            result = self.run_arc_gen(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("compile_commands.json", result.stderr)
        self.assertIn("expected string", result.stderr)

    def test_compile_db_rejects_invalid_unknown_literals(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "compile_commands.json").write_text(
                '[{"directory":"/tmp","file":"probe.cpp","output":tru}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("invalid JSON literal", result.stderr)

    def test_compile_db_rejects_non_json_string_escapes(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "compile_commands.json").write_text(
                '[{"directory":"/tmp","file":"probe\\x20.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("invalid string escape", result.stderr)

    def test_compile_db_decodes_unicode_surrogate_pairs(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            src = root / "src"
            src.mkdir()
            (src / f"probe{chr(0x1F680)}.cpp").write_text(
                "\n".join(
                    [
                        "#include <cstdint>",
                        "struct Rocket { std::uint8_t id{}; };",
                        "ARC_PACK_REFLECT(Rocket, &Rocket::id);",
                    ]
                ),
                encoding="utf-8",
            )
            (root / "compile_commands.json").write_text(
                f'[{{"directory":"{root}","file":"src/probe\\uD83D\\uDE80.cpp"}}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("Rocket u8:id", result.stdout)

    def test_compile_db_rejects_missing_low_unicode_surrogate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "compile_commands.json").write_text(
                '[{"directory":"/tmp","file":"probe\\uD83D.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("missing low unicode surrogate", result.stderr)

    def test_compile_db_rejects_invalid_low_unicode_surrogate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "compile_commands.json").write_text(
                '[{"directory":"/tmp","file":"probe\\uD83D\\u0041.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("invalid low unicode surrogate", result.stderr)

    def test_compile_db_rejects_lone_low_unicode_surrogate(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / "compile_commands.json").write_text(
                '[{"directory":"/tmp","file":"probe\\uDE80.cpp"}]\n',
                encoding="utf-8",
            )

            result = self.run_arc_gen(root)

        self.assertNotEqual(result.returncode, 0)
        self.assertIn("lone low unicode surrogate", result.stderr)


if __name__ == "__main__":
    unittest.main()
