"""A discarded assembly-backed parent cannot discard a migrated native child."""

import struct
from types import SimpleNamespace as NS
import unittest
from unittest.mock import patch

import postprocess_object as p
from test_literal_data import symbol, relocation


class NativeChildTests(unittest.TestCase):
    def fixture(self, retained_child=False):
        sections = [NS(name='', sh_flags=0),
                    NS(name='.text', sh_flags=6, sh_name=0, sh_type=1,
                       data=struct.pack('<I', 0x27820000)),
                    NS(name='.data', sh_flags=2, sh_name=0, sh_type=1, data=bytes(4)),
                    NS(name='.rodata', sh_flags=2, sh_name=0, sh_type=1, data=b'X\0'),
                    NS(name='.data', sh_flags=2, sh_name=0, sh_type=1, data=bytes(4))]
        syms = [symbol('caller', 1, 4, bind=1, kind=p.STT_FUNC), symbol('at_99', 2, 4),
                symbol('at_98', 3, 2), symbol('table', 4, 4, bind=1)]
        held = {4}
        if retained_child:
            sections.append(NS(name='.rodata', sh_flags=2, sh_name=0, sh_type=1, data=b'X\0' + bytes(6)))
            syms.append(symbol('literal', 5, 8, bind=1))
            held.add(5)
        code_ref = relocation(0, p.R_MIPS_GPREL16, 1)
        elf = NS(sections=sections, symtab=NS(symbols=syms), add_sh_symbol=lambda name: len(name),
                 relocations=[NS(sh_info=1, sh_name=0, name='.rel.text', relocations=[code_ref]),
                              NS(sh_info=2, sh_name=0, name='.rel.data',
                                 relocations=[relocation(0, p.R_MIPS_32, 2)])])
        lay = NS(kinds={'unit': 'cpp'}, sections=lambda unit: [('.text', 0x1000, 0x1004),
                  ('.data', 0x3000, 0x3004), ('.rodata', 0x4000, 0x4008)])
        rows = [(0x2000, '_gp', 0, False), (0x1000, 'caller', 4, True),
                (0x3000, 'table', 4, False), (0x4000, 'literal', 2, False)]
        retail = NS(word={0x1000: 0x27821000, 0x3000: 0x4000}.__getitem__,
                    bytes=lambda lo, hi: b'X\0'[lo - 0x4000:hi - 0x4000])
        return elf, held, lay, rows, retail

    def apply(self, fixture):
        elf, held, lay, rows, retail = fixture
        with patch.object(p.layout, 'Layout', return_value=lay), \
             patch.object(p.layout, 'read_symbols', return_value=rows), \
             patch.object(p.layout, 'Retail', return_value=retail), \
             patch.object(p, 'rename_shared_names'):
            return p.bind_local_data(elf, 'unit', held)

    def test_native_child_without_marker_survives_discarded_parent(self):
        fixture = self.fixture()
        self.assertEqual(self.apply(fixture), ['at_99'])
        self.assertEqual(fixture[0].sections[2].name, p.DEAD)
        self.assertEqual(fixture[0].sections[3].name, '.rodata')
        self.assertEqual(fixture[0].sections[3].data, b'X\0')

    def test_child_with_retained_placeholder_still_discards_compiler_copy(self):
        fixture = self.fixture(retained_child=True)
        self.assertEqual(self.apply(fixture), ['at_99', 'at_98'])
        self.assertEqual(fixture[0].sections[3].name, p.DEAD)


if __name__ == '__main__':
    unittest.main()
