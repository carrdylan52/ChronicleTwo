#!/usr/bin/env python3
"""Compile the game units in one MWCC run per flag set and compare each object with the build's.

    whole_program.py [--build-dir build/pal] [--out DIR] [--runner CMD] [--per-unit]
                     [--units a,b,...] [--separate a,b,...] [--compare-to DIR] [-v]

Run from the project root inside the container. Units come from <build>/main_o_files in
link order; their flags and sources come from the objdiff base compile commands in
<build>/build.ninja, without the dependency flags. Units sharing a flag set are compiled
by one `RUNNER COMPILER FLAGS -o OUT SOURCE...` invocation, in link order; `--per-unit`
gives every unit its own invocation instead, and `--separate` does so for the named units.
RUNNER defaults to plain wibo.

Each object is compared with <build>/objdiff/base/<unit>.o, or with the object of the
same name in --compare-to: section by section (type, flags, alignment, bytes), then the
relocations (offset, type, addend and target) and the symbol table. Compiler-generated
`@N` names are compared once as spelled and once with the number dropped, so a shifted
name counter shows up apart from code differences.
Prints one line per unit and a summary, and exits non-zero if any object differs.
"""

import argparse
import concurrent.futures
import os
import re
import shlex
import shutil
import struct
import subprocess
import sys
import tempfile
from collections import Counter
from pathlib import Path

COMPILER = "tools/compilers/mw/3.0-011126/mwccps2.exe"
LIB_INCLUDES = "ps2/include/std;ps2/include/sce"
DEPENDENCY_FLAGS = ("-MD", "-MMD")
GENERATED_NAME = re.compile(r"@\d+")

SHT_PROGBITS, SHT_SYMTAB, SHT_NOBITS, SHT_REL, SHT_RELA = 1, 2, 8, 9, 4


def unit_commands(build_dir):
    """Map each objdiff base object's stem to (flags, source) from build.ninja."""
    commands = {}
    for line in (build_dir / "build.ninja").read_text().splitlines():
        if "kz-mwcc --obj" not in line or "/objdiff/base/" not in line:
            continue
        words = shlex.split(line.strip().removeprefix("COMMAND = ").split(" && ")[1])
        obj = Path(words[words.index("--obj") + 1]).name
        rest = words[words.index("--") + 1:]
        flags = [word for word in rest[:-1] if word not in DEPENDENCY_FLAGS]
        commands[obj.removesuffix(".o")] = (flags, rest[-1])
    return commands


def link_order(build_dir, commands):
    """The game units of main_o_files that have a C/C++ compile, in link order."""
    units = []
    for line in (build_dir / "main_o_files").read_text().split():
        name = Path(line).name.removesuffix(".o")
        if name in commands:
            units.append(name)
    return units


def groups(units, commands, per_unit, separate=()):
    """Units sharing a flag set, each list in link order."""
    if per_unit:
        return [(commands[unit][0], [unit]) for unit in units]
    grouped = {}
    for unit in units:
        if unit in separate:
            grouped[unit] = [unit]
            continue
        grouped.setdefault(tuple(commands[unit][0]), []).append(unit)
    return [(commands[members[0]][0], members) for members in grouped.values()]


def compile_group(runner, flags, units, commands, out_dir):
    """Run the compiler once over the group's sources; return the objects by unit."""
    directory = Path(tempfile.mkdtemp(prefix="run.", dir=out_dir))
    sources = [commands[unit][1] for unit in units]
    stems = [Path(source).stem for source in sources]
    if len(set(stems)) != len(stems):
        raise ValueError("two sources in one run share a basename")
    env = dict(os.environ, MWCIncludes=LIB_INCLUDES)
    proc = subprocess.run([*runner, COMPILER, *flags, "-o", str(directory), *sources],
                          env=env, stdin=subprocess.DEVNULL, capture_output=True, text=True)
    if proc.returncode != 0:
        sys.stderr.write(proc.stdout + proc.stderr)
        raise RuntimeError("compiler failed for %s" % ", ".join(units))
    objects = {}
    for unit, stem in zip(units, stems):
        target = out_dir / (unit + ".o")
        os.replace(directory / (stem + ".o"), target)
        objects[unit] = target
    shutil.rmtree(directory)
    return objects


class Object:
    """The comparable content of a little-endian ELF32 relocatable object."""

    def __init__(self, path):
        data = path.read_bytes()
        shoff, = struct.unpack_from("<I", data, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", data, 0x2e)
        headers = [struct.unpack_from("<10I", data, shoff + i * shentsize) for i in range(shnum)]
        names = headers[shstrndx]
        self.sections = []
        for header in headers:
            name = cstring(data, names[4] + header[0])
            self.sections.append((name, header))
        symtab = next(i for i, (_, h) in enumerate(self.sections) if h[1] == SHT_SYMTAB)
        strtab = headers[headers[symtab][6]]
        self.symbols = []
        sym = headers[symtab]
        for offset in range(sym[4], sym[4] + sym[5], 16):
            st_name, value, size, info, other, shndx = struct.unpack_from("<IIIBBH", data, offset)
            name = cstring(data, strtab[4] + st_name)
            if not name and info & 0xf == 3 and shndx < len(self.sections):
                name = self.sections[shndx][0]
            self.symbols.append((name, value, size, info, other, shndx))
        self.data = data

    def contents(self, index):
        header = self.sections[index][1]
        if header[1] == SHT_NOBITS:
            return b""
        return self.data[header[4]:header[4] + header[5]]

    def relocations(self, index):
        header = self.sections[index][1]
        size = 12 if header[1] == SHT_RELA else 8
        rows = []
        for offset in range(header[4], header[4] + header[5], size):
            r_offset, r_info = struct.unpack_from("<II", self.data, offset)
            addend = struct.unpack_from("<i", self.data, offset + 8)[0] if size == 12 else 0
            rows.append((r_offset, r_info & 0xff, self.symbols[r_info >> 8][0], addend))
        return rows


def cstring(data, offset):
    return data[offset:data.index(b"\0", offset)].decode("latin-1")


def function_at(obj, index):
    """The first function symbol defined in a section, or its name."""
    for name, _, _, info, _, shndx in obj.symbols:
        if shndx == index and info & 0xf == 2:
            return name
    return obj.sections[index][0]


def compare(ours, theirs):
    """Differences between two objects, as (kind, detail) pairs."""
    differences = []
    if len(ours.sections) != len(theirs.sections):
        differences.append(("sections", "%d sections, expected %d"
                            % (len(ours.sections), len(theirs.sections))))
        return differences
    for index, ((name, header), (their_name, their_header)) in enumerate(
            zip(ours.sections, theirs.sections)):
        kind = name if name.startswith((".text", ".data", ".rodata", ".sdata", ".bss",
                                        ".sbss", ".rel", ".comment")) else "other"
        if header[1] in (SHT_REL, SHT_RELA):
            mine = ours.relocations(index)
            expected = theirs.relocations(index)
            if mine != expected:
                target = header[7]
                same_code = [row[:2] + (GENERATED_NAME.sub("@", row[2]),) + row[3:]
                             for row in mine] == \
                            [row[:2] + (GENERATED_NAME.sub("@", row[2]),) + row[3:]
                             for row in expected]
                differences.append(("names" if same_code else "relocations",
                                    function_at(theirs, target)))
            continue
        if header[1] in (SHT_SYMTAB, 3):
            continue
        if (name, header[1], header[2], header[5], header[8]) != \
                (their_name, their_header[1], their_header[2], their_header[5], their_header[8]) \
                or ours.contents(index) != theirs.contents(index):
            differences.append((kind.split(".", 2)[1] if kind != "other" else kind,
                                function_at(theirs, index)))
    mine = [(GENERATED_NAME.sub("@", s[0]),) + s[1:] for s in ours.symbols]
    expected = [(GENERATED_NAME.sub("@", s[0]),) + s[1:] for s in theirs.symbols]
    if mine != expected:
        differences.append(("symbols", "symbol table differs"))
    elif ours.symbols != theirs.symbols:
        differences.append(("names", "generated names differ"))
    return differences


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("--build-dir", type=Path, default=Path("build/pal"))
    parser.add_argument("--out", type=Path, help="keep the objects here (default: a temp dir)")
    parser.add_argument("--runner", default="wibo", help="compiler runner command (shlex)")
    parser.add_argument("--per-unit", action="store_true", help="one invocation per unit")
    parser.add_argument("--units", help="comma-separated units to compile (link order kept)")
    parser.add_argument("--separate", default="", help="comma-separated units compiled alone")
    parser.add_argument("--compare-to", type=Path, help="directory of objects to compare with")
    parser.add_argument("-v", "--verbose", action="store_true")
    args = parser.parse_args(argv)

    commands = unit_commands(args.build_dir)
    units = link_order(args.build_dir, commands)

    def unit_list(text):
        names = {name if name.endswith((".c", ".cpp")) else name + ".cpp"
                 for name in text.split(",") if name}
        unknown = names - set(units)
        if unknown:
            parser.error("unknown units: %s" % ", ".join(sorted(unknown)))
        return names

    if args.units:
        wanted = unit_list(args.units)
        units = [unit for unit in units if unit in wanted]
    separate = unit_list(args.separate)

    out_dir = args.out or Path(tempfile.mkdtemp(prefix="whole-program."))
    out_dir.mkdir(parents=True, exist_ok=True)
    runner = shlex.split(args.runner)
    work = groups(units, commands, args.per_unit, separate)
    for flags, members in work:
        if len(work) > 1 and not args.per_unit:
            print("run: %d units, flags %s" % (len(members), shlex.join(flags)))
    objects = {}
    with concurrent.futures.ThreadPoolExecutor(os.cpu_count() if args.per_unit else 1) as pool:
        for result in pool.map(lambda group: compile_group(runner, *group, commands, out_dir),
                               work):
            objects.update(result)

    reference = args.compare_to or args.build_dir / "objdiff/base"
    tally = Counter()
    failed = 0
    for unit in units:
        differences = compare(Object(objects[unit]), Object(reference / (unit + ".o")))
        kinds = Counter(kind for kind, _ in differences)
        tally.update(kinds)
        code = [d for d in differences if d[0] != "names"]
        if code:
            failed += 1
            status = "DIFF " + " ".join("%s=%d" % item for item in sorted(kinds.items()))
        elif differences:
            status = "OK (generated names differ)"
        else:
            status = "OK"
        print("%-28s %s" % (unit, status))
        if args.verbose:
            for kind, detail in code:
                print("    %-12s %s" % (kind, detail))
    print("%d of %d units differ; %s; objects in %s"
          % (failed, len(units), ", ".join("%s=%d" % i for i in sorted(tally.items())) or
             "no differences", out_dir))
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
