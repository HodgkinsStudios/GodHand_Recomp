import importlib.util
from pathlib import Path
import struct
import tempfile
import unittest

spec=importlib.util.spec_from_file_location('godhand_iso_pipeline', Path(__file__).parents[1] / 'godhand_iso_pipeline.py')
iso=importlib.util.module_from_spec(spec)
spec.loader.exec_module(iso)

def entry(name, extent, size, directory=False):
    raw = name if isinstance(name, bytes) else name.encode('ascii')
    length=33+len(raw)+(len(raw)%2 == 0)
    rec=bytearray(length)
    rec[0]=length
    rec[2:6]=struct.pack('<I',extent)
    rec[6:10]=struct.pack('>I',extent)
    rec[10:14]=struct.pack('<I',size)
    rec[14:18]=struct.pack('>I',size)
    rec[25]=2 if directory else 0
    rec[28:30]=struct.pack('<H',1)
    rec[30:32]=struct.pack('>H',1)
    rec[32]=len(raw)
    rec[33:33+len(raw)]=raw
    return rec


def sample_iso(path, valid=True):
    data=bytearray(32*2048)
    pvd=16*2048
    data[pvd]=1
    data[pvd+1:pvd+6]=b'CD001'
    data[pvd+156:pvd+190]=entry(b'\0',20,2048,True)
    system=b'BOOT2 = cdrom0:\\SLUS_000.00;1\r\nVER=1.00\r\n'
    data[20*2048:20*2048+len(entry(b'\0',20,2048,True))]=entry(b'\0',20,2048,True)
    offset=20*2048+34
    for rec in (entry('SYSTEM.CNF;1',21,len(system)),entry('SLUS_000.00;1',22,52)):
        data[offset:offset+len(rec)]=rec;offset+=len(rec)
    data[21*2048:21*2048+len(system)]=system
    elf=bytearray(52)
    elf[0:6]=b'\x7fELF\x01\x01'
    struct.pack_into('<H',elf,18,8 if valid else 3)
    struct.pack_into('<I',elf,24,0x100000)
    data[22*2048:22*2048+52]=elf
    path.write_bytes(data)

class TestIso(unittest.TestCase):
    def test_boot_and_hash(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d)
            sample_iso(root/'image.iso')
            report=iso.process(root/'image.iso',root/'report',True)
            self.assertEqual(report['boot_file'],'SLUS_000.00')
            self.assertEqual(report['boot_elf']['entry_point'],'0x00100000')
            self.assertEqual((root/'report/boot.elf').stat().st_size,52)
            self.assertFalse(report['game_playable'])
    def test_wrong_machine(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d); sample_iso(root/'image.iso',False)
            with self.assertRaises(iso.IsoError): iso.process(root/'image.iso',root/'out',False)
    def test_traversal_rejected(self):
        for value in (b'BOOT2=cdrom0:\\..\\SECRET;1',b'BOOT2=cdrom0:\\FOO\\..\\SECRET;1',b'BOOT2=cdrom0:\\;1'):
            with self.assertRaises(iso.IsoError): iso.boot_elf_path(value)
    def test_invalid_iso(self):
        with tempfile.TemporaryDirectory() as d:
            root=Path(d); (root/'i.iso').write_bytes(b'hello')
            with self.assertRaises(iso.IsoError): iso.process(root/'i.iso',root/'out',True)

if __name__=='__main__': unittest.main()
