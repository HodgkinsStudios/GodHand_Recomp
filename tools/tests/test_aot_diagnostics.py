import sys
import unittest
from pathlib import Path
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from aot_diagnostics import analyze


class AotDiagnosticsTests(unittest.TestCase):
    def test_identifies_unhandled_guest_opcodes_even_when_exit_was_zero(self):
        log = '''Recompilation completed successfully
Unhandled instructions: 3
  [error] unhandled-instruction function=sub_003C01D8 addr=0x3f4b4c - Unhandled SPECIAL instruction: 0xE raw=0xf000e
  [error] unhandled-instruction function=sub_003C01D8 addr=0x3f4b60 - Unhandled SPECIAL instruction: 0x1C raw=0x460c5c
  [error] unhandled-instruction function=sub_00100008 addr=0x100024 - Unhandled REGIMM instruction: 0x4 raw=0x4040490
'''
        result = analyze(log, ['output/a.cpp', 'output/b.cpp'])
        self.assertEqual(result['unhandled_events_observed'], 3)
        self.assertEqual(result['generated_translation_units'], 2)
        self.assertTrue(result['has_unhandled_instructions'])
        self.assertEqual(result['most_affected_functions'][0]['count'], 2)
        self.assertEqual(result['most_affected_functions'][0]['first_pc'], '0x003F4B4C')

    def test_absent_errors_is_not_playability_claim(self):
        r = analyze('Recompilation completed successfully\nUnhandled instructions: 0\n')
        self.assertFalse(r['has_unhandled_instructions'])
        self.assertEqual(r['unhandled_events_observed'], 0)
        self.assertIsNone(r['generated_translation_units'])


if __name__ == '__main__':
    unittest.main()
