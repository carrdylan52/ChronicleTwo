"""Exercise the Satan's Fiddle adapter without requiring a private compiler or ROM."""

import importlib.util
import json
import os
from pathlib import Path
import shlex
import tempfile
import unittest
from unittest.mock import patch


spec = importlib.util.spec_from_file_location(
    "satansfiddle_wibo", Path(__file__).with_name("satansfiddle-wibo.py")
)
adapter = importlib.util.module_from_spec(spec)
spec.loader.exec_module(adapter)


class AdapterTests(unittest.TestCase):
    def test_preserves_compiler_option_boundaries(self):
        compiler, options, output, sources = adapter.invocation([
            "compiler.exe", "-c", "-pragma", "divbyzerocheck on", "-i",
            "include with spaces", "-o", "object with spaces.o", "source with spaces.cpp",
        ])
        self.assertEqual(options, ["-c", "-pragma", "divbyzerocheck on", "-i", "include with spaces"])
        self.assertEqual(shlex.split(shlex.join(options)), options)
        self.assertEqual(compiler.name, "compiler.exe")
        self.assertEqual(output.name, "object with spaces.o")
        self.assertEqual([source.name for source in sources], ["source with spaces.cpp"])

    def test_trailing_sources_end_the_options(self):
        _, options, output, sources = adapter.invocation([
            "compiler.exe", "-c", "-i", "ps2/include", "-o", "out", "a.cpp", "b.c",
        ])
        self.assertEqual(options, ["-c", "-i", "ps2/include"])
        self.assertEqual(output.name, "out")
        self.assertEqual([source.name for source in sources], ["a.cpp", "b.c"])

    def test_rejects_ambiguous_output(self):
        for arguments in (["cc", "-c", "source"],
                          ["cc", "-o", "first.o", "-o", "second.o", "source"],
                          ["cc", "-c", "-o", "source"]):
            with self.subTest(arguments=arguments), self.assertRaises(ValueError):
                adapter.invocation(arguments)

    def test_direct_compile_uses_original_unit_and_current_selectors(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            profile = directory / "profile.json"
            expression = lambda unit: {
                "translation_unit": unit, "function": "f__Fv", "value_type": "binary32",
                "value_bits": "0x3f800000", "evaluate_first": True,
            }
            placement = lambda unit: {
                "translation_unit": unit, "function": "f__FPv",
                "allocator": "__nw__FUiPv", "constructor": "__ct__5ThingFv",
                "conversion": "after_constructor_inline", "expected_matches": 1,
            }
            profile.write_text(json.dumps({
                "compiler_path": "unused", "compiler_options": "unused",
                "translation_units": [
                    {"name": "unit.cpp", "gpr_helper_mask": 48, "fpr_helper_mask": 4096},
                    {"name": "other.cpp", "gpr_helper_mask": 0, "fpr_helper_mask": 0},
                ],
                "floating_point": {
                    "expression_overrides": [expression("unit.cpp"), expression("other.cpp")],
                    "literal_overrides": [],
                },
                "placement_new": {
                    "statement_conversions": [placement("unit.cpp"), placement("other.cpp")],
                },
            }))
            source_root = directory / "checkout"
            (source_root / "ps2/src").mkdir(parents=True)
            for unit in ("unit.cpp", "other.cpp"):
                (source_root / "ps2/src" / unit).touch()
            observed = {}

            def run(command):
                observed["arguments"] = command
                observed["configuration"] = json.loads(Path(command[2]).read_text())
                return type("Completed", (), {"returncode": 7})()

            with patch.object(adapter, "ROOT", source_root), patch.dict(os.environ, {
                "SATANSFIDDLE": "fake-satansfiddle", "SATANSFIDDLE_CONFIG": str(profile),
                "SATANSFIDDLE_TRANSLATION_UNIT": "ps2\\src\\unit.cpp",
            }), patch.object(adapter.shutil, "which", return_value="/tool/satansfiddle"), \
                    patch.object(adapter.subprocess, "run", side_effect=run):
                status = adapter.main(["compiler.exe", "-c", "-pragma", "divbyzerocheck on",
                                       "-o", str(directory / "output.o"), str(directory / "unit.cpp")])
            self.assertEqual(status, 7)
            self.assertEqual(observed["arguments"][-3:-1], ["--translation-unit", "unit.cpp"])
            self.assertEqual(Path(observed["arguments"][-1]).name, "unit.cpp")
            selected = observed["configuration"]
            self.assertEqual(shlex.split(selected["compiler_options"]),
                             ["-c", "-pragma", "divbyzerocheck on"])
            self.assertEqual([row["name"] for row in selected["translation_units"]], ["unit.cpp"])
            self.assertEqual(selected["floating_point"]["expression_overrides"], [expression("unit.cpp")])
            self.assertEqual(selected["placement_new"]["statement_conversions"], [placement("unit.cpp")])
            self.assertFalse(Path(observed["arguments"][2]).exists())

    def run_profile(self, directory, profile, arguments, environment=None):
        """Run the adapter on a profile with a fake executable; return its command and profile."""
        source_root = directory / "checkout"
        (source_root / "ps2/src").mkdir(parents=True, exist_ok=True)
        for unit in ("unit.cpp", "other.cpp", "third.cpp"):
            (source_root / "ps2/src" / unit).touch()
        path = directory / "profile.json"
        path.write_text(json.dumps(profile))
        observed = {}

        def run(command):
            observed["arguments"] = command
            observed["configuration"] = json.loads(Path(command[2]).read_text())
            return type("Completed", (), {"returncode": 0})()

        with patch.object(adapter, "ROOT", source_root), patch.dict(os.environ, {
            "SATANSFIDDLE_CONFIG": str(path), **(environment or {}),
        }), patch.object(adapter.shutil, "which", return_value="/tool/satansfiddle"), \
                patch.object(adapter.subprocess, "run", side_effect=run):
            self.assertEqual(adapter.main(arguments), 0)
        return observed["arguments"], observed["configuration"]

    def test_several_sources_keep_the_rows_of_every_unit_in_one_run(self):
        expression = lambda unit: {
            "translation_unit": unit, "function": "f__Fv", "value_type": "binary32",
            "value_bits": "0x3f800000", "evaluate_first": True,
        }
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            sources = [str(directory / "unit.cpp"), str(directory / "other.cpp")]
            command, selected = self.run_profile(directory, {
                "compiler_path": "unused",
                "translation_units": [
                    {"name": unit, "gpr_helper_mask": 48, "fpr_helper_mask": 0}
                    for unit in ("unit.cpp", "other.cpp", "third.cpp")],
                "floating_point": {"expression_overrides": [
                    expression(unit) for unit in ("unit.cpp", "other.cpp", "third.cpp")]},
            }, ["compiler.exe", "-c", "-o", str(directory), *sources])
        self.assertEqual(command[3:], ["--output", str(directory), *sources])
        self.assertEqual([row["name"] for row in selected["translation_units"]],
                         ["unit.cpp", "other.cpp"])
        self.assertEqual(selected["floating_point"]["expression_overrides"],
                         [expression("unit.cpp"), expression("other.cpp")])

    def test_ignored_helper_masks_are_inherited_between_units(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            _, selected = self.run_profile(directory, {
                "compiler_path": "unused", "default_gpr_helper_mask": 0,
                "default_fpr_helper_mask": 0,
                "translation_units": [{"name": "unit.cpp", "gpr_helper_mask": 48,
                                       "fpr_helper_mask": 0}],
            }, ["compiler.exe", "-c", "-o", str(directory), str(directory / "unit.cpp"),
                str(directory / "other.cpp")], {"SATANSFIDDLE_IGNORE": "helper_masks"})
        self.assertEqual(selected["translation_units"], [])
        self.assertTrue(selected["inherit_helper_masks"])
        self.assertNotIn("default_gpr_helper_mask", selected)
        self.assertNotIn("default_fpr_helper_mask", selected)

    def test_one_source_into_a_directory_names_its_object(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            command, _ = self.run_profile(directory, {"compiler_path": "unused"}, [
                "compiler.exe", "-c", "-o", str(directory), str(directory / "unit.cpp")])
        self.assertEqual(command[3:5], ["--output", str(directory / "unit.o")])
        self.assertEqual(command[5:7], ["--translation-unit", "unit.cpp"])

    def test_a_logical_unit_name_needs_a_single_source(self):
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            with self.assertRaisesRegex(ValueError, "names a single source"):
                self.run_profile(directory, {"compiler_path": "unused"}, [
                    "compiler.exe", "-c", "-o", str(directory), str(directory / "unit.cpp"),
                    str(directory / "other.cpp")], {"SATANSFIDDLE_TRANSLATION_UNIT": "unit.cpp"})

    def test_rejects_unknown_units_before_filtering_or_running_compiler(self):
        tables = (
            ("translation_units", None, "name"),
            ("floating_point", "expression_overrides", "translation_unit"),
            ("floating_point", "literal_overrides", "translation_unit"),
            ("placement_new", "statement_conversions", "translation_unit"),
        )
        with tempfile.TemporaryDirectory() as directory:
            directory = Path(directory)
            source_root = directory / "checkout"
            (source_root / "ps2/src").mkdir(parents=True)
            (source_root / "ps2/src/unit.cpp").touch()
            profile_path = directory / "profile.json"
            output = directory / "output.o"
            output.write_bytes(b"existing object")
            for section, table, field in tables:
                with self.subTest(section=section, table=table):
                    rows = [{field: "unit.cpp"}, {field: "stale.cpp"}]
                    profile = {section: rows if table is None else {table: rows}}
                    profile_path.write_text(json.dumps(profile))
                    with patch.object(adapter, "ROOT", source_root), patch.dict(os.environ, {
                        "SATANSFIDDLE_CONFIG": str(profile_path),
                        "SATANSFIDDLE_TRANSLATION_UNIT": "unit.cpp",
                    }), patch.object(adapter.shutil, "which", return_value="/tool/satansfiddle"), \
                            patch.object(adapter.subprocess, "run") as run:
                        run.return_value.returncode = 0
                        with self.assertRaisesRegex(ValueError, "unknown translation unit 'stale.cpp'"):
                            adapter.main(["compiler.exe", "-c", "-o", str(output),
                                          str(directory / "unit.cpp")])
                        run.assert_not_called()
                    self.assertEqual(output.read_bytes(), b"existing object")

    def test_checked_in_profile_names_existing_game_sources(self):
        profile = json.loads((adapter.ROOT / "scripts/build/satansfiddle.json").read_text())
        adapter.validate_profile_units(profile)

    def test_missing_binary_has_setup_error(self):
        with patch.object(adapter.shutil, "which", return_value=None):
            with self.assertRaisesRegex(ValueError, "set SATANSFIDDLE"):
                adapter.main(["compiler", "-c", "-o", "object.o", "source.cpp"])


if __name__ == "__main__":
    unittest.main()
