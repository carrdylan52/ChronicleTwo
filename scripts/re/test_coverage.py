"""Keep coverage counts consistent with objdiff despite generated assembly."""

from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import coverage


class CoverageTests(unittest.TestCase):
    def test_scores_override_generated_files_and_source_spelling(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            source = root / 'src'
            source.mkdir()
            (source / 'unit.cpp').write_text('''
extern "C" void __sinit_unit_cpp() {}
#ifdef NONMATCHING
void draft() {}
#else
INCLUDE_ASM("unit", draft);
#endif
''')
            generated = root / 'matchings' / 'unit'
            generated.mkdir(parents=True)
            (generated / 'fuzzy.s').write_text('glabel fuzzy\n')
            report = {'units': [{'name': 'unit', 'functions': [
                {'name': 'exact', 'fuzzy_match_percent': 100.0},
                {'name': 'fuzzy', 'fuzzy_match_percent': 99.0},
                {'name': 'draft'},
                {'name': '__sinit_unit_cpp'},
                {'name': '.L00100000'},
            ]}]}
            with patch.object(coverage, 'SOURCES', source), patch.object(
                coverage, 'MATCHINGS', root / 'matchings', create=True
            ):
                statuses = {symbol: status for _, symbol, status in coverage.rows(report)}
            self.assertEqual(statuses, {
                'exact': 'matched', 'fuzzy': 'fuzzy',
                'draft': 'guarded_draft', '__sinit_unit_cpp': 'asm_only',
            })


if __name__ == '__main__':
    unittest.main()
