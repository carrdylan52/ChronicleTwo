#!/usr/bin/env python3
"""Write the asm/data replacement ledger: one row per symbol the assembled and data objects of the PS2
link define, with whether the game reaches it and what replaces it on PC.

    ledger_reach.py [--objects build/pal/main_o_files] [--include ps2/include] [--out docs/port/ASM_LEDGER.tsv]

`live` is the closure over the objects' relocations: it starts at every relocation of the game's
objects (obj/<unit>.cpp.o) and follows each reached library symbol's own relocations, so a symbol
only unreachable library code refers to is not live. A relocation belongs to the symbol whose
extent holds its offset; splat symbols carry no size, so an extent runs to the next symbol of the
section.
"""

import argparse
import collections
import os
import re
import struct
import sys

R_MIPS_32, R_MIPS_26, R_MIPS_HI16, R_MIPS_LO16 = 2, 4, 5, 6

Symbol = collections.namedtuple("Symbol", "name value size type bind shndx")


class Elf:
    def __init__(self, path):
        with open(path, "rb") as f:
            self.data = f.read()
        if self.data[:4] != b"\x7fELF" or self.data[4] != 1 or self.data[5] != 1:
            sys.exit(f"{path}: not a little-endian ELF32 object")
        shoff, = struct.unpack_from("<I", self.data, 0x20)
        shentsize, shnum, shstrndx = struct.unpack_from("<HHH", self.data, 0x2E)
        self.sections = [struct.unpack_from("<IIIIIIIIII", self.data, shoff + i * shentsize) for i in range(shnum)]
        names = self.sections[shstrndx]
        self.names = [self.string(names[4], s[0]) for s in self.sections]
        self.symbols = []
        for index, section in enumerate(self.sections):
            if section[1] == 2:  # SHT_SYMTAB
                strtab = self.sections[section[6]][4]
                for offset in range(section[4], section[4] + section[5], 16):
                    name, value, size, info, _, shndx = struct.unpack_from("<IIIBBH", self.data, offset)
                    self.symbols.append(Symbol(self.string(strtab, name), value, size, info & 15, info >> 4, shndx))

    def string(self, base, offset):
        end = self.data.index(b"\0", base + offset)
        return self.data[base + offset:end].decode("latin-1")

    def relocations(self):
        """(section index, offset, type, symbol, addend) for every REL entry; the addend is only
        decoded for relocations against a section symbol, the only ones whose target it decides."""
        for section in self.sections:
            if section[1] != 9:  # SHT_REL
                continue
            target = section[7]
            base = self.sections[target][4]
            entries = [struct.unpack_from("<II", self.data, o) for o in range(section[4], section[4] + section[5], 8)]
            for i, (offset, info) in enumerate(entries):
                kind, symbol = info & 0xFF, self.symbols[info >> 8]
                addend = 0
                if symbol.type == 3:  # STT_SECTION
                    word, = struct.unpack_from("<I", self.data, base + offset)
                    if kind == R_MIPS_26:
                        addend = (word & 0x03FFFFFF) << 2
                    elif kind == R_MIPS_32:
                        addend = word
                    elif kind == R_MIPS_HI16:
                        low = next((o for o, n in entries[i + 1:] if n & 0xFF == R_MIPS_LO16), None)
                        low_word = struct.unpack_from("<I", self.data, base + low)[0] if low is not None else 0
                        addend = ((word & 0xFFFF) << 16) + ((low_word & 0xFFFF) ^ 0x8000) - 0x8000
                    elif kind == R_MIPS_LO16:
                        addend = ((word & 0xFFFF) ^ 0x8000) - 0x8000
                yield target, offset, kind, symbol, addend


def owners(elf):
    """Per section, the symbols that own extents in it, sorted, with each one's extent."""
    by_section = collections.defaultdict(list)
    for symbol in elf.symbols:
        if (symbol.name and symbol.type in (0, 1, 2) and 0 < symbol.shndx < 0xFF00 and
                not symbol.name.startswith(".L") and symbol.name != "MACRO_INC"):
            by_section[symbol.shndx].append(symbol)
    extents = {}
    for shndx, symbols in by_section.items():
        symbols.sort(key=lambda s: (s.value, s.name))
        size = elf.sections[shndx][5]
        starts = sorted({s.value for s in symbols})
        for symbol in symbols:
            later = [v for v in starts if v > symbol.value]
            end = symbol.value + symbol.size if symbol.size else (later[0] if later else size)
            extents[(shndx, symbol.name)] = (symbol.value, end)
    return by_section, extents


def owner_at(by_section, shndx, offset):
    best = None
    for symbol in by_section.get(shndx, []):
        if symbol.value <= offset:
            best = symbol
        else:
            break
    return best


def read_objects(path):
    with open(path) as f:
        return [p for p in f.read().split() if p]


GAME = re.compile(r"(?:^|/)obj/[a-z0-9_]+\.cpp\.o$")


def declared(include):
    """Every function name a game or SDK header declares, with the header that does."""
    names = {}
    for root, _, files in os.walk(include):
        for name in sorted(files):
            path = os.path.join(root, name)
            with open(path, encoding="utf-8", errors="replace") as f:
                text = re.sub(r"//[^\n]*|/\*.*?\*/", " ", f.read(), flags=re.S)
            for match in re.finditer(r"\b([A-Za-z_]\w*)\s*\(", text):
                names.setdefault(match[1], os.path.relpath(path, os.path.dirname(include)))
    return names


# The PC side of each library, by object. Most rows take their object's rule; the symbol tables after
# it name the exceptions.
OBJECT_RULES = [
    (r"lib/libc/", "host-libc"),
    (r"lib/libm/", "host-libc"),
    (r"lib/libgcc/", "clang-rt"),
    (r"lib/mwcc/fpcmp", "clang-rt"),
    (r"lib/mwcc/(CPlusLib|New|NMWException|MWException|ExceptionPS2|global_destructor_chain)", "clang-rt"),
    (r"lib/mwcc/mwinit", "kazarov-runtime"),
    (r"lib/sce/kernl/glue", "host-libc"),
    (r"lib/sce/libvu0", "sce-core+adapter"),
    (r"lib/sce/", "stub"),
    (r"crt0", "clang-rt"),
    (r"data/vutext|data/vudata", "game-override"),
]

KAZAROV_RUNTIME = {
    "fptosi": "RuntimeMw.FptosiTruncatesAndSaturates",
    "fptoui": "RuntimeMw.FptouiTakesNegativesToZeroAndSaturates",
    "exit": "RuntimeMw.ExitClosesTheCardsRunsHandlersAndReturnsTheStatus",
    "__assert": "RuntimeMw.AFailedAssertionEndsWithTheGamesStatus",
    "mwInit": "RuntimeMw.StaticConstructorsRunBeforeMain",
}

# The MW runtime routines the game calls by name with no Kazarov counterpart: port/src/runtime.cpp.
PORT_RUNTIME = {
    "__construct_new_array": "RuntimePort.ConstructNewArrayWritesMwccsHeader",
}

# No IOP and no non-coherent DMA on PC: these return success and do nothing, as Chronicle's adapters do.
NO_OP_ADAPTERS = {"sceSifInitRpc", "FlushCache", "iFlushCache", "SyncDCache", "iSyncDCache",
                  "InvalidDCache", "iInvalidDCache"}

# The DC2 libvu0 functions whose maths Kazarov sce_core/vu0_math.hpp already has.
VU0_CORE = {"sceVu0" + n for n in (
    "CopyVector CopyVectorXYZ FTOI0Vector FTOI4Vector ITOF0Vector ITOF4Vector ScaleVector ScaleVectorXYZ "
    "AddVector SubVector MulVector InterVector InterVectorXYZ DivVector DivVectorXYZ InnerProduct "
    "OuterProduct Normalize ApplyMatrix UnitMatrix CopyMatrix TransposeMatrix MulMatrix InversMatrix "
    "RotMatrixX RotMatrixY RotMatrixZ RotMatrix TransMatrix CameraMatrix NormalLightMatrix LightColorMatrix "
    "ViewScreenMatrix DropShadowMatrix ClipAll ClampVector ClipScreen ClipScreen3 RotTransPersN "
    "RotTransPers").split()}

# Where each stubbed SDK family's native replacement lands (the plan's work packages).
STUB_WP = [
    (r"^(scePad)", "T6 (sce_core/pad)"),
    (r"^(sceMc)", "T10 (sce_core/mc_store)"),
    (r"^(sceOpen|sceClose|sceRead|sceWrite|sceLseek|sceIoctl|sceGetstat|sceDopen|sceDread|sceDclose|sceMkdir|"
     r"sceRemove|sceFsReset)", "T5 (sce_core/host0)"),
    (r"^(sceCd)", "T5/T6"),
    (r"^(sceSd|sceSif|sceSifCallRpc)", "T6 (init) / T11 (sound RPC)"),
    (r"^(sceMpeg|sceIpu)", "T13e (game-override)"),
    (r"^(sceGs|sceDma|sceVif|sceGif|sceDev)", "T7-V4 (game-override)"),
    (r"^(sceMSIn)", "T11 (game-override)"),
    (r"^i?(Create|Start|Exit|Delete|Sleep|Wakeup|Suspend|Resume|RotateThread|ReleaseWait|GetThreadId|Refer|"
     r"Change|Signal|Wait|Poll|InitThread)", "T6 (ClockPump)"),
    (r"^(Add|Remove|i?Enable|i?Disable)(Intc|Dmac)", "T6 (ClockSetTickCallback)"),
]

def classify(obj, symbol, kind, live, headers, sdk_headers, lib_referrers_only):
    """(replacement, signature_checked, test, note)."""
    rule = next((r for pattern, r in OBJECT_RULES if re.search(pattern, obj)), None)
    if rule is None:
        sys.exit(f"{obj}: no replacement rule")
    header = headers.get(symbol)
    # gen_sce_stubs.py and the adapters take their prototypes from the SDK headers alone.
    if rule in ("stub", "sce-core+adapter"):
        header = sdk_headers.get(symbol)
    signature = ("header " + header) if header and kind == "func" else ("data" if kind == "data" else "internal")
    if symbol in KAZAROV_RUNTIME:
        return "kazarov-runtime", signature, KAZAROV_RUNTIME[symbol], ""
    if symbol in PORT_RUNTIME:
        return "port-runtime", signature, PORT_RUNTIME[symbol], ""
    if not live:
        return "dead", signature, "-", "no relocation chain from the game's objects reaches it"
    if rule == "sce-core+adapter":
        if lib_referrers_only:
            return "caller-replaced", signature, "-", "only libvu0 code refers to it"
        if kind == "data" or not header:
            return "game-override", signature, "-", "the game refers to it directly; its referrers are inventory rows"
        if symbol in VU0_CORE:
            return "sce-core+adapter", signature, "SceCoreVu0.*", ""
        return "adapter", signature, "-", "T3b: not in sce_core/vu0_math; adapter written from the libvu0 disassembly"
    if rule == "stub":
        if lib_referrers_only:
            return "caller-replaced", signature, "-", "only SDK code refers to it"
        if symbol in NO_OP_ADAPTERS and header:
            return "adapter", signature, "-", "no-op: nothing on PC to synchronise"
        if kind == "data" or not header:
            return "game-override", signature, "-", "the game refers to it directly; its referrers are inventory rows"
        wp = next((w for pattern, w in STUB_WP if re.search(pattern, symbol)), "T16b")
        return "stub", signature, "-", "native in " + wp
    if rule == "game-override":
        return "game-override", "data", "-", "readers are inventory rows (vu-data); VU1 programs replaced in V2-V4"
    if rule == "clang-rt":
        if "NMWException" in obj or "MWException" in obj or "ExceptionPS2" in obj:
            if not symbol.startswith("__nw") and lib_referrers_only:
                return "caller-replaced", signature, "-", ("MW exception runtime reached only through the MW "
                                                           "runtime; the port compiles with -fno-exceptions")
        if obj.endswith("crt0.s.o"):
            return "clang-rt", signature, "-", "host C runtime start-up"
        if "libgcc" in obj or "fpcmp" in obj:
            return "clang-rt", "compiler", "-", "MWCC-emitted helper; clang emits the operation itself"
        return "clang-rt", signature if header else "compiler", "-", "host C++ runtime"
    if rule == "host-libc":
        return "host-libc", signature if kind == "func" else "data", "-", ""
    return rule, signature, "-", ""


def main():
    parser = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    parser.add_argument("--root", default=os.getcwd())
    parser.add_argument("--objects", default="build/pal/main_o_files")
    parser.add_argument("--include", default="ps2/include")
    parser.add_argument("--out", default="docs/port/ASM_LEDGER.tsv")
    options = parser.parse_args()
    root = os.path.realpath(options.root)
    paths = read_objects(os.path.join(root, options.objects))
    headers = declared(os.path.join(root, options.include))
    sdk_headers = declared(os.path.join(root, options.include, "sce"))
    sdk_headers = {name: "include/sce/" + os.path.basename(path) for name, path in sdk_headers.items()}

    game, libs = [], []
    for path in paths:
        (game if GAME.search(path) else libs).append(path)
    elves = {p: Elf(os.path.join(root, p)) for p in paths}
    print(f"{len(game)} game objects, {len(libs)} library and data objects", file=sys.stderr)

    defined = {}
    rows = []
    layout = {}
    for path in libs:
        elf = elves[path]
        by_section, extents = owners(elf)
        layout[path] = by_section
        for shndx, symbols in by_section.items():
            section = elf.names[shndx]
            for symbol in symbols:
                start, end = extents[(shndx, symbol.name)]
                kind = "func" if "text" in section else "data"
                rows.append([path, symbol.name, kind, end - start, section])
                if symbol.bind != 0:
                    defined.setdefault(symbol.name, (path, shndx))

    def target_of(path, symbol, addend):
        """The (object, symbol) a relocation reaches, or None for a game or undefined target."""
        if symbol.type == 3:
            owner = owner_at(layout[path], symbol.shndx, addend) if path in layout else None
            return (path, owner.name) if owner else None
        if 0 < symbol.shndx < 0xFF00 and path not in layout:
            return None
        if 0 < symbol.shndx < 0xFF00:
            owner = owner_at(layout[path], symbol.shndx, symbol.value)
            if owner and (symbol.bind == 0 or symbol.name.startswith(".L")):
                return path, owner.name
            if owner:
                return path, symbol.name
        if symbol.name in defined:
            return defined[symbol.name][0], symbol.name
        return None

    referrers = collections.defaultdict(set)
    edges = collections.defaultdict(set)
    roots = set()
    for path in paths:
        elf = elves[path]
        is_game = path in game
        by_section = layout.get(path)
        if is_game:
            by_section, _ = owners(elf)
        unit = os.path.basename(path).removesuffix(".cpp.o")
        for shndx, offset, _, symbol, addend in elf.relocations():
            target = target_of(path, symbol, addend)
            if target is None:
                continue
            owner = owner_at(by_section, shndx, offset)
            source = owner.name if owner else elf.names[shndx]
            if is_game:
                roots.add(target)
                referrers[target].add((True, unit + ":" + source))
            elif (path, source) != target:
                edges[(path, source)].add(target)

    live = set()
    pending = list(roots)
    while pending:
        node = pending.pop()
        if node in live:
            continue
        live.add(node)
        pending += [t for t in edges.get(node, ()) if t not in live]

    def live_referrers(node):
        found = list(referrers.get(node, ()))
        for source, targets in edges.items():
            if node in targets and source in live:
                found.append((False, os.path.basename(source[0]).removesuffix(".s.o") + ":" + source[1]))
        return found

    out = []
    for path, symbol, kind, size, section in rows:
        node = (path, symbol)
        is_live = node in live
        found = live_referrers(node) if is_live else []
        game_refs = sorted({n for g, n in found if g})
        lib_refs = sorted({n for g, n in found if not g})
        replacement, signature, test, note = classify(path, symbol, kind, is_live, headers, sdk_headers,
                                                      bool(found) and not game_refs)
        shown = game_refs[:3] + lib_refs[:max(0, 3 - len(game_refs))]
        total = len(game_refs) + len(lib_refs)
        refs = ", ".join(shown) + (f" +{total - len(shown)}" if total > len(shown) else "") if total else "-"
        obj = re.sub(r"^.*?obj/", "", path)
        out.append([obj, symbol, kind, str(size), "yes" if is_live else "no", replacement, signature, test,
                    refs, note])

    lines = [
        "# The assembled and data objects of the PAL link (build/pal/main_o_files less the 149 units), one row per",
        "# symbol they define. Written by scripts/port/ledger_reach.py; do not edit by hand, change its rules.",
        "# live: reached by the relocation closure from the game's objects. replacement: host-libc, clang-rt (the",
        "# host compiler and C/C++ runtime), kazarov-runtime (tools/kazarov/port/runtime), port-runtime",
        "# (port/src/runtime.cpp), sce-core+adapter (port/src/sce over sce_core), adapter (port/src/sce alone),",
        "# stub (gen_sce_stubs.py), game-override (the game functions that refer to it are inventory rows and get",
        "# overridden), caller-replaced (only library code refers to it, and every such referrer is itself",
        "# replaced, so nothing links it), dead (not live).",
        "# signature_checked: header <file> = the game's header declares it, and since every unit's object",
        "# matches retail byte for byte, the call sites were compiled from that prototype; compiler = an MWCC",
        "# helper no source names; internal = no game header declares it; data = an object, not called.",
        "\t".join(["object", "symbol", "kind", "size", "live", "replacement", "signature_checked", "test",
                   "referrers", "note"]),
    ]
    lines += ["\t".join(r) for r in out]
    destination = os.path.join(root, options.out)
    os.makedirs(os.path.dirname(destination), exist_ok=True)
    with open(destination, "w") as f:
        f.write("\n".join(lines) + "\n")
    counts = collections.Counter((r[4], r[5]) for r in out)
    print(f"{len(out)} rows, {sum(1 for r in out if r[4] == 'yes')} live", file=sys.stderr)
    for (is_live, replacement), n in sorted(counts.items()):
        print(f"  live={is_live} {replacement}: {n}", file=sys.stderr)
    bad = [r for r in out if r[4] == "yes" and r[5] == "dead"]
    if bad:
        sys.exit(f"{len(bad)} live rows have no replacement")
    return 0


if __name__ == "__main__":
    sys.exit(main())
