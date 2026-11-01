import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from observed_entry_recovery import collect_observed_targets, recovery


def make_elf(words, flags=5):
    blob = bytearray(0x300)
    blob[:6] = b'\x7fELF\x01\x01'
    struct.pack_into('<H', blob, 18, 8)
    struct.pack_into('<I', blob, 28, 52)
    struct.pack_into('<HH', blob, 42, 32, 1)
    struct.pack_into('<IIIIIIII', blob, 52, 1, 0x100, 0x3baf00,
                     0, 0x100, 0x100, flags, 4)
    struct.pack_into('<IIII', blob, 0x1b8, *words)
    return bytes(blob)


class ObservedEntryRecoveryTests(unittest.TestCase):
    def setUp(self):
        self.elf = make_elf((0x3c040040,0x3c050078,0x2484c9b4,0x080c8934))
        self.config = '[general]\nentry_points=["0x00100008"]\n'

    def test_real_boot_log_and_idempotence(self):
        log = '[guest-branch:missing-target] kind=IndirectCall op=JALR source=0x31db2c target=0x3bafb8 codeRegion=yes policy=1\n'
        updated, report = recovery(self.elf, self.config, log)
        self.assertIn('0x003BAFB8', updated)
        self.assertTrue(report['config_changed'])
        self.assertFalse(report['guest_instruction_bytes_patched'])
        same, again = recovery(self.elf, updated, log)
        self.assertEqual(same, updated)
        self.assertFalse(again['config_changed'])

    def test_refuses_nonexecutable_trace(self):
        log = '[guest-branch:missing-target] source=0x31db2c target=0x3bafb8 codeRegion=no\n'
        self.assertEqual(collect_observed_targets(log), [])
        with self.assertRaises(ValueError):
            recovery(self.elf, self.config, log)

    def test_refuses_target_outside_original_elf(self):
        log = '[guest-branch:missing-target] source=0x31db2c target=0x123456 codeRegion=yes\n'
        with self.assertRaises(ValueError):
            recovery(self.elf, self.config, log)

    def test_deduplication_and_alignment(self):
        log = '\n'.join([
            '[guest-branch:missing-target] source=0x31db2c target=0x3bafb8 codeRegion=yes',
            '[guest-branch:missing-target] source=0x31db2c target=0x3bafb8 codeRegion=yes',
            '[guest-branch:missing-target] source=0x31db2c target=0x3bafb9 codeRegion=yes',
            '[guest-branch:missing-target] source=0x31db2c target=0x31db2c codeRegion=yes'])
        self.assertEqual(len(collect_observed_targets(log)), 1)

    def test_ignores_unrelated_debug_lines(self):
        self.assertEqual(collect_observed_targets('[run:tick] target=0x3bafb8 codeRegion=yes\n'), [])

    def test_all_candidates_require_original_elf_validation(self):
        log = '\n'.join([
            '[guest-branch:missing-target] source=0x31db2c target=0x3bafb8 codeRegion=yes',
            '[guest-branch:missing-target] source=0x31db2c target=0x3bb000 codeRegion=yes'])
        with self.assertRaises(ValueError):
            recovery(self.elf, self.config, log)


if __name__ == '__main__':
    unittest.main()
