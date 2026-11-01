import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from seed_aot_entry import inspect_elf_target, add_entry_hint

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

if __name__=='__main__':
    unittest.main()
