"""Linked objects must rebuild when the postprocessing steps change."""

from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import time
import unittest

ROOT = Path(__file__).resolve().parents[2]


def postprocess(revision):
    return ("import pathlib,sys\n"
            f"pathlib.Path(sys.argv[1]).write_text('postprocessed {revision}')\n")


@unittest.skipUnless(shutil.which('cmake') and shutil.which('ninja'), 'CMake and Ninja required')
class ObjectGraphTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.write('src/first.cpp', '')
        self.write('src/second.cpp', '')
        self.write('scripts/build/postprocess_object.py', postprocess('1'))
        self.write('scripts/build/fixup_sections.sh', 'exit 0\n')
        self.write('compiler/mwccps2.py',
                   'import pathlib,sys\n'
                   'pathlib.Path(sys.argv[sys.argv.index("-o")+1]).write_bytes(b"compiled");pathlib.Path(pathlib.Path(sys.argv[-1]).stem+\".d\").write_text(\"x.o: \"+sys.argv[-1]+\"\\n\")\n')
        self.write('ObjectLists.cmake', (ROOT / 'ps2/cmake/ObjectLists.cmake').read_text())
        self.write('CMakeLists.txt', f'''cmake_minimum_required(VERSION 3.20)
project(graph NONE)
set(BUILD_DIR out)
set(SCRIPTS_DIR scripts)
set(INCLUDE_DIR include)
set(PYTHON "{sys.executable}")
set(SATANSFIDDLE none)
set(SATANSFIDDLE_CONFIG none)
set(SATANSFIDDLE_DEPENDENCIES "")
set(CC_FLAGS -c)
set(KZ_MWCC compiler/mwccps2.py)
set(KZ_MWCC_RUNNER "{sys.executable}")
set(KZ_LIB_INCLUDE_DIRS include/std)
include("{ROOT / 'tools/kazarov/cmake/Kazarov.cmake'}")
include(ObjectLists.cmake)
set(objects out/first.cpp.o out/second.cpp.o)
kz_object_dirs(${{objects}})
foreach(unit first second)
  add_cpp_object(out/${{unit}}.cpp.o src/${{unit}}.cpp ${{unit}})
endforeach()
add_custom_target(objects DEPENDS ${{objects}})
''')
        self.command('cmake', '-S', '.', '-B', 'out', '-G', 'Ninja')

    def write(self, name, text):
        path = self.root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text)

    def command(self, *args):
        result = subprocess.run(args, cwd=self.root, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def test_postprocessing_inputs_rebuild_linked_objects_without_recompiling(self):
        self.command('cmake', '--build', 'out', '--target', 'objects')
        self.assertNotIn('CC src/', self.command('ninja', '-C', 'out', '-n', 'objects'))
        self.assertEqual((self.root / 'out/objdiff/base/first.cpp.o').read_bytes(), b'compiled')

        time.sleep(0.01)
        self.write('scripts/build/postprocess_object.py', postprocess('2'))
        pending = self.command('ninja', '-C', 'out', '-n', 'objects')
        self.assertNotIn('CC src/', pending)
        for unit in ('first', 'second'):
            self.assertIn(f'POST src/{unit}.cpp', pending)

        self.command('cmake', '--build', 'out', '--target', 'objects')
        for unit in ('first', 'second'):
            self.assertEqual((self.root / f'out/{unit}.cpp.o').read_bytes(), b'postprocessed 2')
            self.assertEqual((self.root / f'out/objdiff/base/{unit}.cpp.o').read_bytes(), b'compiled')
        self.assertNotIn('POST src/', self.command('ninja', '-C', 'out', '-n', 'objects'))


BATCH_COMPILER = """import pathlib, sys
args = sys.argv[1:]
out = pathlib.Path(args[args.index("-o") + 1])
sources = [arg for arg in args if arg.endswith(".cpp")]
for source in sources:
    stem = pathlib.Path(source).stem
    target = out / (stem + ".o") if out.is_dir() else out
    target.write_text(" ".join(args[:args.index("-o")]) + " | " + " ".join(sources))
    (target.parent / (stem + ".d")).write_text(stem + ".o: " + source + "\\n")
"""


@unittest.skipUnless(shutil.which('cmake') and shutil.which('ninja'), 'CMake and Ninja required')
class WholeProgramGraphTests(unittest.TestCase):
    """One compile per flag set in the given order, a lone unit apart, one POST per unit."""

    units = ('first', 'second', 'lone', 'third', 'fourth')

    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        for unit in self.units:
            (self.root / 'src').mkdir(exist_ok=True)
            (self.root / f'src/{unit}.cpp').write_text('')
        (self.root / 'scripts/build').mkdir(parents=True)
        (self.root / 'scripts/build/postprocess_object.py').write_text(postprocess('1'))
        (self.root / 'scripts/build/fixup_sections.sh').write_text('exit 0\n')
        (self.root / 'compiler').mkdir()
        (self.root / 'compiler/mwccps2.py').write_text(BATCH_COMPILER)
        (self.root / 'ObjectLists.cmake').write_text((ROOT / 'ps2/cmake/ObjectLists.cmake').read_text())
        sources = "\n".join(f'set(CPP_SOURCE_{unit} src/{unit}.cpp)' for unit in self.units)
        (self.root / 'CMakeLists.txt').write_text(f'''cmake_minimum_required(VERSION 3.20)
project(graph NONE)
set(BUILD_DIR out)
set(SCRIPTS_DIR scripts)
set(INCLUDE_DIR include)
set(PYTHON "{sys.executable}")
set(SATANSFIDDLE none)
set(SATANSFIDDLE_CONFIG none)
set(SATANSFIDDLE_DEPENDENCIES "")
set(CC_FLAGS -c)
set(UNIT_CC_FLAGS_third -O2)
set(KZ_MWCC compiler/mwccps2.py)
set(KZ_MWCC_RUNNER "{sys.executable}")
set(KZ_LIB_INCLUDE_DIRS include/std)
include("{ROOT / 'tools/kazarov/cmake/Kazarov.cmake'}")
include(ObjectLists.cmake)
{sources}
set(objects "")
foreach(unit {' '.join(self.units)})
  list(APPEND objects out/obj/${{unit}}.cpp.o)
endforeach()
kz_object_dirs(${{objects}})
add_whole_program_objects(UNITS {' '.join(self.units)} ALONE lone)
add_custom_target(objects DEPENDS ${{objects}})
''')
        self.command('cmake', '-S', '.', '-B', 'out', '-G', 'Ninja')

    def command(self, *args):
        result = subprocess.run(args, cwd=self.root, text=True, capture_output=True)
        self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
        return result.stdout

    def compiles(self, text):
        return sorted(line.split('] ', 1)[1] for line in text.splitlines() if '] CC ' in line)

    def test_units_sharing_flags_compile_together_in_order(self):
        built = self.command('ninja', '-C', 'out', 'objects')
        self.assertEqual(self.compiles(built), [
            'CC 1 units in one run, from lone', 'CC 1 units in one run, from third',
            'CC 3 units in one run, from first'])
        base = self.root / 'out/objdiff/base'
        self.assertEqual((base / 'second.cpp.o').read_text(),
                         '-c -lang c++ -c -MD | src/first.cpp src/second.cpp src/fourth.cpp')
        self.assertTrue((base / 'third.cpp.o').read_text().startswith('-c -lang c++ -c -O2 -MD |'))
        for unit in self.units:
            self.assertEqual((self.root / f'out/obj/{unit}.cpp.o').read_text(), 'postprocessed 1')

    def test_touching_one_source_recompiles_only_its_run(self):
        self.command('ninja', '-C', 'out', 'objects')
        self.assertIn('no work to do', self.command('ninja', '-C', 'out', '-n', 'objects'))
        time.sleep(0.01)
        (self.root / 'src/fourth.cpp').touch()
        self.assertEqual(self.compiles(self.command('ninja', '-C', 'out', '-n', 'objects')),
                         ['CC 3 units in one run, from first'])


if __name__ == '__main__':
    unittest.main()
