"""Check stable BSS selection and optionally an actual dng_main compiler object.

    python3 scripts/build/test_static_bss.py
    python3 scripts/build/test_static_bss.py --object build/pal/obj/dng_main.cpp.o
    python3 scripts/build/test_static_bss.py --raw-object /tmp/dng_main.cpp.o

The object check uses the genuine compiler output supplied by the caller. It
changes no file and constructs no replacement compiler object.
"""
import argparse
import re
from pathlib import Path

from postprocess_object import (
    DEAD, Elf, ROOT, SHF_ALLOC, SHT_PROGBITS, STT_SECTION, bind_named_static_bss, drop_placeholder_aliases,
    name_sections, project_name, section_size, static_bss_pairs,
)


SOURCE = '''
static mgCMemory debug_event_stack;
INCLUDE_BSS(debug_event_stack_1106, 0x30);
static sceVu0FVECTOR chk_pos;
INCLUDE_BSS(chk_pos_2870, 0x10);
'''
NATIVE = [(1, 'debug_event_stack_388', 48), (2, 'chk_pos_1961', 16)]
HELD = [(3, 'debug_event_stack_1106', 48), (4, 'chk_pos_2870', 16)]


def test_selection():
    assert static_bss_pairs(SOURCE, NATIVE, HELD) == [(1, 3), (2, 4)]
    # Neither spelling nor value of MWCC's changing suffix is an identity.
    renamed = [(1, 'debug_event_stack$999999', 48), (2, 'chk_pos_7', 16)]
    assert static_bss_pairs(SOURCE, renamed, HELD) == [(1, 3), (2, 4)]
    assert static_bss_pairs(SOURCE, [(1, 'debug_event_stack_1', 16)], HELD) == []
    assert static_bss_pairs(SOURCE, [(1, 'debug_event_stack_1', 0)], HELD) == []
    assert static_bss_pairs(SOURCE, [(1, 'different_1', 48)], HELD) == []
    assert static_bss_pairs(SOURCE, [(1, 'debug_event_stack', 48)], HELD) == []


def test_ambiguity():
    assert static_bss_pairs(SOURCE, NATIVE + [(5, 'chk_pos_2', 16)], HELD) == [(1, 3)]
    source = SOURCE + '\nINCLUDE_BSS(chk_pos_1234, 0x10);'
    held = HELD + [(5, 'chk_pos_1234', 16)]
    assert static_bss_pairs(source, NATIVE, held) == [(1, 3)]
    source = SOURCE + '\nstatic sceVu0FVECTOR chk_pos;'
    assert static_bss_pairs(source, NATIVE, HELD) == [(1, 3)]
    reused = ('void first() { static sceVu0FVECTOR chk_pos; }\n'
              'void second() { static sceVu0FVECTOR chk_pos; }\n'
              'INCLUDE_BSS(chk_pos_2870, 0x10);')
    assert static_bss_pairs(reused, NATIVE, HELD) == []
    assert static_bss_pairs('/* ' + SOURCE + ' */', NATIVE, HELD) == []
    assert static_bss_pairs('static void chk_pos();\nINCLUDE_BSS(chk_pos_2870,0x10);',
                            NATIVE, HELD) == []
    assert static_bss_pairs('static sceVu0FVECTOR chk_pos;', NATIVE, HELD) == []


def test_actual_object(data, raw=False):
    elf = Elf(data)
    name_sections(elf)
    placeholders = drop_placeholder_aliases(elf)
    for symbol in elf.symtab.symbols:
        if symbol.type != STT_SECTION and not symbol.name.startswith('.'):
            symbol.name = project_name(symbol.name)
    before = [(section.name, section_size(section), bytes(section.data))
              for section in elf.sections]
    references = [(relocation, relocation.symbol_index,
                   elf.symtab.symbols[relocation.symbol_index].st_shndx,
                   elf.symtab.symbols[relocation.symbol_index].st_value)
                  for record in elf.relocations for relocation in record.relocations]
    dropped = bind_named_static_bss(elf, 'dng_main', placeholders)
    assert any(name.startswith('debug_event_stack_') for name in dropped), dropped
    assert any(name.startswith('chk_pos_') for name in dropped), dropped
    if not raw:
        assert len(dropped) == 2, f'expected two unmatched native statics, found {dropped}'
    changed = []
    for index, section in enumerate(elf.sections):
        if not section.sh_flags & SHF_ALLOC:
            continue
        original_name, size, contents = before[index]
        assert section_size(section) == size
        assert bytes(section.data) == contents
        if section.name != original_name:
            assert section.name == DEAD
            changed.append(index)
    assert len(changed) == len(dropped)
    marker_names = set(re.findall(r'INCLUDE_BSS\(\s*([A-Za-z_]\w*)\s*,',
                                 (ROOT / 'ps2/src/dng_main.cpp').read_text()))
    changed_targets = []
    for relocation, original_index, original_section, original_value in references:
        if relocation.symbol_index != original_index:
            original = elf.symtab.symbols[original_index]
            target = elf.symtab.symbols[relocation.symbol_index]
            assert original_section in changed or original_section == 0
            assert target.name in marker_names
            assert target.st_value == original_value == 0
            changed_targets.append(target.name)
    if not raw:
        assert changed_targets.count('debug_event_stack_1106') == 6
        assert changed_targets.count('chk_pos_2870') == 2
    else:
        assert changed_targets.count('debug_event_stack_1106') >= 6
        assert changed_targets.count('chk_pos_2870') >= 2
    print(f'genuine dng_main: {len(changed)} sections, {len(changed_targets)} references; '
          'all bytes/addends unchanged')


def test_actual_rejections(data):
    # Mutate metadata from a genuine object in memory, without constructing or
    # writing any ELF object. Each unsafe variant must retain its native copy.
    for mutation in ('interior_alias', 'initialized_target', 'initialized_native', 'global_native'):
        elf = Elf(data)
        name_sections(elf)
        placeholders = drop_placeholder_aliases(elf)
        for symbol in elf.symtab.symbols:
            if symbol.type != STT_SECTION and not symbol.name.startswith('.'):
                symbol.name = project_name(symbol.name)
        native = next(symbol for symbol in elf.symtab.symbols
                      if symbol.bind == 0 and symbol.name.startswith('debug_event_stack_'))
        held = next(symbol for symbol in elf.symtab.symbols
                    if symbol.name == 'debug_event_stack_1106' and symbol.bind != 0)
        original = elf.sections[native.st_shndx]
        if mutation == 'interior_alias':
            alias = next(symbol for symbol in elf.symtab.symbols if symbol.type == STT_SECTION)
            alias.st_shndx = native.st_shndx
            alias.st_value = 1
        elif mutation == 'initialized_target':
            section = elf.sections[held.st_shndx]
            section.sh_type = SHT_PROGBITS
            section.data = b'\x01' + bytes(section_size(original) - 1)
        elif mutation == 'initialized_native':
            original.sh_type = SHT_PROGBITS
            original.data = bytes(native.st_size)
        elif mutation == 'global_native':
            native.bind = 1
        dropped = bind_named_static_bss(elf, 'dng_main', placeholders)
        assert native.name not in dropped, (mutation, dropped)
        assert original.name != DEAD, mutation
        assert any(name.startswith('chk_pos_') for name in dropped), mutation
    print('genuine object metadata: interior aliases and initialized/global storage rejected')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--object', type=Path)
    parser.add_argument('--raw-object', type=Path)
    args = parser.parse_args()
    test_selection()
    test_ambiguity()
    if args.object is not None:
        test_actual_object(args.object.read_bytes())
        test_actual_rejections(args.object.read_bytes())
    if args.raw_object is not None:
        test_actual_object(args.raw_object.read_bytes(), raw=True)
    print('static BSS checks passed')


if __name__ == '__main__':
    main()
