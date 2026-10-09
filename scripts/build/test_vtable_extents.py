"""Vtable copies must supply the entire declared retail object before discarding."""

from types import SimpleNamespace as NS
import unittest
from unittest.mock import patch
import postprocess_object as p
from test_literal_data import symbol


class VtableExtentTests(unittest.TestCase):
    def fixture(self, size=16, held=False):
        name = '__vt__5Thing'
        native = NS(name='.vtables', sh_type=1, sh_flags=3, sh_name=0, data=bytes(size))
        held_section = NS(name='.data', sh_type=1, sh_flags=3, sh_name=0, data=bytes(16))
        syms = [symbol(name, 1, size)]
        sections = [NS(name='', sh_flags=0), native]
        if held:
            sections.append(held_section)
            syms.append(symbol(name, 2, 16))
        elf = NS(sections=sections, symtab=NS(symbols=syms), relocations=[],
                 add_sh_symbol=lambda name: len(name))
        rows = [(0x3000, name, 16, False)]
        lay = NS(sections=lambda unit: [('.text', 0x1000, 0x1100)])
        retail = NS(bytes=lambda lo, hi: bytes(hi-lo), relocations={})
        return elf, rows, lay, retail

    def apply(self, fx, held=False, native_sizes=None):
        elf, rows, lay, retail = fx
        with patch.object(p.layout, 'Layout', return_value=lay), \
             patch.object(p.layout, 'read_symbols', return_value=rows), \
             patch.object(p, 'retail_addresses', return_value={row[1]:row[0] for row in rows}), \
             patch.object(p.layout, 'Retail', return_value=retail):
            if held:
                if native_sizes is None:
                    p.discard_shadow_vtables(elf, {2})
                else:
                    p.discard_shadow_vtables(elf, {2}, native_sizes=native_sizes)
            else:
                p.discard_external_vtables(elf, 'unit', set())

    def test_short_external_vtable_is_rejected_even_with_zero_retail_suffix(self):
        fx = self.fixture(size=12)
        with self.assertRaisesRegex(ValueError, 'extent'):
            self.apply(fx)
        self.assertNotEqual(fx[0].sections[1].name, p.DEAD)
        self.assertEqual(fx[0].symtab.symbols[0].st_shndx, 1)

    def test_short_shadow_vtable_is_rejected_even_with_zero_retail_suffix(self):
        fx = self.fixture(size=12, held=True)
        with self.assertRaisesRegex(ValueError, 'extent'):
            self.apply(fx, held=True)
        self.assertNotEqual(fx[0].sections[1].name, p.DEAD)

    def test_exact_declared_external_and_shadow_vtables_are_discarded(self):
        for held in (False, True):
            with self.subTest(held=held):
                fx = self.fixture(held=held)
                self.apply(fx, held=held)
                self.assertEqual(fx[0].sections[1].name, p.DEAD)

    def test_missing_declared_vtable_extent_is_rejected(self):
        for held in (False, True):
            with self.subTest(held=held):
                fx = self.fixture(held=held)
                fx[1][0] = (0x3000, '__vt__5Thing', 0, False)
                with self.assertRaisesRegex(ValueError, 'extent'):
                    self.apply(fx, held=held)

    def test_padding_cannot_repair_an_originally_short_shadow_vtable(self):
        fx = self.fixture(held=True)
        native_sizes = {id(fx[0].symtab.symbols[0]):12}
        with self.assertRaisesRegex(ValueError, 'extent'):
            self.apply(fx, held=True, native_sizes=native_sizes)

    def test_exact_original_shadow_vtable_may_retain_verified_piece_padding(self):
        fx = self.fixture(held=True)
        owner = fx[0].symtab.symbols[0]
        owner.st_size = 24
        fx[0].sections[1].data = fx[0].sections[2].data = bytes(24)
        self.apply(fx, held=True, native_sizes={id(owner):16})
        self.assertEqual(fx[0].sections[1].name, p.DEAD)
