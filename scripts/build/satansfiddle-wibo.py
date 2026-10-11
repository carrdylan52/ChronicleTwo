#!/usr/bin/env python3
"""Run one MWCC compilation through Satan's Fiddle with a private JSON profile.

    satansfiddle-wibo.py COMPILER [OPTIONS] -o OUTPUT SOURCE...

With several sources, one compiler run takes them all, in order, and OUTPUT is the
directory that receives <stem>.o for each; the profile keeps the rows of every one.

SATANSFIDDLE_IGNORE is an optional comma-separated list of profile kinds to leave out:
helper_masks (the translation-unit helper masks; each unit then keeps the masks the
unit before it in the same run left, zero for the first), expression_overrides (the
floating-point rows) and placement_new (the statement conversion rows). The default
floating-point initialisation always applies.
"""

import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import sys
import tempfile


ROOT = Path(__file__).resolve().parents[2]


SOURCE_SUFFIXES = (".c", ".cc", ".cp", ".cpp", ".cxx", ".c++")


def invocation(arguments):
    """Separate only the compiler, generated output option and trailing sources.

    The last argument is always a source; the ones before it are too while they are named
    like C or C++ files, as kz-mwcc --batch passes them.
    """
    if len(arguments) < 4:
        raise ValueError("expected COMPILER [OPTIONS] -o OBJECT SOURCE...")
    compiler, *options = arguments
    count = 1
    while count < len(options) and options[-1 - count].lower().endswith(SOURCE_SUFFIXES):
        count += 1
    options, sources = options[:-count], options[-count:]
    outputs = [index for index, value in enumerate(options) if value == "-o"]
    if len(outputs) != 1 or outputs[0] + 1 >= len(options):
        raise ValueError("expected exactly one -o OBJECT option")
    index = outputs[0]
    output = options[index + 1]
    del options[index:index + 2]
    return (Path(compiler).resolve(), options, Path(output).resolve(),
            [Path(source).resolve() for source in sources])


def unit_name(value):
    return value.replace("\\", "/").rsplit("/", 1)[-1]


def validate_profile_units(profile):
    """Reject unknown game source identities before discarding other units' rows."""
    source_directory = ROOT / "ps2/src"
    known_units = {
        path.name for suffix in ("*.c", "*.cpp")
        for path in source_directory.rglob(suffix) if path.is_file()
    }
    tables = [("translation_units", profile.get("translation_units", []), "name")]
    for section, names in (
        ("floating_point", ("expression_overrides", "literal_overrides")),
        ("placement_new", ("statement_conversions",)),
    ):
        block = profile.get(section, {})
        if not isinstance(block, dict):
            raise ValueError(f"{section} must be a JSON object")
        for name in names:
            tables.append((f"{section}.{name}", block.get(name, []), "translation_unit"))
    for location, rows, field in tables:
        if not isinstance(rows, list):
            raise ValueError(f"{location} must be a JSON array")
        for row in rows:
            if not isinstance(row, dict):
                raise ValueError(f"{location} rows must be JSON objects")
            name = row.get(field)
            if not isinstance(name, str) or unit_name(name) not in known_units:
                raise ValueError(f"unknown translation unit {name!r} in {location}")


IGNORABLE_KINDS = ("helper_masks", "expression_overrides", "placement_new")


def ignored_kinds():
    kinds = [kind for kind in os.environ.get("SATANSFIDDLE_IGNORE", "").split(",") if kind]
    unknown = sorted(set(kinds) - set(IGNORABLE_KINDS))
    if unknown:
        raise ValueError(f"unknown SATANSFIDDLE_IGNORE kinds: {', '.join(unknown)}")
    return set(kinds)


def drop_ignored(profile, kinds):
    """Remove the rows of the ignored kinds from a profile."""
    if "helper_masks" in kinds:
        profile["translation_units"] = []
        profile.pop("default_gpr_helper_mask", None)
        profile.pop("default_fpr_helper_mask", None)
        profile["inherit_helper_masks"] = True
    if "expression_overrides" in kinds:
        profile.get("floating_point", {}).pop("expression_overrides", None)
    if "placement_new" in kinds:
        profile.pop("placement_new", None)


def main(arguments):
    compiler, options, output, sources = invocation(arguments)
    binary = os.environ.get("SATANSFIDDLE", "satansfiddle")
    executable = shutil.which(binary)
    if executable is None:
        raise ValueError(
            f"Satan's Fiddle executable {binary!r} is unavailable; "
            "build Satan's Fiddle and set SATANSFIDDLE to its executable"
        )
    profile_path = Path(os.environ.get(
        "SATANSFIDDLE_CONFIG", ROOT / "ps2/config/pal/satansfiddle.json"
    ))
    profile = json.loads(profile_path.read_text())
    if not isinstance(profile, dict):
        raise ValueError("Satan's Fiddle profile must be a JSON object")
    validate_profile_units(profile)
    drop_ignored(profile, ignored_kinds())
    if len(sources) == 1:
        logical_unit = unit_name(os.environ.get("SATANSFIDDLE_TRANSLATION_UNIT", sources[0].name))
        units = {logical_unit}
        naming = ["--translation-unit", logical_unit]
        if output.is_dir():
            output = output / (sources[0].stem + ".o")
    else:
        if os.environ.get("SATANSFIDDLE_TRANSLATION_UNIT"):
            raise ValueError("SATANSFIDDLE_TRANSLATION_UNIT names a single source")
        units = {source.name for source in sources}
        naming = []

    # The caller supplies the complete compiler flags; retain argument boundaries.
    profile["compiler_path"] = str(compiler)
    profile["compiler_options"] = shlex.join(options)
    profile["translation_units"] = [
        row for row in profile.get("translation_units", [])
        if unit_name(row["name"]) in units
    ]
    floating_point = profile.get("floating_point", {})
    for key in ("expression_overrides", "literal_overrides"):
        if key in floating_point:
            floating_point[key] = [
                row for row in floating_point[key]
                if unit_name(row["translation_unit"]) in units
            ]
    placement_new = profile.get("placement_new", {})
    if "statement_conversions" in placement_new:
        placement_new["statement_conversions"] = [
            row for row in placement_new["statement_conversions"]
            if unit_name(row["translation_unit"]) in units
        ]

    # The compiler runs in a fresh process with a private configuration.
    with tempfile.TemporaryDirectory(prefix="chronicletwo-satansfiddle-") as directory:
        configuration = Path(directory) / "config.json"
        configuration.write_text(json.dumps(profile, indent=2) + "\n")
        return subprocess.run([
            executable, "--config", str(configuration), "--output", str(output),
            *naming, *map(str, sources),
        ]).returncode


if __name__ == "__main__":
    try:
        sys.exit(main(sys.argv[1:]))
    except (OSError, ValueError, KeyError, TypeError) as error:
        print(f"satansfiddle-wibo: {error}", file=sys.stderr)
        sys.exit(1)
