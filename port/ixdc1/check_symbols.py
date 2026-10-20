"""Reject donor leaks/collisions even when DC2's linker accepts duplicates."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def symbols(nm, paths, defined=False):
    args = [nm, '--extern-only', '--format=posix']
    if defined:
        args.append('--defined-only')
    result = subprocess.run(args + [str(p) for p in paths], capture_output=True, text=True, check=True)
    rows = []
    for line in result.stdout.splitlines():
        parts = line.split()
        if len(parts) >= 2 and len(parts[1]) == 1:
            rows.append((parts[0], parts[1]))
    return rows


def private(name):
    if name.startswith('.refptr.'):
        name = name[len('.refptr.'):]
    return name.startswith(('ixdc1_', 'ixion_dc1_', '_ZN5ixdc1', '_ZNK5ixdc1',
                            '_ZTVN5ixdc1', '_ZTIN5ixdc1', '_ZTSN5ixdc1'))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--nm', required=True)
    parser.add_argument('--archive', required=True)
    parser.add_argument('--binary')
    parser.add_argument('--host-objects')
    parser.add_argument('--donor')
    parser.add_argument('--helpers')
    args = parser.parse_args()
    rows = symbols(args.nm, [args.archive])
    definitions = {name for name, kind in rows if kind != 'U'}
    strong = {name for name, kind in rows if kind not in ('U', 'w', 'v', 'W', 'V')}
    # LLVM/CRT-generated symbols are not game symbols. Undefined references are
    # separately constrained so a forgotten shim cannot bind to a DC2 method.
    runtime = {'atan2f', 'sinf', 'cosf', 'sqrtf', 'memcpy', 'memset', 'strcmp',
               '__stack_chk_fail', '__stack_chk_guard', '__chkstk', 'sincosf'}
    leaks = sorted(name for name in strong if not private(name))
    unresolved = {name for name, kind in rows if kind == 'U'} - definitions
    bad_refs = sorted(name for name in unresolved if not private(name) and name not in runtime)
    if leaks or bad_refs:
        raise SystemExit(f'FAIL donor symbols: leaks={leaks}, foreign references={bad_refs}')
    if args.donor:
        manifest = json.loads(Path(args.helpers).read_text())
        charakey = (Path(args.donor) / 'ps2/src/charakey.cpp').read_bytes()
        if hashlib.sha256(charakey).hexdigest() != manifest['charakey_sha256']:
            raise SystemExit('FAIL donor charakey.cpp drift from the reviewed source')
        for item in manifest['snippets']:
            data = (Path(args.donor) / item['path']).read_bytes()
            if item.get('match') == 'lines':
                # Xiao R1: BattleActionPlay_Jinn's #ifdef PAL_TIMING opens one of two `if (...) {` lines, so its
                # braces do not balance as text. Its whole-line span is hashed, and must start with the signature.
                lines = data.splitlines(keepends=True)
                span = b''.join(lines[item['start'] - 1:item['end']]).rstrip(b'\r\n')
                if not span.startswith(item['signature'].encode()) or \
                        hashlib.sha256(span).hexdigest() != item['sha256']:
                    raise SystemExit(f'FAIL donor helper drift: {item["signature"]}')
                continue
            start = data.index(item['signature'].encode())
            brace = data.index(b'{', start)
            depth, end = 1, brace + 1
            while depth:
                depth += (data[end] == 123) - (data[end] == 125)
                end += 1
            # A data initialiser (element_tbl, MyEntryEffect_*) also takes its ';'.
            if not item['signature'].endswith('('):
                end += 1
            if hashlib.sha256(data[start:end]).hexdigest() != item['sha256']:
                raise SystemExit(f'FAIL donor helper drift: {item["signature"]}')
        copied = Path(args.helpers).with_name('helpers.inc').read_bytes()
        expected = []
        for item in manifest['snippets']:
            data = (Path(args.donor) / item['path']).read_bytes()
            lines = data.splitlines(keepends=True)
            expected.append(b''.join(lines[item['start'] - 1:item['end']]).rstrip(b'\r\n'))
        if copied != b'\n\n'.join(expected) + b'\n':
            raise SystemExit('FAIL copied helper bytes differ from donor')
        print(f'PASS provenance: charakey.cpp unchanged; {len(manifest["snippets"])} helper snippets byte-identical to donor')
    if args.binary:
        host_paths = Path(args.host_objects).read_text().splitlines()
        host_defs = set()
        # Batches keep the Windows command line below its size limit.
        for i in range(0, len(host_paths), 40):
            host_defs.update(name for name, _ in symbols(args.nm, host_paths[i:i+40], True))
        collisions = sorted(definitions & host_defs)
        if collisions:
            raise SystemExit(f'FAIL donor/host object collisions: {collisions}')
        binary_rows = symbols(args.nm, [args.binary], True)
        for required in ('ixion_dc1_toan_reset', 'ixion_dc1_toan_tick', 'ixion_dc1_reset', 'ixion_dc1_tick',
                         'ixion_dc1_character_info', 'ixion_dc1_set_point_resolver',
                         '_ZN5ixdc112ToanKey_PlayEv', '_ZN5ixdc110ToanKey_OnEv',  # gitleaks:allow - C++ mangled symbol
                         '_ZN5ixdc112GoroKey_PlayEv', '_ZN5ixdc110GoroKey_OnEv',  # gitleaks:allow - C++ mangled symbol
                         '_ZN5ixdc114UngagaKey_PlayEv', '_ZN5ixdc112UngagaKey_OnEv',  # gitleaks:allow - C++ mangled symbol
                         '_ZN5ixdc119BattleActionOn_JinnEv', '_ZN5ixdc121BattleActionPlay_JinnEPNS_10CCharacterEi'):
            if sum(name == required for name, _ in binary_rows) != 1:
                raise SystemExit(f'FAIL linked donor missing/duplicated: {required}')
        readobj = Path(args.nm).with_name('llvm-readobj.exe' if Path(args.nm).suffix == '.exe' else 'llvm-readobj')
        exports = subprocess.check_output([str(readobj), '--coff-exports', args.binary], text=True)
        names = [line.split(':', 1)[1].strip() for line in exports.splitlines() if line.strip().startswith('Name:')]
        if names != ['ixion_get_api']:
            raise SystemExit(f'FAIL DLL exports: {names}')
        print(f'PASS linked DLL: {len(host_paths)} host object/archive inputs; 0 donor collisions; Toan/Goro/Ungaga/Xiao boundary and handlers retained; only ixion_get_api exported')
    print(f'PASS donor archive: {len(strong)} private strong symbols; 0 leaked definitions; 0 foreign game references')


if __name__ == '__main__':
    main()
