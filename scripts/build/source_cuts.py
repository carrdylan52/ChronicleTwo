#!/usr/bin/env python3
"""Track only explicit source-address identities that affect the global split."""

import argparse
from pathlib import Path

from disassemble import source_addresses


def write_source_cuts(output, sources):
    """Preserve the output timestamp when source changes leave its cut set intact."""
    addresses = set()
    for source in sources:
        addresses.update(source_addresses(source.read_text(encoding='utf-8')))
    text = ''.join(f'D_{address:08X}\n' for address in sorted(addresses))
    if output.is_file() and output.read_text(encoding='utf-8') == text:
        return False
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(text, encoding='utf-8')
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('output', type=Path)
    parser.add_argument('sources', type=Path, nargs='+')
    args = parser.parse_args()
    write_source_cuts(args.output, args.sources)
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
