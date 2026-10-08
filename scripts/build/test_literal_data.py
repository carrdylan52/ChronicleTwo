"""Anonymous data identities require exact extents and real retail references."""

import struct
from types import SimpleNamespace as NS
import unittest
from unittest.mock import patch

import postprocess_object as p


def symbol(name, section, size=0, value=0, bind=0, kind=p.STT_OBJECT):
    return NS(name=name, st_shndx=section, st_size=size, st_value=value,
              bind=bind, type=kind, st_name=0)


def relocation(offset, kind, target):
    return NS(r_offset=offset, reloc_type=kind, symbol_index=target)


class AnonymousBssTests(unittest.TestCase):
    def fixture(self, small=False):
        kind = '.sbss' if small else '.bss'
        code = struct.pack('<II', 0x3c020000, 0x24420000)
        refs = [relocation(0, p.R_MIPS_HI16, 0), relocation(4, p.R_MIPS_LO16, 0)]
        words = {0x1000: 0x3c020000, 0x1004: 0x24423000}
        kinds = {0x1000: p.R_MIPS_HI16, 0x1004: p.R_MIPS_LO16}
        if small:
            code = struct.pack('<I', 0x27820000)
            refs = [relocation(0, p.R_MIPS_GPREL16, 0)]
            words = {0x1000: 0x27821000}
            kinds = {0x1000: p.R_MIPS_GPREL16}
        elf = NS(sections=[None, NS(name=kind, sh_type=p.SHT_NOBITS, sh_size=8, data=b''),
                           NS(name='.text', data=code)],
                 symtab=NS(symbols=[symbol('at_999', 1, 8),
                                   symbol('caller', 2, kind=p.STT_FUNC)]),
                 relocations=[NS(sh_info=2, relocations=refs)],
                 strtab=NS(add_symbol=lambda name: len(name)))
        pieces = NS(layout=NS(sections=lambda unit: []),
                    unit=lambda unit: [('.text', [('caller', 0x1000, 0x1008)]),
                                       (kind, [('at_1__2', 0x3000, 0x3010),
                                               ('at_2', 0x3010, 0x3020)])])
        retail = NS(relocations=kinds, word=words.__getitem__)
        rows = [(0x3000, 'at_1__2', 8, False), (0x3010, 'at_2', 8, False)]
        return elf, pieces, retail, rows

    def apply(self, fixture, placeholders=()):
        elf, pieces, retail, rows = fixture
        with patch.object(p.layout, 'Retail', return_value=retail), \
             patch.object(p.disassemble, 'Pieces', return_value=pieces), \
             patch.object(p, 'retail_addresses', return_value={'_gp': 0x2000}), \
             patch.object(p.layout, 'read_symbols', return_value=rows):
            p.name_literal_data(elf, 'unit', set(placeholders))
        return elf.symtab.symbols[0].name

    def test_exact_extent_and_hi_lo_evidence(self):
        fixture = self.fixture()
        before = fixture[0].sections[2].data
        self.assertEqual(self.apply(fixture), 'at_1__2')
        self.assertEqual(fixture[0].sections[2].data, before)
        self.assertEqual(fixture[0].sections[1].sh_size, 8)

    def test_gp_relative_evidence(self):
        self.assertEqual(self.apply(self.fixture(small=True)), 'at_1__2')

    def test_wrong_declared_size(self):
        fixture = self.fixture()
        fixture[3][0] = (0x3000, 'at_1__2', 4, False)
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_candidate_must_fit_piece(self):
        fixture = self.fixture()
        fixture[0].sections[1].sh_size = 17
        fixture[0].symtab.symbols[0].st_size = 17
        fixture[3][0] = (0x3000, 'at_1__2', 17, False)
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_missing_evidence(self):
        fixture = self.fixture()
        fixture[0].relocations = []
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_missing_retail_relocation(self):
        fixture = self.fixture()
        fixture[2].relocations.clear()
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_mismatched_opcode(self):
        fixture = self.fixture()
        fixture[0].sections[2].data = struct.pack('<II', 0x3c030000, 0x24420000)
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_missing_low_partner(self):
        fixture = self.fixture()
        fixture[0].relocations[0].relocations.pop()
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_interior_alias(self):
        fixture = self.fixture()
        fixture[0].symtab.symbols.append(symbol('interior', 1, value=4))
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_multiple_objects(self):
        fixture = self.fixture()
        fixture[0].symtab.symbols.append(symbol('other', 1, 8))
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_outgoing_relocation(self):
        fixture = self.fixture()
        fixture[0].relocations.append(NS(sh_info=1, relocations=[relocation(0, p.R_MIPS_32, 1)]))
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_global_storage(self):
        fixture = self.fixture()
        fixture[0].symtab.symbols[0].bind = 1
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_placeholder_is_untouched(self):
        self.assertEqual(self.apply(self.fixture(), [1]), 'at_999')

    def test_section_extent_must_equal_object(self):
        fixture = self.fixture()
        fixture[0].sections[1].sh_size = 12
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_conflicting_reference(self):
        fixture = self.fixture()
        elf, pieces, retail, _rows = fixture
        elf.sections[2].data += struct.pack('<II', 0x3c020000, 0x24420000)
        elf.relocations[0].relocations.extend([
            relocation(8, p.R_MIPS_HI16, 0), relocation(12, p.R_MIPS_LO16, 0)])
        retail.relocations.update({0x1008: p.R_MIPS_HI16, 0x100c: p.R_MIPS_LO16})
        old_word = retail.word
        retail.word = lambda addr: {0x1008: 0x3c020000, 0x100c: 0x24423010}.get(addr, 0) \
            if addr >= 0x1008 else old_word(addr)
        self.assertEqual(self.apply(fixture), 'at_999')


class LiteralPointerTests(unittest.TestCase):
    def fixture(self, addend=0, target_offset=0):
        literal = symbol('at_999', 1, 1)
        target = literal
        symbols = [literal, symbol('table', 2, 4, bind=1)]
        if target_offset:
            target = symbol('alias', 1, value=target_offset, kind=p.STT_SECTION)
            symbols.append(target)
        elf = NS(sections=[None, NS(name='.rodata', sh_type=1, data=b'\0'),
                           NS(name='.data', sh_type=1, data=struct.pack('<I', addend))],
                 symtab=NS(symbols=symbols),
                 relocations=[NS(sh_info=2, relocations=[relocation(0, p.R_MIPS_32,
                                                                  symbols.index(target))])],
                 strtab=NS(add_symbol=lambda name: len(name)))
        pieces = NS(layout=NS(sections=lambda unit: [('.rodata', 0x3000, 0x3010)]),
                    unit=lambda unit: [('.rodata', [('at_1', 0x3000, 0x3008),
                                                   ('at_2', 0x3008, 0x3010)]),
                                       ('.data', [('table', 0x4000, 0x4004)])])
        retail = NS(relocations={0x4000: p.R_MIPS_32},
                    bytes=lambda lo, hi: bytes(hi - lo),
                    word=lambda address: 0x3008 + addend + target_offset)
        return elf, pieces, retail, []

    def apply(self, fixture):
        elf, pieces, retail, rows = fixture
        with patch.object(p.layout, 'Retail', return_value=retail), \
             patch.object(p.disassemble, 'Pieces', return_value=pieces), \
             patch.object(p, 'retail_addresses', return_value={'table': 0x4000}), \
             patch.object(p.layout, 'read_symbols', return_value=rows):
            p.name_literal_data(elf, 'unit', set())
        return elf.symtab.symbols[0].name

    def test_empty_literal_is_identified_by_native_pointer(self):
        fixture = self.fixture()
        pointer_bytes = fixture[0].sections[2].data
        self.assertEqual(self.apply(fixture), 'at_2')
        self.assertEqual(fixture[0].sections[1].data, bytes(8))
        self.assertEqual(fixture[0].sections[2].data, pointer_bytes)

    def test_addend_and_target_symbol_offset_are_subtracted(self):
        fixture = self.fixture(addend=3, target_offset=1)
        self.assertEqual(self.apply(fixture), 'at_2')
        self.assertEqual(fixture[0].sections[2].data, struct.pack('<I', 3))

    def test_inferred_pointer_has_no_identity_authority(self):
        fixture = self.fixture()
        fixture[2].relocations.clear()
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_wrong_relocation_kind_is_rejected(self):
        fixture = self.fixture()
        fixture[0].relocations[0].relocations[0].reloc_type = p.R_MIPS_HI16
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_unknown_or_anonymous_owner_is_not_an_anchor(self):
        for name in ('other', 'at_123__2'):
            fixture = self.fixture()
            fixture[0].symtab.symbols[1].name = name
            self.assertEqual(self.apply(fixture), 'at_999')

    def test_destination_must_be_exact_byte_candidate(self):
        fixture = self.fixture()
        fixture[2].word = lambda address: 0x3004
        self.assertEqual(self.apply(fixture), 'at_999')

    def test_conflicting_pointers_are_rejected(self):
        fixture = self.fixture()
        elf, _pieces, retail, _rows = fixture
        elf.sections[2].data += bytes(4)
        elf.relocations[0].relocations.append(relocation(4, p.R_MIPS_32, 0))
        retail.relocations[0x4004] = p.R_MIPS_32
        retail.word = lambda address: 0x3008 if address == 0x4000 else 0x3000
        self.assertEqual(self.apply(fixture), 'at_999')


if __name__ == '__main__':
    unittest.main()
