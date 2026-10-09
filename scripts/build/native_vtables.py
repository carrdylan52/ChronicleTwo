"""Import exact compiler-native vtables from declared source-only producers."""

import copy
import hashlib
import json
from pathlib import Path
import struct
from typing import NamedTuple

import postprocess_object as p


# A table's retail owner need not be the translation unit whose constructors
# make MWCC emit it. The source-only producer has no assembly replacements.
TABLE_PRODUCERS = {'object': {'__vt__7CObject': 'map'},
                   'mapparts': {'__vt__9CMapParts': 'map'}}


class Donor(NamedTuple):
    """Immutable raw compiler input and the source that establishes its provenance."""

    name: str
    producer: str
    path: Path
    raw: bytes
    source: bytes


def donor_inputs(unit, source, base_dir, *, lay=None):
    """Read required raw objdiff producers; retained markers stay authoritative."""
    from objdiff_data import fallback_data_names

    held = fallback_data_names(source)
    requested = {name: producer for name, producer in TABLE_PRODUCERS.get(unit, {}).items()
                 if name not in held}
    if not requested:
        return ()
    base_dir = Path(base_dir)
    if base_dir.name != 'base' or base_dir.parent.name != 'objdiff':
        raise ValueError(f'{unit}: native vtables require raw objdiff/base producers')
    lay = p.layout.Layout() if lay is None else lay
    inputs = []
    for name, producer in sorted(requested.items()):
        path = base_dir / f'{producer}.cpp.o'
        source_path = p.ROOT / lay.source(producer)
        source_bytes = source_path.read_bytes()
        if name in fallback_data_names(source_bytes.decode('utf-8')):
            raise ValueError(f'{name}: producer retains an assembly data marker')
        if not path.is_file() or path.is_symlink():
            raise ValueError(f'{name}: missing raw native producer {path}')
        if path.stat().st_mtime_ns < source_path.stat().st_mtime_ns:
            raise ValueError(f'{name}: stale native producer {path}')
        inputs.append(Donor(name, producer, path, path.read_bytes(), source_bytes))
    return tuple(inputs)


def donor_fingerprint(inputs):
    """Include producer contents and source provenance in comparison cache inputs."""
    return json.dumps([(entry.name, entry.producer, str(entry.path),
                        hashlib.sha256(entry.raw).hexdigest(),
                        hashlib.sha256(entry.source).hexdigest())
                       for entry in inputs]).encode()


def validate_vtable(donor, name, unit, *, retail, pieces, rows, addresses):
    """Prove a unique whole-section table, its initializer and all native consumers."""
    symbols = donor.symtab.symbols
    candidates = [symbol for symbol in symbols if symbol.name == name
                  and 0 < symbol.st_shndx < len(donor.sections)]
    if len(candidates) != 1:
        raise ValueError(f'{name}: missing or ambiguous native definition')
    symbol = candidates[0]
    index = symbol.st_shndx
    section = donor.sections[index]
    declarations = [(start, size) for start, spelling, size, function in rows
                    if spelling == name and not function]
    if len(declarations) != 1:
        raise ValueError(f'{name}: missing or ambiguous retail declaration')
    start, size = declarations[0]
    owners = [(lo, hi) for kind, run in pieces.unit(unit) if kind == '.vtables'
              for spelling, lo, hi in run if spelling == name]
    if (symbol.type != p.STT_OBJECT or symbol.bind != 1 or symbol.st_value
            or not size or size != symbol.st_size or size != len(section.data)
            or section.name != '.vtables' or section.sh_type != p.SHT_PROGBITS
            or section.sh_flags not in (p.SHF_ALLOC, p.FLAGS['.vtables'])
            or len(owners) != 1 or owners[0][0] != start or start + size > owners[0][1]
            or section.sh_addralign < 1 or section.sh_addralign & (section.sh_addralign - 1)
            or start % section.sh_addralign
            or any(other is not symbol and other.st_shndx == index
                   and other.type != p.STT_SECTION for other in symbols)):
        raise ValueError(f'{name}: unsupported native storage, extent or aliases')
    records = [record for record in donor.relocations if record.sh_info == index]
    entries = [entry for record in records for entry in record.relocations]
    fields = {entry.r_offset: entry.reloc_type for entry in entries}
    expected = {address - start: kind for address, kind in retail.relocations.items()
                if start <= address < start + size}
    if len(fields) != len(entries) or fields != expected:
        raise ValueError(f'{name}: native relocation metadata differs from retail')
    functions = {spelling for _start, spelling, _size, function in rows if function}
    data = bytearray(section.data)
    for entry in entries:
        offset = entry.r_offset
        if (entry.reloc_type != p.R_MIPS_32 or offset % 4 or not 0 <= offset <= size - 4
                or not 0 <= entry.symbol_index < len(symbols)):
            raise ValueError(f'{name}: unsupported native relocation')
        target = symbols[entry.symbol_index]
        address = addresses.get(target.name)
        if (target.type not in (0, p.STT_FUNC) or target.st_value
                or target.name not in functions or address is None):
            raise ValueError(f'{name}: unresolved native callback {target.name}')
        value = struct.unpack_from('<I', data, offset)[0]
        struct.pack_into('<I', data, offset, (address + value) & 0xFFFFFFFF)
    if data != retail.bytes(start, start + size):
        raise ValueError(f'{name}: native initializer differs from retail')
    consumers = {record.sh_info for record in donor.relocations
                 if any(symbols[entry.symbol_index].st_shndx == index
                        for entry in record.relocations)}
    code_rows = {spelling: (address, extent, function)
                 for address, spelling, extent, function in rows}
    if not consumers or not all(p.complete_code_consumer(
            donor, consumer, retail=retail, rows=code_rows,
            address_of_symbol=lambda target: p.address_of(target.name, addresses),
            gp=addresses.get('_gp')) for consumer in consumers):
        raise ValueError(f'{name}: unmatched native consumer')
    return symbol, section, records


def import_vtables(elf, unit, inputs, *, retail=None, pieces=None, rows=None, addresses=None):
    """Copy verified native tables and relocation links without changing any code."""
    from objdiff_data import code_snapshot, fallback_data_names

    if not inputs:
        return
    if len({entry.name for entry in inputs}) != len(inputs):
        raise ValueError(f'{unit}: duplicate native table inputs')
    retail = p.layout.Retail() if retail is None else retail
    pieces = p.disassemble.Pieces() if pieces is None else pieces
    rows = p.layout.read_symbols() if rows is None else rows
    addresses = p.retail_addresses() if addresses is None else addresses
    before = code_snapshot(elf)
    plans = []
    for entry in inputs:
        if TABLE_PRODUCERS.get(unit, {}).get(entry.name) != entry.producer:
            raise ValueError(f'{entry.name}: undeclared native producer')
        if (entry.path.name != f'{entry.producer}.cpp.o' or entry.path.parent.name != 'base'
                or entry.path.parent.parent.name != 'objdiff'
                or entry.name in fallback_data_names(entry.source.decode('utf-8'))):
            raise ValueError(f'{entry.name}: unsupported native producer provenance')
        existing_tables = [symbol for symbol in elf.symtab.symbols if symbol.name == entry.name]
        if len(existing_tables) > 1 or any(symbol.st_shndx for symbol in existing_tables):
            raise ValueError(f'{entry.name}: competing recipient definition')
        donor = p.Elf(entry.raw)
        p.name_sections(donor)
        donor_before = code_snapshot(donor)
        placeholders = {symbol.st_shndx for symbol in donor.symtab.symbols
                        if symbol.name.endswith(p.layout.PLACEHOLDER_SUFFIX)}
        for symbol in donor.symtab.symbols:
            if symbol.type != p.STT_SECTION and not symbol.name.startswith('.'):
                symbol.name = p.project_name(symbol.name)
        p.name_literal_data(donor, entry.producer, placeholders, retail=retail,
                            pieces=pieces, addresses=addresses, rows=rows)
        if code_snapshot(donor) != donor_before:
            raise ValueError(f'{entry.name}: native producer normalization changed code')
        symbol, section, records = validate_vtable(
            donor, entry.name, unit, retail=retail, pieces=pieces, rows=rows, addresses=addresses)
        for record in records:
            for relocation in record.relocations:
                target = donor.symtab.symbols[relocation.symbol_index]
                existing = [other for other in elf.symtab.symbols if other.name == target.name]
                if len(existing) > 1 or any(other.type not in (0, p.STT_FUNC)
                                           or other.st_value for other in existing):
                    raise ValueError(f'{entry.name}: ambiguous recipient callback {target.name}')
        plans.append((donor, symbol, section, records))
    for donor, symbol, section, records in plans:
        copied = copy.deepcopy(section)
        copied.sh_name = elf.add_sh_symbol('.vtables')
        copied.sh_flags = p.FLAGS['.vtables']
        copied.sh_addralign = 1
        index = elf.add_section(copied)
        target_index, existing = elf.symtab.get_symbol_by_name(symbol.name)
        copied_symbol = copy.deepcopy(symbol)
        copied_symbol.st_shndx = index
        copied_symbol.st_name = elf.strtab.add_symbol(symbol.name)
        if target_index is None:
            elf.add_symbol(copied_symbol)
        else:
            elf.symtab.symbols[target_index] = copied_symbol
        for original in records:
            record = copy.deepcopy(original)
            record.name = '.rel.vtables'
            record.sh_name = elf.add_sh_symbol(record.name)
            record.sh_link = elf.symtab_index
            record.sh_info = index
            for relocation in record.relocations:
                target = donor.symtab.symbols[relocation.symbol_index]
                target_index, existing = elf.symtab.get_symbol_by_name(target.name)
                if target_index is None:
                    copied_target = p.Symbol(0, 0, 0, 0x12, 0, 0)
                    copied_target.name = target.name
                    target_index = elf.add_symbol(copied_target)
                relocation.symbol_index = target_index
            elf.add_section(record)
    if code_snapshot(elf) != before:
        raise ValueError(f'{unit}: native vtable import changed recipient code')
