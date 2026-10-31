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
