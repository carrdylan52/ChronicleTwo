"""Native table transfers reject ambiguity, fallback provenance and stale caches."""

import copy
from pathlib import Path
import struct
import tempfile
from types import SimpleNamespace as NS
import unittest
from unittest.mock import patch

import native_vtables as n
import objdiff_data as d
from test_objdiff_data import symbol
from mwccgap.elf import Relocation, RelocationRecord


def elf(sections, symbols, records=()):
    table = NS(symbols=symbols)
    table.get_symbol_by_name = lambda name: next(
        ((index, entry) for index, entry in enumerate(symbols) if entry.name == name), (None, None))
    result = NS(sections=sections, symtab=table, relocations=list(records), symtab_index=0,
                strtab=NS(add_symbol=lambda name: len(name)), add_sh_symbol=lambda name: len(name))

    def add_symbol(entry):
        index, _existing = table.get_symbol_by_name(entry.name)
        if index is None:
            index = len(symbols)
            symbols.append(entry)
        return index

    def add_section(section):
        sections.append(section)
        if isinstance(section, RelocationRecord):
            result.relocations.append(section)
        return len(sections) - 1

    result.add_symbol, result.add_section = add_symbol, add_section
    return result


def record(index, entries):
    result = RelocationRecord(0, 9, 0, 0, 0, 0, 0, index, 4, 8, b'')
    result.relocations = [Relocation(offset, (target << 8) | kind) for offset, kind, target in entries]
    return result


class NativeTableTests(unittest.TestCase):
    def fixture(self):
        code = struct.pack('<IIII', 0x3c020000, 0x24420000, 0x0c000000, 0)
        contents = struct.pack('<III', 0, 0, 0x5000)
        expected_code = struct.pack('<IIII', 0x3c020000, 0x24423000, 0x0c001400, 0)
        sections = [NS(name='', sh_flags=0),
                    NS(name='.vtables', sh_flags=2, sh_type=1, sh_addralign=16, data=bytes(12)),
                    NS(name='.text', sh_flags=6, sh_type=1, data=code)]
        symbols = [symbol('__vt__4Base', 1, 12, info=0x11),
                   symbol('caller', 2, 16, info=0x12), symbol('callback', info=0x12)]
        donor = elf(sections, symbols, [record(1, [(8, 2, 2)]),
                                       record(2, [(0, 5, 0), (4, 6, 0), (8, 4, 2)])])
        recipient = elf([NS(name='', sh_flags=0), NS(name='.text', sh_flags=6, data=code)],
                        [symbol('caller', 1, 16, info=0x12), symbol('__vt__4Base'),
                         symbol('callback')], [record(1, [(0, 5, 1), (4, 6, 1), (8, 4, 2)])])
        spans = [(0x1000, expected_code), (0x3000, contents)]
        retail = NS(bytes=lambda lo, hi: next((data[lo - start:hi - start]
                                              for start, data in spans
                                              if start <= lo <= hi <= start + len(data)), b''),
                    relocations={0x1000: 5, 0x1004: 6, 0x1008: 4, 0x3008: 2})
        retail.word = lambda address: int.from_bytes(retail.bytes(address, address + 4), 'little')
        rows = [(0x1000, 'caller', 16, True), (0x3000, '__vt__4Base', 12, False),
                (0x5000, 'callback', 4, True)]
        pieces = NS(unit=lambda unit: [('.vtables', [('__vt__4Base', 0x3000, 0x3010)])])
        kwargs = dict(retail=retail, rows=rows, pieces=pieces,
                      addresses={name: start for start, name, _size, _function in rows})
        return recipient, donor, kwargs

    def apply(self, fixture):
        recipient, donor, kwargs = fixture
        entry = n.Donor('__vt__4Base', 'producer', Path('objdiff/base/producer.cpp.o'), b'raw', b'')
        with patch.dict(n.TABLE_PRODUCERS, {'owner': {'__vt__4Base': 'producer'}}), \
             patch.object(n.p, 'Elf', return_value=donor), patch.object(n.p, 'name_sections'), \
             patch.object(n.p, 'name_literal_data'):
            n.import_vtables(recipient, 'owner', (entry,), **kwargs)

    def test_import_preserves_code_indices_bytes_and_donor(self):
        fixture = self.fixture()
        recipient, donor, _kwargs = fixture
        before = d.code_snapshot(recipient)
        native = copy.deepcopy(donor.sections[1].data)
        self.apply(fixture)
        self.assertEqual(d.code_snapshot(recipient), before)
        self.assertEqual(donor.sections[1].data, native)
        self.assertEqual(recipient.sections[2].data, bytes(12))
        self.assertEqual(recipient.sections[2].sh_flags, 3)
        self.assertEqual(recipient.sections[2].sh_addralign, 1)
        self.assertEqual(recipient.symtab.symbols[1].st_shndx, 2)
        self.assertEqual(recipient.relocations[1].relocations[0].symbol_index, 2)

    def test_native_storage_initializer_and_relocations_fail_closed(self):
        for invalid in ('missing', 'duplicate', 'alias', 'placeholder', 'extent', 'binding',
                        'flags', 'section', 'type', 'alignment', 'payload', 'target',
                        'target_type', 'target_offset', 'relocation', 'duplicate_site',
                        'missing_site', 'retail_site', 'retail_size', 'recipient'):
            with self.subTest(invalid=invalid):
                fixture = self.fixture()
                recipient, donor, kwargs = fixture
                section, table = donor.sections[1], donor.symtab.symbols[0]
                if invalid == 'missing': table.st_shndx = 0
                elif invalid == 'duplicate': donor.symtab.symbols.append(copy.deepcopy(table))
                elif invalid == 'alias': donor.symtab.symbols.append(symbol('alias', 1))
                elif invalid == 'placeholder': table.name += '__DATA'
                elif invalid == 'extent': table.st_size = 8
                elif invalid == 'binding': table.bind = 2
                elif invalid == 'flags': section.sh_flags = 6
                elif invalid == 'section': section.name = '.data'
                elif invalid == 'type': section.sh_type = 8
                elif invalid == 'alignment': section.sh_addralign = 3
                elif invalid == 'payload': section.data = b'X' + section.data[1:]
                elif invalid == 'target': donor.symtab.symbols[2].name = 'unknown'
                elif invalid == 'target_type': donor.symtab.symbols[2].type = 1
                elif invalid == 'target_offset': donor.symtab.symbols[2].st_value = 4
                elif invalid == 'relocation': donor.relocations[0].relocations[0].reloc_type = 4
                elif invalid == 'duplicate_site':
                    donor.relocations[0].relocations.append(copy.deepcopy(donor.relocations[0].relocations[0]))
                elif invalid == 'missing_site': donor.relocations[0].relocations = []
                elif invalid == 'retail_site': kwargs['retail'].relocations.pop(0x3008)
                elif invalid == 'retail_size': kwargs['rows'][1] = (0x3000, '__vt__4Base', 8, False)
                else: recipient.symtab.symbols[1].st_shndx = 1
                before = d.code_snapshot(recipient)
                count = len(recipient.sections)
                with self.assertRaises(ValueError): self.apply(fixture)
                self.assertEqual(d.code_snapshot(recipient), before)
                self.assertEqual(len(recipient.sections), count)

    def test_every_complete_consumer_is_required(self):
        for invalid in ('instruction', 'extent', 'call', 'unknown', 'data', 'orphan', 'duplicate'):
            with self.subTest(invalid=invalid):
                fixture = self.fixture()
                _recipient, donor, _kwargs = fixture
                if invalid == 'instruction': donor.sections[2].data = donor.sections[2].data[:-4] + b'FAIL'
                elif invalid == 'extent': donor.symtab.symbols[1].st_size = 12
                elif invalid == 'call': donor.sections[2].data = donor.sections[2].data[:8] + struct.pack('<II', 0x0c000001, 0)
                elif invalid == 'unknown': donor.symtab.symbols[1].name = 'unknown'
                elif invalid == 'data': donor.sections[2].name = '.data'
                elif invalid == 'orphan': donor.relocations[1].relocations.pop(0)
                else: donor.relocations[1].relocations.append(copy.deepcopy(donor.relocations[1].relocations[0]))
                with self.assertRaisesRegex(ValueError, 'consumer'): self.apply(fixture)

    def test_source_only_provenance_and_freshness(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, base = root / 'producer.cpp', root / 'objdiff/base'
            base.mkdir(parents=True)
            lay = NS(source=lambda unit: source)
            with patch.dict(n.TABLE_PRODUCERS, {'owner': {'__vt__4Base': 'producer'}}):
                # A held table does not require a donor and gains no native credit.
                self.assertEqual(n.donor_inputs('owner', 'INCLUDE_RODATA("dir", __vt__4Base__DATA);', base, lay=lay), ())
                source.write_text('')
                with self.assertRaisesRegex(ValueError, 'missing'): n.donor_inputs('owner', '', base, lay=lay)
                native = base / 'producer.cpp.o'
                native.write_bytes(b'raw')
                self.assertEqual(n.donor_inputs('owner', '', base, lay=lay)[0].raw, b'raw')
                with self.assertRaisesRegex(ValueError, 'raw objdiff'): n.donor_inputs('owner', '', root / 'obj', lay=lay)
                source.write_text('INCLUDE_RODATA("dir", __vt__4Base__DATA);')
                with self.assertRaisesRegex(ValueError, 'assembly'): n.donor_inputs('owner', '', base, lay=lay)
                source.write_text('// changed source')
                with self.assertRaisesRegex(ValueError, 'stale'): n.donor_inputs('owner', '', base, lay=lay)

    def test_comparison_cache_includes_raw_donor_and_provenance(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source, cpp, output = root / 'input.o', root / 'owner.cpp', root / 'output.o'
            source.write_bytes(b'raw recipient')
            cpp.write_text('')
            ctx = NS(fingerprint='fixture', layout=NS(source=lambda unit: cpp))
            entry = n.Donor('__vt__4Base', 'producer', Path('objdiff/base/producer.cpp.o'), b'one', b'cpp')
            compiled = NS(sections=[], symtab=NS(symbols=[]), relocations=[], pack=lambda: b'prepared')
            with patch.object(d.p, 'Elf', return_value=compiled), patch.object(d.p, 'name_sections'), \
                 patch.object(n, 'donor_inputs', return_value=(entry,)) as inputs, \
                 patch.object(d, 'prepare_native_data') as prepare:
                d.comparison_copy(source, output, 'owner', ctx, True)
                d.comparison_copy(source, output, 'owner', ctx, True)
                self.assertEqual(prepare.call_count, 1)
                inputs.return_value = (entry._replace(raw=b'two'),)
                d.comparison_copy(source, output, 'owner', ctx, True)
                self.assertEqual(prepare.call_count, 2)
                inputs.return_value = (entry._replace(raw=b'two', source=b'changed cpp'),)
                d.comparison_copy(source, output, 'owner', ctx, True)
                self.assertEqual(prepare.call_count, 3)

    def test_cache_inputs_have_unambiguous_component_boundaries(self):
        entry = n.Donor('__vt__4Base', 'producer', Path('objdiff/base/producer.cpp.o'), b'a', b'bc')
        self.assertNotEqual(n.donor_fingerprint((entry,)),
                            n.donor_fingerprint((entry._replace(raw=b'ab', source=b'c'),)))

    def test_import_rechecks_provenance_and_duplicate_inputs(self):
        for invalid in ('path', 'marker', 'producer', 'duplicate'):
            fixture = self.fixture()
            recipient, donor, kwargs = fixture
            entry = n.Donor('__vt__4Base', 'producer', Path('objdiff/base/producer.cpp.o'), b'raw', b'')
            if invalid == 'path': entry = entry._replace(path=Path('obj/producer.cpp.o'))
            elif invalid == 'marker': entry = entry._replace(source=b'INCLUDE_RODATA("dir", __vt__4Base__DATA);')
            elif invalid == 'producer': entry = entry._replace(producer='other')
            entries = (entry, entry) if invalid == 'duplicate' else (entry,)
            with self.subTest(invalid=invalid), \
                 patch.dict(n.TABLE_PRODUCERS, {'owner': {'__vt__4Base': 'producer'}}), \
                 patch.object(n.p, 'Elf', return_value=donor), patch.object(n.p, 'name_sections'), \
                 patch.object(n.p, 'name_literal_data'):
                with self.assertRaises(ValueError):
                    n.import_vtables(recipient, 'owner', entries, **kwargs)
            self.assertEqual(len(recipient.sections), 2)


if __name__ == '__main__':
    unittest.main()
