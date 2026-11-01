"""Synthetic CRI AFS unit tests. No retail game data is used."""
import importlib.util
import io
from pathlib import Path
import struct
import tempfile
import unittest

script = Path(__file__).resolve().parents[1] / "godhand_afs.py"
spec = importlib.util.spec_from_file_location("godhand_afs", script)
afs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(afs)


def fixture(with_names=True, damaged=False):
    data = bytearray(0x1400)
    data[:4] = b'AFS\x00'
    struct.pack_into('<I', data, 4, 2)
    struct.pack_into('<II', data, 8, 0x800, 5)
    struct.pack_into('<II', data, 16, 0x1000, 7 if not damaged else 0x999999)
    if with_names:
        struct.pack_into('<II', data, 24, 0x1100, 96)
        data[0x1100:0x110b] = b'file_0.tm3\x00'
        data[0x1130:0x113e] = b'../bad/second\x00'
    data[0x800:0x805] = b'TIM3\x00'
    data[0x1000:0x1007] = b'\x7fELF\x01\x01\x01'
    return data


class AfsTests(unittest.TestCase):
    def test_index_and_signature(self):
        rows, info = afs.archive_index(io.BytesIO(fixture()), 0, 0x1400)
        self.assertEqual(2, len(rows))
        self.assertEqual(0x1000, rows[1]['offset'])
        self.assertTrue(info['filename_toc_present'])
        self.assertEqual('PS2 TIM3 texture', afs.signature(b'TIM3\x00'))

    def test_unnamed_archive(self):
        rows, info = afs.archive_index(io.BytesIO(fixture(False)), 0, 0x1400)
        self.assertFalse(info['filename_toc_present'])
        self.assertEqual('', rows[0]['name'])

    def test_bad_extent_rejected(self):
        with self.assertRaises(afs.ArchiveError):
            afs.archive_index(io.BytesIO(fixture(damaged=True)), 0, 0x1400)

    def test_streamed_extract_sanitizes_names(self):
        image = fixture()
        rows, _ = afs.archive_index(io.BytesIO(image), 0, len(image))
        with tempfile.TemporaryDirectory() as tmp:
            out = Path(tmp)
            result = afs.extract(io.BytesIO(image), 0, len(image), rows, out,
                                 [0, 1], max_total=128, max_entry=32)
            self.assertEqual(12, result['extracted_bytes'])
            self.assertEqual(2, len(list(out.iterdir())))
            self.assertTrue(all(f.parent == out for f in out.iterdir()))
            self.assertTrue(any(f.read_bytes().startswith(b'\x7fELF')
                                for f in out.iterdir()))


if __name__ == '__main__':
    unittest.main()
