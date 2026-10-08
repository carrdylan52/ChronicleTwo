"""Check the project's patched temporary-source naming and isolation."""
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path
import sys
import subprocess
import shutil
import tempfile
from threading import Barrier

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/mwccgap'))

from mwccgap.mwccgap import replace_sinit, temporary_source
import setup_mwccgap


def test_temporary_source():
    barrier = Barrier(2)
    with tempfile.TemporaryDirectory() as directory:
        source = Path(directory) / 'event.cpp'

        def compile_source(content):
            with temporary_source(source) as temporary:
                path = Path(temporary.name)
                temporary.write(content)
                temporary.flush()
                barrier.wait(timeout=10)
                assert path.read_bytes() == content
                assert path.name == 'tmpevent.c'
                assert path.parent.parent == source.parent
                return path

        with ThreadPoolExecutor(max_workers=2) as pool:
            paths = list(pool.map(compile_source, (b'first', b'second')))
        assert paths[0].parent != paths[1].parent
        assert all(not path.parent.exists() for path in paths)

        with temporary_source(source) as temporary:
            assert Path(temporary.name).name == paths[0].name

        try:
            with temporary_source(source) as temporary:
                failed_path = Path(temporary.name)
                raise RuntimeError('compiler failure')
        except RuntimeError:
            pass
        assert not failed_path.parent.exists()

        # The compiler sees only a stable basename, independent of checkout path.
        other_directory = Path(directory) / 'other'
        other_directory.mkdir()
        with temporary_source(other_directory / source.name) as temporary:
            assert Path(temporary.name).name == paths[0].name


def test_patch_setup():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        module = root / 'tools/mwccgap'
        (module / 'mwccgap').mkdir(parents=True)
        (module / 'mwccgap.py').touch()
        paths = ('mwccgap/elf.py', 'mwccgap/mwccgap.py')
        for path in paths:
            original = subprocess.check_output(
                ['git', '-C', str(ROOT / 'tools/mwccgap'), 'show', 'HEAD:' + path])
            (module / path).write_bytes(original)
        (module / 'mwccgap/mwccgap.py').chmod(0o755)
        subprocess.run(['git', 'init', '-q', str(module)], check=True)
        subprocess.run(['git', '-C', str(module), 'add', '.'], check=True)
        patches = root / 'scripts/build/patches'
        patches.mkdir(parents=True)
        for name in ('mwccgap-elf.patch', 'mwccgap-repro.patch'):
            shutil.copy2(ROOT / 'scripts/build/patches' / name, patches / name)
        saved_root = setup_mwccgap.ROOT
        setup_mwccgap.ROOT = root
        try:
            # Fresh setup, then the already-prepared ELF-only state.
            setup_mwccgap.apply_patch('mwccgap-elf.patch')
            elf_bytes = (module / paths[0]).read_bytes()
            setup_mwccgap.apply_patch('mwccgap-elf.patch')
            setup_mwccgap.apply_patch('mwccgap-repro.patch')
            assert (module / paths[0]).read_bytes() == elf_bytes
            assert b'def temporary_source(' in (module / paths[1]).read_bytes()
            fixed_bytes = [(module / path).read_bytes() for path in paths]
            # Both reverse checks succeed on fully patched repeated setup.
            setup_mwccgap.apply_patch('mwccgap-elf.patch')
            setup_mwccgap.apply_patch('mwccgap-repro.patch')
            assert [(module / path).read_bytes() for path in paths] == fixed_bytes
        finally:
            setup_mwccgap.ROOT = saved_root


def test_initializer_names():
    for prefix in ('', '.p', '.mwcats_'):
        temporary = 'tmpevent.c'
        original = 'event.cpp'
        assert replace_sinit(prefix + '__sinit_' + temporary, temporary, original) == (
            prefix + '__sinit_' + original)
        assert replace_sinit(prefix + '__sinit_' + temporary.ljust(ord('t')),
                             temporary, original) == (
            prefix + '__sinit_' + original.ljust(ord('e')))


if __name__ == '__main__':
    test_temporary_source()
    test_initializer_names()
    test_patch_setup()
    print('mwccgap temporary-source checks passed')
