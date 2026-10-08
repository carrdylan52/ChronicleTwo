"""Guard the distinction between function symbols and exported switch labels."""

import unittest
import struct

from prepare_objdiff_target import switch_labels, set_function_sizes


class SwitchLabelTests(unittest.TestCase):
    def test_only_explicit_switch_labels_are_localized(self):
        source = """
glabel SetUp__7CSphidaFi
  jlabel .L002EEA54
glabel .L002EEB18
.L002EEC20:
glabel named_function
jlabel named_function
.word .L002EEA54
"""
        self.assertEqual(switch_labels(source), [".L002EEA54"])

    def test_comments_and_near_matches_are_not_symbols(self):
        source = """
# jlabel .L002EEA54
// jlabel .L002EEB18
jlabel .L002EEA54_suffix
jlabel .L002EEA5
jlabel .L002EEA540
"""
        self.assertEqual(switch_labels(source), [])

    def test_repeated_labels_have_one_metadata_adjustment(self):
        self.assertEqual(switch_labels(
            "  jlabel .L002EEB18\n\tjlabel .L002EEA54\njlabel .L002EEB18\n"),
            [".L002EEA54", ".L002EEB18"])


class FunctionExtentTests(unittest.TestCase):
    def object(self):
        data = bytearray(320)
        data[:6] = b'\x7fELF\x01\x01'
        struct.pack_into('<I', data, 32, 160)
        struct.pack_into('<HH', data, 46, 40, 4)
        data[64:96] = bytes(range(32))
        data[96:102] = b'\0func\0'
        struct.pack_into('<IIIBBH', data, 144, 1, 8, 0, 16, 0, 1)
        struct.pack_into('<10I', data, 200, 0, 1, 6, 0, 64, 32, 0, 0, 16, 0)
        struct.pack_into('<10I', data, 240, 0, 3, 0, 0, 96, 6, 0, 0, 1, 0)
        struct.pack_into('<10I', data, 280, 0, 2, 0, 0, 128, 32, 2, 0, 4, 16)
        return data

    def test_declared_extent_changes_only_function_metadata(self):
        data = self.object()
        expected = data[:]
        struct.pack_into('<I', expected, 152, 12)
        expected[156] = 18
        set_function_sizes(data, {'func': 12})
        self.assertEqual(data, expected)
        set_function_sizes(data, {'func': 12})
        self.assertEqual(data, expected)

    def test_unknown_labels_are_untouched(self):
        data = self.object()
        expected = data[:]
        set_function_sizes(data, {'other': 12})
        self.assertEqual(data, expected)

    def test_out_of_section_extent_is_rejected(self):
        with self.assertRaises(ValueError):
            set_function_sizes(self.object(), {'func': 25})


if __name__ == "__main__":
    unittest.main()
