import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools'))
from aot_batch_build import generated_object_targets, partition


class BatchSelection(unittest.TestCase):
    def test_balanced_batches_with_no_duplicates_or_skips(self):
        items = [str(i) for i in range(11)]
        batches = [partition(items, i, 4) for i in range(1, 5)]
        self.assertEqual([len(x) for x in batches], [3, 3, 3, 2])
        self.assertEqual(sum(batches, []), items)

    def test_reject_invalid_batch(self):
        with self.assertRaises(ValueError):
            partition([1], 0, 4)

    def test_extract_and_sort_targets(self):
        with tempfile.TemporaryDirectory() as temp:
            tmp = Path(temp)
            src = tmp / 'generated'
            src.mkdir()
            build = tmp / 'build'
            build.mkdir()
            entries = []
            for item in ('c', 'a', 'b'):
                source = src / f'{item}.cpp'
                source.write_text('int foo() { return 0; }')
                target = f'CMakeFiles/game.dir/{item}.cpp.o'
                entries.append({'directory': str(build), 'file': str(source),
                                'command': f'c++ -std=c++17 -c {source} -o {target}'})
            (tmp / 'commands.json').write_text(json.dumps(entries))
            targets = generated_object_targets(tmp / 'commands.json', src, build)
            self.assertEqual(targets, [f'CMakeFiles/game.dir/{item}.cpp.o' for item in 'abc'])

    def test_does_not_compile_other_project_files(self):
        with tempfile.TemporaryDirectory() as temp:
            tmp = Path(temp)
            src = tmp / 'generated'; src.mkdir()
            build = tmp / 'build'; build.mkdir()
            entries = [
                {'directory': str(build), 'file': str(src / 'a.cpp'),
                 'arguments': ['g++', '-o', 'CMakeFiles/game.dir/a.cpp.o']},
                {'directory': str(build), 'file': str(tmp / 'runtime.cpp'),
                 'arguments': ['g++', '-o', 'CMakeFiles/game.dir/runtime.cpp.o']},
            ]
            (tmp / 'commands.json').write_text(json.dumps(entries))
            self.assertEqual(generated_object_targets(tmp / 'commands.json', src, build),
                             ['CMakeFiles/game.dir/a.cpp.o'])

    def test_real_ninja_objects_compiled_in_batches(self):
        with tempfile.TemporaryDirectory() as tmpdir:
            tmp = Path(tmpdir)
            src = tmp / 'generated'; src.mkdir()
            for i in range(5):
                (src / f'f{i}.cpp').write_text(f'int fn{i}() {{ return {i}; }}\n')
            (tmp / 'CMakeLists.txt').write_text('cmake_minimum_required(VERSION 3.16)\n'
                  'project(AotFixture CXX)\nset(CMAKE_CXX_STANDARD 17)\n'
                  'file(GLOB SRC generated/*.cpp)\nadd_library(game OBJECT ${SRC})\n')
            build = tmp / 'build'
            subprocess.run(['cmake', '-S', str(tmp), '-B', str(build), '-G', 'Ninja',
                            '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON'], check=True,
                           stdout=subprocess.DEVNULL, stderr=subprocess.PIPE)
            commands = build / 'compile_commands.json'
            script = ROOT / 'tools' / 'aot_batch_build.py'
            for batch in range(1, 4):
                result = subprocess.run([sys.executable, str(script),
                       '--compile-commands', str(commands), '--source-dir', str(src),
                       '--build-dir', str(build), '--batch', str(batch),
                       '--batches', '3', '--jobs', '2', '--log', str(tmp / f'b{batch}.log')],
                       stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
                self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertEqual(len(list(build.rglob('f*.cpp.o'))), 5)


if __name__ == '__main__':
    unittest.main()
