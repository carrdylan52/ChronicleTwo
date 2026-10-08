"""Data padding must preserve object sizes and referenced retail boundaries."""
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import postprocess_object as p


class DataPaddingTests(unittest.TestCase):
    def run_padding(self, size=12, declared=12, end=16, nobits=True, tail=b"\0" * 4, terminal=False):
        section = SimpleNamespace(sh_type=p.SHT_NOBITS if nobits else 1,
                                  sh_size=size, data=b"x" * size,
                                  name=".bss" if nobits else ".data")
        symbol = SimpleNamespace(type=p.STT_OBJECT, st_value=0, st_shndx=1,
                                 name="object", st_size=size)
        elf = SimpleNamespace(sections=[None, section],
                              symtab=SimpleNamespace(symbols=[symbol]))
        run = [("object", 0, end)]
        if not terminal:
            run.append(("following", end, end + 4))
        pieces = SimpleNamespace(unit=lambda unit: [(section.name, run)])
        retail = SimpleNamespace(bytes=lambda start, end: tail)
        with patch.object(p.disassemble, "Pieces", return_value=pieces), \
             patch.object(p.layout, "Retail", return_value=retail), \
             patch.object(p.layout, "read_symbols", return_value=[(0, "object", declared, False)]):
            p.pad_data(elf, "test", set())
            once = p.section_size(section)
            p.pad_data(elf, "test", set())
            self.assertEqual(p.section_size(section), once)
        return once

    def test_exact_object_acquires_only_its_piece_tail(self):
        self.assertEqual(self.run_padding(), 16)
        self.assertEqual(self.run_padding(end=14, tail=b"\0" * 2), 14)

    def test_wrong_object_size_is_not_hidden(self):
        self.assertEqual(self.run_padding(size=8), 8)
        self.assertEqual(self.run_padding(size=13), 13)

    def test_large_or_negative_gaps_are_not_padding(self):
        self.assertEqual(self.run_padding(end=28), 12)
        self.assertEqual(self.run_padding(end=8), 12)

    def test_initialized_tail_must_be_retail_zero(self):
        self.assertEqual(self.run_padding(nobits=False), 16)
        self.assertEqual(self.run_padding(nobits=False, tail=b"\0\0\1\0"), 12)

    def test_terminal_padding_belongs_to_linker(self):
        self.assertEqual(self.run_padding(terminal=True), 12)


if __name__ == "__main__":
    unittest.main()
