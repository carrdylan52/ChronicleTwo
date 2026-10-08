#!/usr/bin/env python3
"""Give each section of a compiled game unit retail's name, type, flags and alignment.

    postprocess_object.py <object>

MWCC emits every function and every datum in a section of its own, named for
what the compiler made of it: a datum tools/mwccgap supplies is a `const`
array to the compiler, so it lands in `.rodata` whatever retail's section is.
Each allocated section here is looked up by the symbol it defines -- in
main.symbols.txt, or by the address an invented `D_<ADDR8>` name spells -- and
given the section retail holds that address in:

- the name `layout.section_of` gives, and the matching `.rel<name>` for its
  relocations;
- NOBITS for `.sbss` and `.bss`, PROGBITS otherwise;
- the flags of that kind of section, `.sdata` and `.sbss` carrying the MIPS
  gp-relative flag as MWCC sets it, `.init` being code;
- alignment 1 for a datum, whose extent already runs to the next symbol, so
  no padding is added between pieces; a function keeps the compiler's.

A section whose symbol retail does not name -- a compiler-generated one, in a
decompiled function's future -- is left as the compiler emitted it, but for
the alignment of a `.rodata` one: retail has every compiler-generated literal
of `.rodata` on a multiple of eight, and this compiler gives one of four
bytes or fewer, such as the string "BIN", a multiple of four, so its
alignment is raised to eight.

A datum's placeholder is defined under an alias (`layout.PLACEHOLDER_SUFFIX`),
so that the source can also see the datum's typed declaration; the alias is
dropped here, before anything is looked up by name.

A compiled function may use data the unit still supplies through a
placeholder: a string or floating-point literal, a function-local static, a
file-local variable. The compiler emits its own copy of each, under a name of
its own. Every reference a compiled function makes to such a copy is repointed
at the placeholder holding the address retail's instruction refers to, and a
copy nothing refers to any more is checked against retail's bytes and marked
`.dead` for scripts/build/fixup_sections.sh to remove -- so a function can be
compiled before the data it uses is migrated (`bind_local_data`). Named local
BSS statics are also bound by a unique source declaration and explicit retail
marker with the same base name and exact extent (`bind_named_static_bss`),
independently of their compiler-generated suffix or instruction positions.

tools/mwccgap adds a symbol a datum's relocations refer to a second time, and
a datum that refers to itself carries the assembler's section index rather
than the object's. Every such duplicate is folded into the symbol the object
already defines, so a reference resolves to the unit's own definition.
"""

import argparse
import bisect
import re
import struct
import sys
from collections import Counter
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "mwccgap"))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from mwccgap.elf import Elf, Symbol, RelocationRecord, SHT_NOBITS, BssSection  # noqa: E402

import layout  # noqa: E402
import disassemble

SHT_PROGBITS = 1
SHF_WRITE = 0x1
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4
SHF_MIPS_GPREL = 0x10000000

STT_SECTION = 3
STT_OBJECT = 1
STT_FUNC = 2
STB_MWCC_COALESCED = 13
STB_LOCAL = 0
STB_WEAK = 2

R_MIPS_32 = 2
R_MIPS_HI16 = 5
R_MIPS_LO16 = 6
R_MIPS_GPREL16 = 7

DEAD = ".dead"

# The least alignment retail gives a compiler-generated literal of `.rodata`.
RODATA_ALIGNMENT = 8

FLAGS = {
    ".text": SHF_ALLOC | SHF_EXECINSTR,
    ".init": SHF_ALLOC | SHF_EXECINSTR,
    ".data": SHF_WRITE | SHF_ALLOC,
    ".rodata": SHF_ALLOC,
    ".ctor": SHF_WRITE | SHF_ALLOC,
    ".vtables": SHF_WRITE | SHF_ALLOC,
    ".sdata": SHF_WRITE | SHF_ALLOC | SHF_MIPS_GPREL,
    ".sbss": SHF_WRITE | SHF_ALLOC | SHF_MIPS_GPREL,
    ".bss": SHF_WRITE | SHF_ALLOC,
}
CODE = (".text", ".init")

INVENTED = re.compile(r"D_([0-9A-F]{8})")


def name_sections(elf):
    """Name every section; mwccgap's reader leaves its `.text` sections unnamed."""
    for section in elf.sections:
        section.name = elf.shstrtab.get_symbol_by_index(section.sh_name)


def retail_addresses():
    return {name: address for address, name, _s, _f in layout.read_symbols(ROOT / layout.SYMBOLS)}


def address_of(name, addresses):
    address = addresses.get(name)
    if address is None:
        m = INVENTED.fullmatch(name)
        if m:
            address = int(m.group(1), 16)
    return address


def drop_placeholder_aliases(elf):
    """Give every placeholder's symbol the name of the datum it stands for.

    Returns the indices of the sections the placeholders occupy.
    """
    suffix = layout.PLACEHOLDER_SUFFIX
    sections = set()
    for symbol in elf.symtab.symbols:
        if re.fullmatch(r'jtbl_[0-9A-Fa-f]{8}', symbol.name):
            symbol.name = 'D_' + symbol.name[5:].upper()
            symbol.st_name = elf.strtab.add_symbol(symbol.name)
        if symbol.name.endswith(suffix):
            symbol.name = symbol.name[:-len(suffix)]
            symbol.st_name = elf.strtab.add_symbol(symbol.name)
            if 0 < symbol.st_shndx < len(elf.sections):
                sections.add(symbol.st_shndx)
    return sections


def project_name(name):
    """A compiler's symbol name as main.symbols.txt spells it."""
    name = re.sub(r"[,<>.$]", "_", name)
    return "at_" + name[1:] if name.startswith("@") else name


def rename_dng_main_local_static(elf, unit):
    """Use retail names for InitDungeonMain's C++ local static and its guard."""
    if unit != 'dng_main':
        return
    symbols = elf.symtab.symbols
    storage = [symbol for symbol in symbols
               if re.fullmatch(r'debug_event_stack_\d+', symbol.name)
               and symbol.st_size == 0x30 and symbol.name != 'debug_event_stack_1106']
    if not storage and any(symbol.name == 'debug_event_stack_1106' for symbol in symbols):
        return
    if len(storage) != 1:
        raise ValueError(f'dng_main: expected one debug event stack, found {len(storage)}')
    owners = [symbol.st_shndx for symbol in symbols
              if symbol.name == 'InitDungeonMain__F13INIT_LOOP_ARG']
    referenced = {relocation.symbol_index for record in elf.relocations
                  if record.sh_info in owners for relocation in record.relocations}
    if not any(symbols[index] is storage[0] for index in referenced):
        raise ValueError('dng_main: initializer does not reference its debug event stack')
    guard = [symbol for index, symbol in enumerate(symbols)
             if index in referenced and re.fullmatch(r'init_\d+', symbol.name)
             and symbol.st_size == 1 and symbol.bind == STB_LOCAL
             and 0 < symbol.st_shndx < len(elf.sections)
             and elf.sections[symbol.st_shndx].sh_type == SHT_NOBITS]
    if len(guard) != 1:
        raise ValueError('dng_main: debug event stack initialization guard missing')
    for symbol, name in ((storage[0], 'debug_event_stack_1106'),
                         (guard[0], 'init_1107')):
        if any(other is not symbol and other.name == name for other in symbols):
            raise ValueError(f'dng_main: duplicate {name}')
        symbol.name = name
        symbol.st_name = elf.strtab.add_symbol(name)


def sext16(value):
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def section_size(section):
    return section.sh_size if section.sh_type == SHT_NOBITS else len(section.data)


def rename_shared_names(elf, rows, address_of_section, retail, gp):
    """Spell an undefined symbol the way main.symbols.txt does.

    Two retail symbols of one name are told apart there by a `__<n>` suffix,
    which compiled code cannot know: it refers to the plain name. Which of
    them a reference means is read off the address retail's instruction has
    at the same place.
    """
    plain = {name: address for address, name, _s, _f in rows}
    candidates = {}
    for address, name, _size, _func in rows:
        m = re.fullmatch(r"(.+)__\d+", name)
        if m:
            candidates.setdefault(m.group(1), []).append((address, name))
    for name, options in candidates.items():
        if name in plain:
            options.append((plain[name], name))
    symbols = elf.symtab.symbols
    for record in elf.relocations:
        base = address_of_section.get(record.sh_info)
        section = elf.sections[record.sh_info]
        if base is None or not section.sh_flags & SHF_EXECINSTR:
            continue
        relocations = record.relocations
        for k, relocation in enumerate(relocations):
            symbol = symbols[relocation.symbol_index]
            options = candidates.get(project_name(symbol.name))
            if symbol.st_shndx != 0 or not options:
                continue
            offset = relocation.r_offset
            word = retail.word(base + offset)
            if relocation.reloc_type == 4:
                target = ((base + offset) & 0xF0000000) | ((word & 0x03FFFFFF) << 2)
            elif relocation.reloc_type == R_MIPS_GPREL16:
                target = gp + sext16(word)
            elif relocation.reloc_type in (R_MIPS_HI16, R_MIPS_LO16):
                kind = R_MIPS_LO16 if relocation.reloc_type == R_MIPS_HI16 else R_MIPS_HI16
                order = list(range(k + 1, len(relocations))) + list(range(k - 1, -1, -1))
                other = next((relocations[j] for j in order
                              if relocations[j].reloc_type == kind
                              and relocations[j].symbol_index == relocation.symbol_index), None)
                if other is None:
                    continue
                hi, lo = ((offset, other.r_offset) if relocation.reloc_type == R_MIPS_HI16
                          else (other.r_offset, offset))
                target = ((retail.word(base + hi) & 0xFFFF) << 16) + sext16(retail.word(base + lo))
            else:
                continue
            chosen = [name for address, name in options if address == target]
            if len(chosen) == 1:
                symbol.name = chosen[0]
                symbol.st_name = elf.strtab.add_symbol(symbol.name)


def static_bss_pairs(source, native, held):
    """Select unique source statics and explicit BSS markers with equal extents.

    The inputs contain (symbol index, symbol name, size), not compiler objects.
    Compiler-generated local-static suffixes identify no stable source property.
    A repeated source name or object/marker base is deliberately left unbound.
    """
    source = re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"', '', source,
                    flags=re.DOTALL)
    declarations = Counter(re.findall(
        r'\bstatic\s+(?:const\s+)?[A-Za-z_]\w*(?:\s+|\s*[*&]\s*)'
        r'([A-Za-z_]\w*)\s*(?=[;=\[])', source))
    markers = set(re.findall(r'\bINCLUDE_BSS\(\s*([A-Za-z_]\w*)\s*,', source))

    def base(name):
        # project_name converts MWCC's dollar separator to an underscore.
        match = re.fullmatch(r'(.+?)[_$]\d+(?:__\d+)?', name)
        return match.group(1) if match else None

    natives = {}
    placeholders = {}
    for index, name, size in native:
        key = base(name)
        if key is not None:
            natives.setdefault(key, []).append((index, size))
    for index, name, size in held:
        key = base(name)
        if key is not None and name in markers:
            placeholders.setdefault(key, []).append((index, size))
    pairs = []
    for key, options in natives.items():
        targets = placeholders.get(key, [])
        if declarations[key] != 1 or len(options) != 1 or len(targets) != 1:
            continue
        native_index, native_size = options[0]
        held_index, held_size = targets[0]
        if native_size and native_size == held_size:
            pairs.append((native_index, held_index))
    return pairs


def bind_named_static_bss(elf, unit, placeholder_sections):
    """Bind unambiguous native local BSS to its explicit retail storage.

    This binding uses the unit, source declaration, base name and exact extent,
    so instruction scheduling cannot move a reference onto the wrong datum.
    It preserves all relocation addends and never binds initialized storage.
    """
    source_path = ROOT / 'ps2' / 'src' / (unit + '.cpp')
    if not source_path.is_file():
        return []
    lay = layout.Layout(ROOT / layout.YAML)
    ranges = [(lo, hi) for section, lo, hi in lay.sections(unit)
              if section in ('.bss', '.sbss')]
    rows = {name: (address, size) for address, name, size, function
            in layout.read_symbols(ROOT / layout.SYMBOLS)
            if not function and any(lo <= address < hi and address + size <= hi
                                    for lo, hi in ranges)}
    symbols = elf.symtab.symbols
    sections = elf.sections
    native, held = [], []
    for index, symbol in enumerate(symbols):
        section_index = symbol.st_shndx
        if (symbol.type != STT_OBJECT or symbol.st_value != 0
                or not 0 < section_index < len(sections)):
            continue
        section = sections[section_index]
        if (section.name == DEAD or not section.sh_flags & SHF_ALLOC
                or section.sh_flags & SHF_EXECINSTR):
            continue
        size = section_size(section)
        # A section alias or interior symbol needs a different addend mapping.
        if any(other.st_shndx == section_index and other.st_value != 0
               for other in symbols):
            continue
        if (symbol.bind == STB_LOCAL and section.sh_type == SHT_NOBITS
                and symbol.st_size == size
                and sum(other.st_shndx == section_index and other.type == STT_OBJECT
                        for other in symbols) == 1):
            native.append((index, symbol.name, size))
        expected = rows.get(symbol.name)
        if (symbol.bind != STB_LOCAL and expected is not None
                and expected[1] == symbol.st_size == size
                and (section.sh_type == SHT_NOBITS or not any(section.data))
                and not any(record.sh_info == section_index and record.relocations
                            for record in elf.relocations)):
            held.append((index, symbol.name, size))

    pairs = static_bss_pairs(source_path.read_text(), native, held)
    dropped = []
    for native_index, held_index in pairs:
        original, target = symbols[native_index], symbols[held_index]
        section_index = original.st_shndx
        if sections[section_index].name != layout.section_of(rows[target.name][0]):
            continue
        aliases = {index for index, symbol in enumerate(symbols)
                   if symbol.st_shndx == section_index}
        undefined = {index for index, symbol in enumerate(symbols)
                     if symbol.st_shndx == 0 and symbol.name == original.name}
        for record in elf.relocations:
            for relocation in record.relocations:
                if relocation.symbol_index in aliases or relocation.symbol_index in undefined:
                    relocation.symbol_index = held_index
        # Name the unresolved aliases too, so duplicate folding cannot send a
        # later reference back to the dead native definition.
        for index in undefined:
            symbols[index].name = target.name
            symbols[index].st_name = elf.strtab.add_symbol(target.name)
            symbols[index].st_shndx = target.st_shndx
            symbols[index].st_value = target.st_value
        section = sections[section_index]
        section.name = DEAD
        section.sh_name = elf.add_sh_symbol(DEAD)
        for record in elf.relocations:
            if record.sh_info == section_index:
                record.name = '.rel' + DEAD
                record.sh_name = elf.add_sh_symbol(record.name)
        placeholder_sections.add(target.st_shndx)
        dropped.append(original.name)
    return dropped


def bind_local_data(elf, unit, placeholder_sections):
    """Point compiled code at the placeholders of the data it uses.

    Returns the names of the compiler's copies that were dropped.
    """
    lay = layout.Layout(ROOT / layout.YAML)
    if lay.kinds.get(unit) != "cpp":
        return []
    ranges = [(lo, hi) for _s, lo, hi in lay.sections(unit)]

    def in_unit(address):
        return any(lo <= address < hi for lo, hi in ranges)

    rows = layout.read_symbols(ROOT / layout.SYMBOLS)
    gp = next(a for a, n, _s, _f in rows if n == "_gp")
    # The unit's own names; a local that shares its name with another unit's
    # carries a `__<n>` suffix in main.symbols.txt and none in the object.
    names = {}
    for address, name, _size, _func in rows:
        if in_unit(address):
            names[name] = address
            names.setdefault(re.sub(r"__\d+$", "", name), address)

    sections = elf.sections
    symbols = elf.symtab.symbols
    address_of_section = {}
    defining_symbol = {}
    for index, symbol in enumerate(symbols):
        at = symbol.st_shndx
        if (not symbol.name or symbol.type == STT_SECTION or symbol.st_value != 0
                or not 0 < at < len(sections) or not sections[at].sh_flags & SHF_ALLOC
                or symbol.name.startswith(".")):
            continue
        code = bool(sections[at].sh_flags & SHF_EXECINSTR)
        if not code and at not in placeholder_sections:
            continue
        name = project_name(symbol.name)
        address = names.get(name)
        if address is None:
            m = INVENTED.fullmatch(name)
            if m and in_unit(int(m.group(1), 16)):
                address = int(m.group(1), 16)
        if address is not None and at not in address_of_section:
            address_of_section[at] = address
            defining_symbol[at] = index

    held = sorted((address_of_section[i], i) for i in placeholder_sections
                  if i in address_of_section)
    starts = [a for a, _i in held]

    def placeholder_at(address):
        k = bisect.bisect_right(starts, address) - 1
        if k < 0:
            return None
        start, index = held[k]
        return (start, index) if address < start + max(section_size(sections[index]), 1) else None

    for symbol in symbols:
        if symbol.st_shndx != 0:
            continue
        name = project_name(symbol.name)
        match = INVENTED.fullmatch(name)
        address = int(match.group(1), 16) if match else names.get(name)
        if address is None:
            continue
        found = placeholder_at(address)
        if found is None:
            found = next(((start, index) for index, start in address_of_section.items()
                          if sections[index].sh_flags & SHF_EXECINSTR
                          and start <= address < start + section_size(sections[index])), None)
        if found is not None:
            start, index = found
            symbol.st_shndx = index
            symbol.st_value = address - start

    retail = layout.Retail(ROOT / layout.ELF_PATH)
    bound = {}
    for record in elf.relocations:
        at = record.sh_info
        if at not in address_of_section or not sections[at].sh_flags & SHF_EXECINSTR:
            continue
        base = address_of_section[at]
        data = bytearray(sections[at].data)
        relocations = record.relocations
        # Pairs are found by the symbols the compiler wrote, which the loop
        # below replaces as it goes.
        original = [r.symbol_index for r in relocations]

        def word(offset):
            return struct.unpack_from("<I", data, offset)[0]

        def partner(k, kind):
            """The nearest relocation of `kind` against the same symbol."""
            symbol = original[k]
            order = list(range(k + 1, len(relocations))) + list(range(k - 1, -1, -1))
            if kind == R_MIPS_HI16:
                order = list(range(k - 1, -1, -1)) + list(range(k + 1, len(relocations)))
            for j in order:
                if relocations[j].reloc_type == kind and original[j] == symbol:
                    return relocations[j]
            return None

        changed = False
        for k, relocation in enumerate(relocations):
            target = symbols[original[k]]
            to = target.st_shndx
            if (not 0 < to < len(sections) or to in placeholder_sections
                    or not sections[to].sh_flags & SHF_ALLOC
                    or sections[to].sh_flags & SHF_EXECINSTR):
                continue
            kind = relocation.reloc_type
            offset = relocation.r_offset
            if kind == R_MIPS_GPREL16:
                theirs = gp + sext16(retail.word(base + offset))
                ours = sext16(word(offset))
            elif kind in (R_MIPS_HI16, R_MIPS_LO16):
                other = partner(k, R_MIPS_LO16 if kind == R_MIPS_HI16 else R_MIPS_HI16)
                if other is None:
                    continue
                hi, lo = (offset, other.r_offset) if kind == R_MIPS_HI16 else (other.r_offset, offset)
                theirs = ((retail.word(base + hi) & 0xFFFF) << 16) + sext16(retail.word(base + lo))
                ours = ((word(hi) & 0xFFFF) << 16) + sext16(word(lo))
            else:
                continue
            found = placeholder_at(theirs)
            if found is None:
                continue
            start, index = found
            addend = theirs - start
            if kind == R_MIPS_HI16:
                field = ((addend + 0x8000) >> 16) & 0xFFFF
            else:
                field = addend & 0xFFFF
            struct.pack_into("<I", data, offset, (word(offset) & 0xFFFF0000) | field)
            if kind != R_MIPS_HI16:
                bound.setdefault(to, theirs - ours - target.st_value)
            relocation.symbol_index = defining_symbol[index]
            changed = True
        if changed:
            sections[at].data = bytes(data)

    rename_shared_names(elf, rows, address_of_section, retail, gp)

    referenced = {symbols[r.symbol_index].st_shndx
                  for record in elf.relocations for r in record.relocations}
    dropped = []
    for index, start in bound.items():
        if index in referenced:
            continue
        section = sections[index]
        label = next((s.name for s in symbols if s.st_shndx == index and s.name
                      and s.type != STT_SECTION), f"section {index}")
        has_relocations = any(r.sh_info == index and r.relocations for r in elf.relocations)
        if section.sh_type != SHT_NOBITS and not has_relocations:
            size = len(section.data)
            if bytes(section.data) != retail.bytes(start, start + size):
                raise ValueError(f"{label}: the compiled datum differs from retail's at "
                                 f"0x{start:08X}")
        section.sh_name = elf.add_sh_symbol(DEAD)
        section.name = DEAD
        for record in elf.relocations:
            if record.sh_info == index:
                record.sh_name = elf.add_sh_symbol(".rel" + DEAD)
                record.name = ".rel" + DEAD
        dropped.append(label)

    starts = {index: start for index, start in bound.items() if sections[index].name == DEAD}
    while True:
        live = {symbols[r.symbol_index].st_shndx for record in elf.relocations
                if sections[record.sh_info].name != DEAD for r in record.relocations}
        found = {}
        for record in elf.relocations:
            base = starts.get(record.sh_info)
            if base is None:
                continue
            data = sections[record.sh_info].data
            for relocation in record.relocations:
                target = symbols[relocation.symbol_index]
                to = target.st_shndx
                if (relocation.reloc_type != R_MIPS_32 or not 0 < to < len(sections)
                        or to in live or to in starts or to in found or to in placeholder_sections
                        or not sections[to].sh_flags & SHF_ALLOC
                        or sections[to].sh_flags & SHF_EXECINSTR):
                    continue
                ours = struct.unpack_from("<I", data, relocation.r_offset)[0]
                found[to] = retail.word(base + relocation.r_offset) - ours - target.st_value
        if not found:
            break
        for index, start in found.items():
            section = sections[index]
            label = next((s.name for s in symbols if s.st_shndx == index and s.name
                          and s.type != STT_SECTION), f"section {index}")
            has_relocations = any(r.sh_info == index and r.relocations for r in elf.relocations)
            if section.sh_type != SHT_NOBITS and not has_relocations:
                size = len(section.data)
                if bytes(section.data) != retail.bytes(start, start + size):
                    raise ValueError(f"{label}: the compiled datum differs from retail's at "
                                     f"0x{start:08X}")
            section.sh_name = elf.add_sh_symbol(DEAD)
            section.name = DEAD
            for record in elf.relocations:
                if record.sh_info == index:
                    record.sh_name = elf.add_sh_symbol(".rel" + DEAD)
                    record.name = ".rel" + DEAD
            starts[index] = start
            dropped.append(label)
    return dropped


def discard_external_vtables(elf, unit, placeholder_sections):
    lay = layout.Layout(ROOT / layout.YAML)
    ranges = [(lo, hi) for section, lo, hi in lay.sections(unit)]
    addresses = retail_addresses()
    retail = layout.Retail(ROOT / layout.ELF_PATH)
    symbols = elf.symtab.symbols
    for symbol in symbols:
        index = symbol.st_shndx
        if not symbol.name.startswith('__vt__') or not 0 < index < len(elf.sections):
            continue
        if index in placeholder_sections:
            continue
        start = address_of(project_name(symbol.name), addresses)
        if start is None or any(lo <= start < hi for lo, hi in ranges):
            continue
        data = bytearray(elf.sections[index].data)
        for record in elf.relocations:
            if record.sh_info != index:
                continue
            for relocation in record.relocations:
                target = address_of(project_name(symbols[relocation.symbol_index].name), addresses)
                if relocation.reloc_type != 2 or target is None:
                    raise ValueError(f'{symbol.name}: unresolved external vtable slot')
                offset = relocation.r_offset
                value = struct.unpack_from('<I', data, offset)[0]
                struct.pack_into('<I', data, offset, (value + target) & 0xFFFFFFFF)
        if data != retail.bytes(start, start + len(data)):
            raise ValueError(f'{symbol.name}: external vtable differs from retail')
        elf.sections[index].sh_name = elf.add_sh_symbol(DEAD)
        elf.sections[index].name = DEAD
        symbol.st_shndx = 0
        symbol.st_value = 0
        for record in elf.relocations:
            if record.sh_info == index:
                record.sh_name = elf.add_sh_symbol('.rel' + DEAD)
                record.name = '.rel' + DEAD


def discard_external_functions(elf, unit):
    ranges = layout.Layout(ROOT / layout.YAML).sections(unit)
    addresses = retail_addresses()
    for symbol in elf.symtab.symbols:
        index = symbol.st_shndx
        address = addresses.get(symbol.name)
        if (symbol.type != STT_FUNC or symbol.bind not in (STB_WEAK, STB_MWCC_COALESCED)
                or not 0 < index < len(elf.sections) or address is None
                or any(lo <= address < hi for section, lo, hi in ranges)):
            continue
        elf.sections[index].sh_name = elf.add_sh_symbol(DEAD)
        elf.sections[index].name = DEAD
        for record in elf.relocations:
            if record.sh_info == index:
                record.sh_name = elf.add_sh_symbol('.rel' + DEAD)
                record.name = '.rel' + DEAD
        for entry in elf.symtab.symbols:
            if entry.st_shndx == index:
                entry.st_shndx = 0
                entry.st_value = 0


def referenced_data_starts(elf, index, retail, addresses, code_starts, data_starts=None,
                           size=None):
    """Resolve every live incoming reference, rejecting incomplete evidence.

    HI16 entries share the next LO16 for their target in relocation order.
    No numeric compiler name or unchecked consumer supplies an identity.
    """
    data_starts = {} if data_starts is None else data_starts
    destinations = set()
    symbols = elf.symtab.symbols
    for record in elf.relocations:
        section = elf.sections[record.sh_info]
        if section.name == DEAD:
            continue
        incoming = [entry for entry in record.relocations
                    if symbols[entry.symbol_index].st_shndx == index]
        if not incoming:
            continue
        code_base = code_starts.get(record.sh_info)
        data_base = data_starts.get(record.sh_info)
        if code_base is None and data_base is None:
            return None
        contents = section.data
        pending = {}
        seen_offsets = set()
        for entry in incoming:
            offset, kind = entry.r_offset, entry.reloc_type
            target = symbols[entry.symbol_index]
            if offset % 4 or offset < 0 or offset + 4 > len(contents) or offset in seen_offsets:
                return None
            seen_offsets.add(offset)
            base = code_base if code_base is not None else data_base
            if retail.relocations.get(base + offset) != kind:
                return None
            value = struct.unpack_from('<I', contents, offset)[0]
            expected = retail.word(base + offset)
            if code_base is None:
                if kind != R_MIPS_32:
                    return None
                addend, destination = value, expected
            else:
                if kind not in (R_MIPS_HI16, R_MIPS_LO16, R_MIPS_GPREL16):
                    return None
                if (value ^ expected) & 0xFFFF0000:
                    return None
                if kind == R_MIPS_HI16:
                    pending.setdefault(entry.symbol_index, []).append((value, expected))
                    continue
                if kind == R_MIPS_LO16:
                    highs = pending.pop(entry.symbol_index, [])
                    if not highs:
                        return None
                    for high, retail_high in highs:
                        addend = ((high & 0xFFFF) << 16) + sext16(value) + target.st_value
                        destination = ((retail_high & 0xFFFF) << 16) + sext16(expected)
                        if size is not None and not 0 <= addend < size:
                            return None
                        destinations.add((destination - addend) & 0xFFFFFFFF)
                    continue
                if '_gp' not in addresses:
                    return None
                addend, destination = sext16(value), addresses['_gp'] + sext16(expected)
            addend += target.st_value
            if size is not None and not 0 <= addend < size:
                return None
            destinations.add((destination - addend) & 0xFFFFFFFF)
        if pending:
            return None
    return destinations


def bss_data_names(elf, unit, placeholders, *, retail, pieces, addresses, rows):
    """Select isolated native BSS identities by exact extent and all code consumers."""
    runs = pieces.unit(unit)
    declared = {name: size for _start, name, size, function in rows if not function and size}
    cuts = {start: (section, name, end) for section, run in runs if section in layout.NOBITS
            for name, start, end in run if name in declared}
    code = {name: start for section, run in runs if section in CODE for name, start, _end in run}
    symbols = elf.symtab.symbols
    code_starts = {symbol.st_shndx: code[symbol.name] for symbol in symbols if symbol.name in code}
    assignments = []
    for symbol in symbols:
        index = symbol.st_shndx
        if (index in placeholders or symbol.bind != STB_LOCAL or symbol.type != STT_OBJECT
                or symbol.st_value or not 0 < index < len(elf.sections)):
            continue
        section = elf.sections[index]
        if (section.name not in layout.NOBITS or section.sh_type != SHT_NOBITS
                or not section.sh_flags & SHF_ALLOC or not symbol.st_size
                or symbol.st_size != section_size(section)
                or any(other.st_shndx == index and other.st_value for other in symbols)
                or sum(other.st_shndx == index and other.type == STT_OBJECT for other in symbols) != 1
                or any(record.sh_info == index and record.relocations for record in elf.relocations)):
            continue
        targets = referenced_data_starts(elf, index, retail, addresses, code_starts,
                                         size=symbol.st_size)
        if targets is None or len(targets) != 1:
            continue
        start = next(iter(targets))
        cut = cuts.get(start)
        if cut is None:
            continue
        kind, name, end = cut
        if (kind != section.name or declared[name] != symbol.st_size
                or start + symbol.st_size > end
                or any(other is not symbol and other.name == name
                       and 0 < other.st_shndx < len(elf.sections)
                       and elf.sections[other.st_shndx].name != DEAD for other in symbols)):
            continue
        assignments.append((symbol, name))
    counts = Counter(name for _symbol, name in assignments)
    return {id(symbol): name for symbol, name in assignments
            if counts[name] == 1 and symbol.name != name}


def pointer_table_names(elf, unit, placeholders, *, retail, pieces, addresses, rows):
    """Select named local pointer tables before their compiler-owned literals.

    All code consumers establish the table address. Its nonpointer bytes,
    real relocation shape and each pointed-to native literal must match retail.
    """
    runs = pieces.unit(unit)
    cuts = {start: (section, name, end) for section, run in runs
            if section in ('.data', '.sdata', '.rodata') for name, start, end in run}
    declared = {name: size for _start, name, size, function in rows if not function and size}
    code = {name: start for section, run in runs if section in CODE for name, start, _end in run}
    symbols = elf.symtab.symbols
    code_starts = {symbol.st_shndx: code[symbol.name] for symbol in symbols if symbol.name in code}
    assignments = []
    for symbol in symbols:
        index = symbol.st_shndx
        if (index in placeholders or symbol.bind != STB_LOCAL or symbol.type != STT_OBJECT
                or symbol.st_value or not 0 < index < len(elf.sections)
                or re.fullmatch(r'at_\d+', symbol.name)
                or (symbol.name in addresses and addresses[symbol.name] in cuts)):
            continue
        section = elf.sections[index]
        if (section.name not in ('.data', '.sdata', '.rodata')
                or section.sh_type != SHT_PROGBITS or not section.sh_flags & SHF_ALLOC
                or not symbol.st_size or symbol.st_size != len(section.data)
                or any(other.st_shndx == index and other.st_value for other in symbols)
                or sum(other.st_shndx == index and other.type == STT_OBJECT for other in symbols) != 1):
            continue
        targets = referenced_data_starts(elf, index, retail, addresses, code_starts,
                                         size=symbol.st_size)
        if targets is None or len(targets) != 1:
            continue
        start = next(iter(targets))
        cut = cuts.get(start)
        if cut is None:
            continue
        kind, name, end = cut
        if (kind != section.name or declared.get(name) != symbol.st_size
                or start + symbol.st_size > end
                or any(other is not symbol and other.name == name
                       and 0 < other.st_shndx < len(elf.sections)
                       and elf.sections[other.st_shndx].name != DEAD for other in symbols)):
            continue
        entries = [entry for record in elf.relocations if record.sh_info == index
                   for entry in record.relocations]
        expected = {address - start: kind for address, kind in retail.relocations.items()
                    if start <= address < start + symbol.st_size}
        actual = {entry.r_offset: entry.reloc_type for entry in entries}
        if not entries or len(actual) != len(entries) or actual != expected:
            continue
        data = bytearray(section.data)
        literal_starts = {}
        valid = True
        for entry in entries:
            offset = entry.r_offset
            if entry.reloc_type != R_MIPS_32 or offset % 4 or not 0 <= offset <= len(data) - 4:
                valid = False
                break
            target = symbols[entry.symbol_index]
            addend = struct.unpack_from('<I', data, offset)[0]
            destination = retail.word(start + offset)
            known = (addresses.get(target.name)
                     if not re.fullmatch(r'at_\d+', target.name) else None)
            if known is not None:
                valid = (known + addend) & 0xFFFFFFFF == destination
            elif 0 < target.st_shndx < len(elf.sections):
                literal_index = target.st_shndx
                literal = elf.sections[literal_index]
                base = (destination - addend - target.st_value) & 0xFFFFFFFF
                literal_cut = cuts.get(base)
                owners = [other for other in symbols
                          if other.st_shndx == literal_index and other.type == STT_OBJECT]
                valid = (literal_index not in placeholders and literal.name == '.rodata'
                         and literal.sh_type == SHT_PROGBITS and literal.sh_flags & SHF_ALLOC
                         and len(owners) == 1 and owners[0].bind == STB_LOCAL
                         and owners[0].st_value == 0 and literal.data
                         and owners[0].st_size == len(literal.data)
                         and re.fullmatch(r'at_\d+', owners[0].name) is not None
                         and literal_cut is not None and literal_cut[0] == '.rodata'
                         and base + len(literal.data) <= literal_cut[2]
                         and literal.data == retail.bytes(base, base + len(literal.data))
                         and not any(record.sh_info == literal_index and record.relocations
                                     for record in elf.relocations)
                         and literal_starts.get(literal_index, base) == base)
                literal_starts[literal_index] = base
            else:
                valid = False
            if not valid:
                break
            struct.pack_into('<I', data, offset, destination)
        if valid and data == retail.bytes(start, start + len(data)):
            assignments.append((symbol, name))
    counts = Counter(name for _symbol, name in assignments)
    return {id(symbol): name for symbol, name in assignments if counts[name] == 1}


def name_literal_data(elf, unit, placeholders, *, retail=None, pieces=None, addresses=None, rows=None):
    retail = layout.Retail() if retail is None else retail
    pieces = disassemble.Pieces(references=[]) if pieces is None else pieces
    addresses = retail_addresses() if addresses is None else addresses
    rows = layout.read_symbols(ROOT / layout.SYMBOLS) if rows is None else rows
    regions = [(lo, retail.bytes(lo, hi)) for name, lo, hi in pieces.layout.sections(unit)
               if name in ('.rodata', '.sdata', '.data', '.ctor')]
    cuts = {start: (name, end) for section, run in pieces.unit(unit)
            if section in ('.rodata', '.sdata', '.data', '.ctor') for name, start, end in run}
    trailing = {run[-1][1] for section, run in pieces.unit(unit)
                if section in ('.rodata', '.sdata', '.data', '.ctor') and run}
    declared_sizes = {name: size for _address, name, size, function in rows
                      if not function and size}
    bss_names = bss_data_names(elf, unit, placeholders, retail=retail, pieces=pieces,
                               addresses=addresses, rows=rows)
    pointer_names = pointer_table_names(elf, unit, placeholders, retail=retail, pieces=pieces,
                                        addresses=addresses, rows=rows)
    positions = sorted(retail.relocations)
    code_addresses = {name: start for section, run in pieces.unit(unit)
                      if section in CODE for name, start, end in run}
    code_starts = {symbol.st_shndx: code_addresses[symbol.name]
                   for symbol in elf.symtab.symbols if symbol.name in code_addresses}
    data_starts = {
        symbol.st_shndx: addresses[symbol.name]
        for symbol in elf.symtab.symbols
        if symbol.type == STT_OBJECT and symbol.name in addresses
        and addresses[symbol.name] in cuts and symbol.st_value == 0
        and 0 < symbol.st_shndx < len(elf.sections)
        and not re.fullmatch(r'at_\d+(?:__\d+)?', symbol.name)
    }
    data_starts.update((symbol.st_shndx, addresses[pointer_names[id(symbol)]])
                       for symbol in elf.symtab.symbols if id(symbol) in pointer_names)
    for symbol in elf.symtab.symbols:
        name = bss_names.get(id(symbol), pointer_names.get(id(symbol)))
        if name is not None:
            symbol.name = name
            symbol.st_name = elf.strtab.add_symbol(name)
            continue
        index = symbol.st_shndx
        if (index in placeholders or not re.fullmatch(r'(?:at_\d+|\.p__sinit_.+)', symbol.name)
                or symbol.type != STT_OBJECT or symbol.st_value != 0
                or not 0 < index < len(elf.sections)):
            continue
        section = elf.sections[index]
        if (section.name not in ('.rodata', '.sdata', '.data', '.ctor') or not section.data
                or symbol.st_size != len(section.data)):
            continue
        data = bytearray(section.data)
        entries = {}
        unresolved = False
        for record in elf.relocations:
            if record.sh_info != index:
                continue
            for entry in record.relocations:
                target = addresses.get(elf.symtab.symbols[entry.symbol_index].name)
                if entry.reloc_type != R_MIPS_32 or target is None:
                    unresolved = True
                    break
                value = struct.unpack_from('<I', data, entry.r_offset)[0]
                struct.pack_into('<I', data, entry.r_offset, (target + value) & 0xFFFFFFFF)
                entries[entry.r_offset] = entry.reloc_type
        if unresolved:
            continue
        found = []
        for lo, contents in regions:
            offset = contents.find(data)
            while offset >= 0:
                start = lo + offset
                first = bisect.bisect_left(positions, start)
                last = bisect.bisect_left(positions, start + len(data))
                actual = {place - start: retail.relocations[place] for place in positions[first:last]}
                if actual == entries and start in cuts:
                    found.append(start)
                offset = contents.find(data, offset + 1)
        # Ambiguous literals use existing code or named-data reference evidence.
        if len(found) != 1:
            targets = set()
            for record in elf.relocations:
                base = code_starts.get(record.sh_info)
                if base is None:
                    continue
                contents = elf.sections[record.sh_info].data
                for position, entry in enumerate(record.relocations):
                    target = elf.symtab.symbols[entry.symbol_index]
                    offset = entry.r_offset
                    kind = entry.reloc_type
                    if (target.st_shndx != index or kind not in (R_MIPS_HI16, R_MIPS_GPREL16)
                            or retail.relocations.get(base + offset) != kind):
                        continue
                    value = struct.unpack_from('<I', contents, offset)[0]
                    expected = retail.word(base + offset)
                    if (value ^ expected) & 0xFFFF0000:
                        continue
                    if kind == R_MIPS_GPREL16:
                        destination = addresses['_gp'] + sext16(expected) - sext16(value)
                    else:
                        partner = next((other for other in record.relocations[position + 1:]
                                        if other.reloc_type == R_MIPS_LO16
                                        and other.symbol_index == entry.symbol_index), None)
                        if partner is None or retail.relocations.get(base + partner.r_offset) != R_MIPS_LO16:
                            continue
                        low = struct.unpack_from('<I', contents, partner.r_offset)[0]
                        expected_low = retail.word(base + partner.r_offset)
                        if (low ^ expected_low) & 0xFFFF0000:
                            continue
                        destination = ((expected & 0xFFFF) << 16) + sext16(expected_low)
                        destination -= ((value & 0xFFFF) << 16) + sext16(low)
                    destination -= target.st_value
                    targets.add(destination)
            for record in elf.relocations:
                base = data_starts.get(record.sh_info)
                if base is None:
                    continue
                contents = elf.sections[record.sh_info].data
                for entry in record.relocations:
                    target = elf.symtab.symbols[entry.symbol_index]
                    if (target.st_shndx != index or entry.reloc_type != R_MIPS_32
                            or retail.relocations.get(base + entry.r_offset) != R_MIPS_32):
                        continue
                    addend = struct.unpack_from('<I', contents, entry.r_offset)[0]
                    targets.add(retail.word(base + entry.r_offset) - addend - target.st_value)
            if len(targets) != 1 or not targets.issubset(found):
                continue
            found = list(targets)
        start = found[0]
        name, end = cuts[start]
        if declared_sizes.get(name, len(data)) != len(data):
            continue
        # Complete linked pieces retain verified terminal tails; comparison
        # copies trim them at the linker's contents_end. Internal gaps must
        # be alignment, never missing object contents.
        terminal_tail = start in trailing and name in declared_sizes
        if start + len(data) > end:
            continue
        if ((end - start - len(data) >= 16 and not terminal_tail)
                or any(start + len(data) <= address < end for address in retail.relocations)):
            continue
        padding = retail.bytes(start + len(data), end)
        if len(padding) != end - start - len(data) or any(padding):
            continue
        section.data += bytes(len(padding))
        symbol.st_size = len(section.data)
        symbol.name = name
        symbol.st_name = elf.strtab.add_symbol(name)


def pad_data(elf, unit, placeholders, *, retail=None, pieces=None, rows=None):
    retail = layout.Retail() if retail is None else retail
    pieces = disassemble.Pieces() if pieces is None else pieces
    rows = layout.read_symbols(ROOT / layout.SYMBOLS) if rows is None else rows
    runs = [(section, run) for section, run in pieces.unit(unit)
            if section in ('.data', '.sdata', '.rodata', '.bss', '.sbss', '.vtables')]
    cuts = {name: (start, end) for section, run in runs for name, start, end in run}
    trailing = {(section, run[-1][0]) for section, run in runs if run}
    declared_sizes = {name: size for _address, name, size, _is_function
                      in rows if size}
    for symbol in elf.symtab.symbols:
        index = symbol.st_shndx
        if (symbol.type != STT_OBJECT or symbol.st_value or index in placeholders
                or not 0 < index < len(elf.sections) or symbol.name not in cuts):
            continue
        start, end = cuts[symbol.name]
        section = elf.sections[index]
        if symbol.name in declared_sizes and (section.name, symbol.name) in trailing:
            end = min(end, start + declared_sizes[symbol.name])
        size = section_size(section)
        # Symbol sizes describe the object, whereas section pieces also own
        # the following alignment gap. Do not hide an incorrect object size
        # by filling it; only a correctly sized object may acquire padding.
        declared_size = declared_sizes.get(symbol.name)
        if declared_size is not None and size != declared_size:
            continue
        if (section.name in layout.NOBITS and section.sh_type == SHT_NOBITS
                and declared_size is not None and size and end - start > size):
            section.sh_size = end - start
            continue
        if (section.name in ('.data', '.sdata', '.rodata', '.vtables') and size
                and 0 < end - start - size < 16):
            padding = retail.bytes(start + size, end)
            if (len(padding) == end - start - size and not any(padding)
                    and not any(start + size <= address < end for address in retail.relocations)):
                section.data += bytes(len(padding))
                symbol.st_size = len(section.data)


def order_sections(elf):
    addresses = retail_addresses()
    starts = {}
    for symbol in elf.symtab.symbols:
        address = address_of(symbol.name, addresses)
        if (address is not None and symbol.type != STT_SECTION
                and 0 < symbol.st_shndx < len(elf.sections)):
            starts[symbol.st_shndx] = address - symbol.st_value
    order = list(range(len(elf.sections)))
    for name in FLAGS:
        indices = [index for index, section in enumerate(elf.sections)
                   if section.name == name and index in starts]
        ordered = sorted((starts[index], index) for index in indices)
        for position, (address, index) in zip(indices, ordered):
            order[position] = index
    remap = {old: new for new, old in enumerate(order)}
    elf.sections = [elf.sections[index] for index in order]
    elf.e_shstrndx = remap[elf.e_shstrndx]
    for symbol in elf.symtab.symbols:
        if symbol.st_shndx in remap:
            symbol.st_shndx = remap[symbol.st_shndx]
    for section in elf.sections:
        if section.sh_link in remap:
            section.sh_link = remap[section.sh_link]
    for record in elf.relocations:
        record.sh_info = remap[record.sh_info]


def discard_shadow_vtables(elf, placeholder_sections):
    symbols = elf.symtab.symbols
    held = {}
    for index, symbol in enumerate(symbols):
        if symbol.st_shndx in placeholder_sections and symbol.name.startswith('__vt__'):
            held[project_name(symbol.name)] = index
    for index, symbol in enumerate(symbols):
        target_index = held.get(project_name(symbol.name))
        if target_index is None or symbol.st_shndx in placeholder_sections:
            continue
        section_index = symbol.st_shndx
        if not 0 < section_index < len(elf.sections):
            continue
        section = elf.sections[section_index]
        target = symbols[target_index]
        original = bytearray(section.data)
        expected = bytearray(elf.sections[target.st_shndx].data)
        if len(original) > len(expected) or any(expected[len(original):]):
            raise ValueError(f'{symbol.name}: vtable extent differs from retail')
        actual_relocations = {}
        expected_relocations = {}
        for record in elf.relocations:
            if record.sh_info not in (section_index, target.st_shndx):
                continue
            destination = actual_relocations if record.sh_info == section_index else expected_relocations
            data = original if record.sh_info == section_index else expected
            for relocation in record.relocations:
                offset = relocation.r_offset
                name = project_name(symbols[relocation.symbol_index].name)
                destination[offset] = (relocation.reloc_type, name, struct.unpack_from('<I', data, offset)[0])
                struct.pack_into('<I', data, offset, 0)
        if original != expected[:len(original)] or actual_relocations != expected_relocations:
            raise ValueError(f'{symbol.name}: compiled vtable differs from retail')
        for record in elf.relocations:
            for relocation in record.relocations:
                if relocation.symbol_index == index:
                    relocation.symbol_index = target_index
            if record.sh_info == section_index:
                record.sh_name = elf.add_sh_symbol('.rel' + DEAD)
                record.name = '.rel' + DEAD
        section.sh_name = elf.add_sh_symbol(DEAD)
        section.name = DEAD


def bind_suffixed_references(elf, unit):
    lay = layout.Layout(ROOT / layout.YAML)
    if lay.kinds.get(unit) != "cpp":
        return set()
    ranges = [(lo, hi) for _s, lo, hi in lay.sections(unit)]
    names = {name for address, name, _size, _func in layout.read_symbols(ROOT / layout.SYMBOLS)
             if any(lo <= address < hi for lo, hi in ranges)}
    own = {name for name in names if re.fullmatch(r".+__\d+", name)}
    symbols = elf.symtab.symbols
    defined = {}
    for index, symbol in enumerate(symbols):
        if (symbol.name and symbol.type != STT_SECTION and 0 < symbol.st_shndx < len(elf.sections)
                and elf.sections[symbol.st_shndx].name != DEAD):
            defined.setdefault(symbol.name, index)
    remap = {}
    shadowed = set()
    for name in sorted(own - set(defined)):
        plain = re.sub(r"__\d+$", "", name)
        if plain not in defined:
            continue
        if plain in names or sum(re.sub(r"__\d+$", "", candidate) == plain for candidate in own) != 1:
            continue
        definition = symbols[defined[plain]]
        for index, symbol in enumerate(symbols):
            if symbol.st_shndx == 0 and symbol.name == name:
                remap[index] = defined[plain]
        definition.name = name
        definition.st_name = elf.strtab.add_symbol(name)
    # The main unit's handwritten callback is assembled under the split's
    # duplicate suffix. Its native C++ declaration uses the original local
    # name; bind that reference to this unit's assembled body.
    if unit == 'main' and 'VSyncCallBack__Fi__2' in defined:
        for index, symbol in enumerate(symbols):
            if symbol.name == 'VSyncCallBack__Fi':
                remap[index] = defined['VSyncCallBack__Fi__2']
                # mwccgap may already have pointed the unresolved declaration
                # at the assembled section; only the suffixed name is exported.
                symbol.st_shndx = 0
                symbol.st_value = 0
    for record in elf.relocations:
        for relocation in record.relocations:
            if relocation.symbol_index in remap:
                relocation.symbol_index = remap[relocation.symbol_index]
    return shadowed


def fold_duplicates(elf):
    """Keep one symbol per global name; repoint relocations at it."""
    symbols = elf.symtab.symbols
    local = {}
    for index, symbol in enumerate(symbols):
        if (index and symbol.bind == STB_LOCAL and symbol.name and symbol.type != STT_SECTION
                and 0 < symbol.st_shndx < len(elf.sections)):
            local.setdefault(symbol.name, index)
    own = {index: local[symbol.name] for index, symbol in enumerate(symbols)
           if index and symbol.st_shndx == 0 and symbol.bind != STB_LOCAL
           and symbol.name in local}
    if own:
        for record in elf.relocations:
            for relocation in record.relocations:
                relocation.symbol_index = own.get(relocation.symbol_index, relocation.symbol_index)
    keep = {}
    for index, symbol in enumerate(symbols):
        if index == 0 or symbol.bind == STB_LOCAL or not symbol.name:
            continue
        held = keep.get(symbol.name)
        if held is None:
            keep[symbol.name] = index
            continue
        previous = symbols[held]
        if previous.st_shndx == 0 and symbol.st_shndx != 0:
            keep[symbol.name] = index
        elif (0 < previous.st_shndx < len(elf.sections)
              and elf.sections[previous.st_shndx].name == DEAD
              and 0 < symbol.st_shndx < len(elf.sections)
              and elf.sections[symbol.st_shndx].name != DEAD):
            keep[symbol.name] = index
    remap = {}
    kept = []
    for index, symbol in enumerate(symbols):
        if index and symbol.bind != STB_LOCAL and symbol.name and keep[symbol.name] != index:
            continue
        remap[index] = len(kept)
        kept.append(symbol)
    if len(kept) == len(symbols):
        return
    # sh_info counts the leading entries the table treats as local.
    elf.symtab.sh_info = sum(1 for i in range(elf.symtab.sh_info) if i in remap)
    for index, symbol in enumerate(symbols):
        if index not in remap:
            remap[index] = remap[keep[symbol.name]]
    elf.symtab.symbols = kept
    for record in elf.relocations:
        for relocation in record.relocations:
            relocation.symbol_index = remap[relocation.symbol_index]


def discard_unused_literals(elf):
    referenced = {elf.symtab.symbols[relocation.symbol_index].st_shndx
                  for record in elf.relocations for relocation in record.relocations}
    for symbol in elf.symtab.symbols:
        index = symbol.st_shndx
        if (symbol.type != STT_OBJECT or symbol.bind != STB_LOCAL
                or not re.fullmatch(r'at_\d+', symbol.name)
                or not 0 < index < len(elf.sections) or index in referenced):
            continue
        section = elf.sections[index]
        if section.name not in ('.rodata', '.sdata', '.sbss', '.bss'):
            continue
        section.sh_name = elf.add_sh_symbol(DEAD)
        section.name = DEAD
        for record in elf.relocations:
            if record.sh_info == index:
                record.sh_name = elf.add_sh_symbol('.rel' + DEAD)
                record.name = '.rel' + DEAD


def discard_dead_code_records(elf):
    """Drop compiler metadata only when every relocation names discarded code."""
    symbols = elf.symtab.symbols
    records_by_section = {}
    for record in elf.relocations:
        records_by_section.setdefault(record.sh_info, []).append(record)

    for index, section in enumerate(elf.sections):
        if section.name != '.mwcats':
            continue
        records = records_by_section.get(index, ())
        targets = [symbols[relocation.symbol_index].st_shndx
                   for record in records for relocation in record.relocations]
        if not targets:
            continue
        dead = [0 < target < len(elf.sections) and elf.sections[target].name == DEAD
                for target in targets]
        if any(dead) and not all(dead):
            raise ValueError(f'.mwcats section {index} references both dead and live code')
        if not all(dead):
            continue
        section.sh_name = elf.add_sh_symbol(DEAD)
        section.name = DEAD
        for record in records:
            record.sh_name = elf.add_sh_symbol('.rel' + DEAD)
            record.name = '.rel' + DEAD


def retail_sections(elf, addresses, unit=None, shadowed=frozenset()):
    """{section index: retail section name} for every section retail names."""
    out = {}
    ranges = layout.Layout(ROOT / layout.YAML).sections(unit) if unit else []
    for symbol in elf.symtab.symbols:
        if symbol.name in shadowed and symbol.bind != STB_LOCAL:
            continue
        index = symbol.st_shndx
        if not symbol.name or symbol.type == STT_SECTION or not (0 < index < len(elf.sections)):
            continue
        if symbol.st_value != 0:
            continue
        section = elf.sections[index]
        if not section.sh_flags & SHF_ALLOC or section.name == DEAD:
            continue
        address = address_of(symbol.name, addresses)
        if address is None:
            continue
        if (symbol.bind == STB_LOCAL and symbol.name.startswith("at_") and ranges
                and not any(lo <= address < hi for section_name, lo, hi in ranges)):
            continue
        name = layout.section_of(address)
        if name is None:
            raise ValueError(f"{symbol.name}: 0x{address:08X} is in no retail section")
        if out.get(index, name) != name:
            raise ValueError(f"section {index} holds symbols of {out[index]} and {name}")
        out[index] = name
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("object", type=Path)
    ap.add_argument("--order-only", action="store_true")
    args = ap.parse_args()

    elf = Elf(args.object.read_bytes())
    name_sections(elf)
    if args.order_only:
        order_sections(elf)
        args.object.write_bytes(elf.pack())
        return 0
    placeholder_sections = drop_placeholder_aliases(elf)
    for symbol in elf.symtab.symbols:
        if symbol.type != STT_SECTION and not symbol.name.startswith('.'):
            symbol.name = project_name(symbol.name)
            symbol.st_name = elf.strtab.add_symbol(symbol.name)
    shadowed = set()
    name = args.object.name
    unit = None
    if name.endswith(".cpp.o"):
        object_path = args.object.resolve()
        parts = object_path.parts
        if 'obj' in parts:
            unit = '/'.join(parts[parts.index('obj') + 1:])[:-len('.cpp.o')]
        else:
            unit = name[:-len('.cpp.o')]
        bind_named_static_bss(elf, unit, placeholder_sections)
        rename_dng_main_local_static(elf, unit)
        bind_local_data(elf, unit, placeholder_sections)
        name_literal_data(elf, unit, placeholder_sections)
        pad_data(elf, unit, placeholder_sections)
        shadowed = bind_suffixed_references(elf, unit)
        pad_data(elf, unit, placeholder_sections)
        discard_external_vtables(elf, unit, placeholder_sections)
        discard_external_functions(elf, unit)
    discard_shadow_vtables(elf, placeholder_sections)
    fold_duplicates(elf)
    discard_unused_literals(elf)
    discard_dead_code_records(elf)
    addresses = retail_addresses()
    renamed = retail_sections(elf, addresses, unit, shadowed)

    for index, name in renamed.items():
        section = elf.sections[index]
        nobits = name in layout.NOBITS
        if nobits and section.sh_type == SHT_PROGBITS and index in placeholder_sections and not any(section.data):
            section.sh_size = len(section.data)
            section.sh_type = SHT_NOBITS
            section = BssSection(section.sh_name, section.sh_type, section.sh_flags,
                                 section.sh_addr, section.sh_offset, section.sh_size,
                                 section.sh_link, section.sh_info, section.sh_addralign,
                                 section.sh_entsize, b'')
            elf.sections[index] = section
        if nobits != (section.sh_type == SHT_NOBITS):
            raise ValueError(f"section {index} ({section.name}) cannot become {name}")
        section.sh_name = elf.add_sh_symbol(name)
        section.name = name
        section.sh_flags = FLAGS[name]
        if name not in CODE:
            section.sh_addralign = 1

    for index, section in enumerate(elf.sections):
        if (index not in renamed and section.name == ".rodata" and section.sh_flags & SHF_ALLOC
                and section.sh_addralign < RODATA_ALIGNMENT):
            section.sh_addralign = RODATA_ALIGNMENT

    for record in elf.relocations:
        if record.sh_info in renamed:
            record.sh_name = elf.add_sh_symbol(".rel" + renamed[record.sh_info])
            record.name = ".rel" + renamed[record.sh_info]

    empty = [s.name for s in elf.sections if s.sh_flags & SHF_ALLOC and s.name != DEAD
             and (s.sh_size if s.sh_type == SHT_NOBITS else len(s.data)) == 0]
    if empty:
        raise ValueError(f"zero-sized sections, which MWLD rejects: {empty}")

    for symbol in elf.symtab.symbols:
        if symbol.bind == STB_MWCC_COALESCED and symbol.type in (STT_FUNC, STT_OBJECT):
            symbol.bind = STB_WEAK

    order_sections(elf)

    args.object.write_bytes(elf.pack())
    return 0


if __name__ == "__main__":
    sys.exit(main())
