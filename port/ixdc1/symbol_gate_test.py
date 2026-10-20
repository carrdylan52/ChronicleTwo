"""Focused negative tests using actual COFF objects, not mocked nm output."""
import argparse
from pathlib import Path
import subprocess
import tempfile


def run(command):
    return subprocess.run([str(x) for x in command], capture_output=True, text=True)


def main():
    p = argparse.ArgumentParser()
    p.add_argument('--compiler', required=True)
    p.add_argument('--nm', required=True)
    p.add_argument('--ar', required=True)
    p.add_argument('--python', required=True)
    p.add_argument('--build', required=True)
    p.add_argument('--archive', required=True)
    p.add_argument('--binary', required=True)
    args = p.parse_args()
    checker = Path(__file__).with_name('check_symbols.py')
    with tempfile.TemporaryDirectory(prefix='ixdc1-gate-', dir=args.build) as temporary:
        root = Path(temporary)

        def compile_object(name, source):
            cpp, obj = root / (name + '.cpp'), root / (name + '.obj')
            cpp.write_text(source)
            result = run([args.compiler, '-c', cpp, '-o', obj])
            if result.returncode:
                raise SystemExit(result.stderr)
            return obj

        def bad_archive(name, source):
            obj = compile_object(name, source)
            archive = root / (name + '.a')
            result = run([args.ar, 'rcs', archive, obj])
            if result.returncode:
                raise SystemExit(result.stderr)
            return archive

        for name, source, reason in (
            ('global', 'extern "C" { int GamePad = 42; }', "leaks=['GamePad']"),
            ('binding', 'extern "C" void MoveCheck(); extern "C" void ixion_dc1_toan_probe() { MoveCheck(); }',
             "foreign references=['MoveCheck']"),
        ):
            archive = bad_archive(name, source)
            result = run([args.python, '-B', checker, '--nm', args.nm, '--archive', archive])
            if result.returncode == 0 or reason not in result.stderr:
                raise SystemExit(f'FAIL {name} was not rejected: {result.stdout}{result.stderr}')
            print(f'PASS gate rejects {name}: {reason}')

        collision = compile_object('collision', 'extern "C" { unsigned char ixdc1_CharaMain = 0; }')
        objects = root / 'host.txt'
        objects.write_text(str(collision) + '\n')
        result = run([args.python, '-B', checker, '--nm', args.nm, '--archive', args.archive,
                      '--binary', args.binary, '--host-objects', objects])
        if result.returncode == 0 or 'donor/host object collisions' not in result.stderr:
            raise SystemExit(f'FAIL collision was not rejected: {result.stdout}{result.stderr}')
        print('PASS gate rejects prefixed host collision: ixdc1_CharaMain')
    print('PASS symbol gate negative tests: 3/3')


if __name__ == '__main__':
    main()
