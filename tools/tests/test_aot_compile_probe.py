import sys
import tempfile
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from aot_compile_probe import choose_commands


class CompileProbeTests(unittest.TestCase):
    def test_prefers_boot_entry_and_registration_and_ignores_other_sources(self):
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / 'output'
            other = Path(td) / 'runtime.cpp'
            commands = [{'file': str(out / f), 'command': 'true', 'directory': td}
                        for f in ('sub_00300000_0x300000.cpp', 'register_functions.cpp',
                                  'sub_00100008_0x100008.cpp')]
            commands.insert(1, {'file': str(other), 'command': 'true', 'directory': td})
            selected = choose_commands(commands, out, 2)
            self.assertEqual([Path(c['file']).name for c in selected],
                             ['sub_00100008_0x100008.cpp', 'register_functions.cpp'])

    def test_rejects_similarly_named_directory_outside_generated_root(self):
        with tempfile.TemporaryDirectory() as td:
            root = Path(td) / 'out'
            rows = [{'file': str(Path(td) / 'out-evil' / 'entry.cpp'), 'arguments': ['true']}]
            self.assertEqual(choose_commands(rows, root), [])


if __name__ == '__main__':
    unittest.main()

class StratifiedCompilerTests(unittest.TestCase):
    def test_scans_across_address_space_and_known_suspects(self):
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / 'output'
            names = ['register_functions.cpp', 'sub_00100008_0x100008.cpp',
                     'entry_3b6990_0x3b6a00.cpp',
                     'sub_003C01D8_0x3c01d8.cpp', 'sub_003FDD40_0x3fdd40.cpp']
            names.extend(f'sub_{a:08X}_0x{a:x}.cpp' for a in range(0x110000, 0x310000, 0x10000))
            commands = [{'file': str(out / name), 'command': 'true', 'directory': td} for name in names]
            selected = [Path(c['file']).name for c in choose_commands(commands, out, 12)]
            self.assertEqual(selected[:3], names[:3][1:2] + names[:1] + names[2:3])
            self.assertIn('sub_003C01D8_0x3c01d8.cpp', selected)
            self.assertIn('sub_003FDD40_0x3fdd40.cpp', selected)
            self.assertEqual(len(selected), len(set(selected)))
            self.assertEqual(len(selected), 12)
            # No longer a pathological cluster of only the earliest functions.
            addresses = [int(m.group(1), 16) for name in selected
                         if (m := __import__('re').search(r'_0x([0-9a-fA-F]+)\.cpp$', name))]
            self.assertGreater(max(addresses) - min(addresses), 0x100000)

    def test_parallel_probe_retains_diagnostics_for_failed_sources(self):
        from aot_compile_probe import compile_probe
        with tempfile.TemporaryDirectory() as td:
            out = Path(td) / 'output'
            commands = [{'file': str(out / 'sub_00100008_0x100008.cpp'),
                         'arguments': ['python3', '-c', 'import sys; print("compiler error",file=sys.stderr); sys.exit(9)'],
                         'directory': td},
                        {'file': str(out / 'register_functions.cpp'),
                         'arguments': ['python3', '-c', 'print("ok")'], 'directory': td}]
            report = compile_probe(commands, out, max_files=2, workers=2)
            self.assertEqual(report['failed_translation_units'], 1)
            self.assertEqual(report['successful_translation_units'], 1)
            self.assertIn('compiler error', report['records'][0]['diagnostic'])
            self.assertFalse(report['native_game_linked'])
