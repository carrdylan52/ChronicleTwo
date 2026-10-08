#!/usr/bin/env python3
"""Keep exported switch labels from splitting objdiff's function boundaries.

The linked assembly exports jlabels so separately assembled jump tables can
refer to them. An objdiff target contains the whole unit, so those labels can
be local without losing references. Only symbol metadata is changed.
"""

import argparse
from pathlib import Path
import re
import struct
import subprocess

import layout


def switch_labels(source):
    """Select splat's explicit jump labels, never ordinary function labels."""
    return sorted(set(re.findall(
        r"^[ \t]*jlabel[ \t]+(\.L[0-9A-Fa-f]{8})[ \t]*$", source, re.MULTILINE)))


def prepare_target(object_path, assembly_path, objcopy):
    labels = switch_labels(assembly_path.read_text())
    if labels:
        subprocess.run([
            objcopy,
            *[f"--localize-symbol={name}" for name in labels],
            str(object_path),
        ], check=True)
    data = bytearray(object_path.read_bytes())
    sizes = {name: size for _address, name, size, function in layout.read_symbols()
             if function and size}
    set_function_sizes(data, sizes)
    object_path.write_bytes(data)


def set_function_sizes(data, sizes):
    """Use declared retail function sizes, excluding linker alignment tails."""
    if data[:6] != b'\x7fELF\x01\x01':
        raise ValueError('Expected a little-endian ELF32 comparison object')
    section_offset = struct.unpack_from('<I', data, 32)[0]
    entry_size, count = struct.unpack_from('<HH', data, 46)
    sections = [struct.unpack_from('<10I', data, section_offset + i * entry_size)
                for i in range(count)]
    for section in sections:
        if section[1] != 2:  # SHT_SYMTAB
            continue
        strings = sections[section[6]]
        strings = data[strings[4]:strings[4] + strings[5]]
        for offset in range(section[4], section[4] + section[5], section[9]):
            name, value, old_size, info, _other, index = struct.unpack_from('<IIIBBH', data, offset)
            name = strings[name:strings.index(0, name)].decode()
            size = sizes.get(name)
            if size is None or not 0 < index < len(sections):
                continue
            owner = sections[index]
            if not owner[2] & 4 or info & 15 not in (0, 2):
                continue
            if value + size > owner[5] or old_size not in (0, size):
                raise ValueError(f'Invalid retail function extent for {name}')
            struct.pack_into('<I', data, offset + 8, size)
            data[offset + 12] = (info & 0xf0) | 2  # STT_FUNC


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("object", type=Path)
    parser.add_argument("assembly", type=Path)
    parser.add_argument("--objcopy", default="mips-ps2-decompals-objcopy")
    args = parser.parse_args()
    prepare_target(args.object, args.assembly, args.objcopy)


if __name__ == "__main__":
    main()
