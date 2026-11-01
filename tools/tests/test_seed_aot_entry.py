import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from seed_aot_entry import inspect_elf_target, add_entry_hint, validate_and_seed_entries

def make_elf(words, flags=5):
    blob=bytearray(0x300)
    blob[:6]=b'\x7fELF\x01\x01'
    struct.pack_into('<H', blob, 18, 8)
    struct.pack_into('<I', blob, 28, 52)
    struct.pack_into('<HH', blob, 42, 32, 1)
    struct.pack_into('<IIIIIIII', blob, 52, 1, 0x100, 0x3baf00, 0, 0x100, 0x100, flags, 4)
    struct.pack_into('<IIII', blob, 0x1b8, *words)
    return bytes(blob)

class GuestEntryHintTests(unittest.TestCase):
    def test_real_elf_entry(self):
        self.assertTrue(inspect_elf_target(make_elf((0x3c040040,0x3c050078,0x2484c9b4,0x080c8934)),0x3bafb8)['first_instruction_lui'])
    def test_not_executable(self):
        with self.assertRaises(ValueError):
            inspect_elf_target(make_elf((1,2,3,4),flags=4),0x3bafb8)
    def test_noncode_fill(self):
        with self.assertRaises(ValueError):
            inspect_elf_target(make_elf((0,0,0,0)),0x3bafb8)
    def test_array_append(self):
        source='[general]\ninput="x"\nentry_points=[\n "0x00100008",\n]\n[foo]\nentry_points=[]\n'
        updated,added=add_entry_hint(source,0x3bafb8)
        self.assertTrue(added)
        self.assertIn('"0x003BAFB8"',updated)
        self.assertFalse(add_entry_hint(updated,0x3bafb8)[1])
    def test_absent_array(self):
        updated,added=add_entry_hint('[general]\ninput="x"\n',0x3bafb8)
        self.assertTrue(added)
        self.assertIn('entry_points = ["0x003BAFB8"]',updated)

    def test_multi_target_seeding_and_deduplication(self):
        elf=bytearray(make_elf((0x3c040040,0x3c050078,0x2484c9b4,0x080c8934)))
        struct.pack_into('<IIII', elf, 0x1c0, 0x27bdffe0, 0xafbf0018, 0x03e00008, 0)
        config='[general]\nentry_points=["0x00100008"]\n'
        updated, entries=validate_and_seed_entries(bytes(elf),config,[0x3bafb8,0x3bafc0,0x3bafc0])
        self.assertEqual(len(entries),2)
        self.assertIn('0x003BAFB8',updated)
        self.assertIn('0x003BAFC0',updated)
        self.assertTrue(all(v['entry_hint_added'] for v in entries))
        self.assertTrue(all(v['guest_instruction_bytes_patched'] is False for v in entries))
        again, results=validate_and_seed_entries(bytes(elf),updated,[0x3bafb8,0x3bafc0])
        self.assertEqual(updated,again)
        self.assertTrue(all(not v['entry_hint_added'] for v in results))

    def test_invalid_second_entry_refuses_transaction(self):
        config='[general]\nentry_points=[]\n'
        with self.assertRaises(ValueError):
            validate_and_seed_entries(make_elf((0x3c040040,0x3c050078,0x2484c9b4,0x080c8934)),config,[0x3bafb8,0x3bb000])


if __name__=='__main__':
    unittest.main()
