import struct
import sys
from pathlib import Path
import unittest
sys.path.insert(0, str(Path(__file__).resolve().parents[1]))
from scan_guest_pointer_tables import load_segments, callable_shape, find_pointer_groups

def fake_elf(pointer_words, *, executable_source=True):
    e=bytearray(0x900)
    e[:6]=b'\x7fELF\x01\x01'
    struct.pack_into('<H', e,18,8)
    struct.pack_into('<I',e,28,52)
    struct.pack_into('<HH',e,42,32,2)
    struct.pack_into('<IIIIIIII',e,52,1,0x100,0x100000,0,0x400,0x400,5,4)
    struct.pack_into('<IIIIIIII',e,84,1,0x600,0x500000,0,0x100,0x100,5 if executable_source else 6,4)
    for off in [0x40,0x60,0x80,0xa0]:
        struct.pack_into('<IIII',e,0x100+off,0x27bdfff0,0xafbf0008,0x03e00008,0)
    for i,word in enumerate(pointer_words):
        struct.pack_into('<I',e,0x600+i*4,word)
    return bytes(e)

class PointerAuditTests(unittest.TestCase):
    def test_executable_section_embedded_callback_group(self):
        e=fake_elf([0x100040,0x100060,0x100080,0x1000a0])
        g=find_pointer_groups(e)
        self.assertEqual(len(g),1)
        self.assertTrue(g[0]['source_segment_executable'])
        self.assertEqual(g[0]['pointer_count'],4)
        self.assertEqual(g[0]['source_address'],'0x00500000')
    def test_data_segment_callback_group(self):
        g=find_pointer_groups(fake_elf([0x100040,0x100060,0x100080],executable_source=False))
        self.assertEqual(len(g),1)
        self.assertFalse(g[0]['source_segment_executable'])
    def test_two_pointers_not_enough(self):
        self.assertFalse(find_pointer_groups(fake_elf([0x100040,0x100060])))
    def test_zeros_and_misalignment_not_addresses(self):
        self.assertFalse(find_pointer_groups(fake_elf([0x100040,0,0x100060,0x100080])))
        self.assertFalse(find_pointer_groups(fake_elf([0x100041,0x100060,0x100080])))
    def test_noise_does_not_create_entry(self):
        self.assertFalse(find_pointer_groups(fake_elf([0x1000e0,0x100060,0x1000e0])))
    def test_bad_elf_rejected(self):
        with self.assertRaises(ValueError):load_segments(b'not an ELF')

if __name__=='__main__': unittest.main()
