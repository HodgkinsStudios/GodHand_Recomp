#!/usr/bin/env python3
"""Read the boot ELF from ISO9660 using Python's standard library.

Does not publish or upload game bytes. Designed for a GitHub-hosted ephemeral runner.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import sys

BLOCK = 2048
MAX_DIR_BYTES = 64 * 1024 * 1024
MAX_BOOT_BYTES = 128 * 1024 * 1024


class IsoError(Exception):
    pass


def directory_records(f, extent, size):
    if size > MAX_DIR_BYTES:
        raise IsoError('ISO directory too large')
    f.seek(extent * BLOCK)
    data = f.read(size)
    if len(data) != size:
        raise IsoError('Truncated ISO directory')
    pos = 0
    while pos < len(data):
        length = data[pos]
        if not length:
            pos = ((pos // BLOCK) + 1) * BLOCK
            continue
        if length < 34 or pos + length > len(data):
            raise IsoError('Invalid ISO directory record')
        rec = data[pos:pos + length]
        name_len = rec[32]
        if 33 + name_len > length:
            raise IsoError('Invalid ISO9660 entry length')
        name = rec[33:33 + name_len]
        if name not in (b'\x00', b'\x01'):
            yield (name.decode('ascii', errors='replace'),
                   struct.unpack_from('<I', rec, 2)[0],
                   struct.unpack_from('<I', rec, 10)[0],
                   bool(rec[25] & 2))
        pos += length


def clean_iso_name(name):
    return name.split(';', 1)[0].rstrip('.').upper()


def iso_entries(f):
    f.seek(16 * BLOCK)
    pvd = f.read(BLOCK)
    if len(pvd) != BLOCK or pvd[0] != 1 or pvd[1:6] != b'CD001':
        raise IsoError('Not an ISO9660 primary volume')
    root = pvd[156:190]
    if root[0] < 34:
        raise IsoError('Invalid ISO root directory')
    extent, size = struct.unpack_from('<I', root, 2)[0], struct.unpack_from('<I', root, 10)[0]
    # Only metadata for matching the boot ELF, no game data in report.
    todo = [('', extent, size, 0)]
    seen = set()
    entries = {}
    while todo:
        prefix, block, length, depth = todo.pop()
        if depth > 12 or (block, length) in seen:
            continue
        seen.add((block, length))
        for name, child_block, child_size, is_dir in directory_records(f, block, length):
            normalized = clean_iso_name(name)
            if normalized in ('', '.', '..') or '/' in normalized or '\\' in normalized:
                continue
            full_name = prefix + normalized
            entries[full_name] = (child_block, child_size, is_dir)
            if is_dir:
                todo.append((full_name + '/', child_block, child_size, depth + 1))
            if len(entries) > 200000:
                raise IsoError('Unusually large ISO file table')
    return entries


def boot_elf_path(cnf):
    text = cnf.decode('latin-1')
    match = re.search(r'^\s*BOOT2?\s*=\s*cdrom0?:[\\/]+([^\r\n]+)', text, re.I | re.M)
    if not match:
        raise IsoError('Missing BOOT/BOOT2=cdrom0: entry in SYSTEM.CNF')
    value = match.group(1).strip().strip('"').replace('\\', '/')
    parts = [p.upper() for p in value.split('/') if p]
    if not parts or any(p in ('.', '..') for p in parts):
        raise IsoError('Unsafe or empty boot ELF path')
    resolved = '/'.join(parts).split(';', 1)[0]
    if not resolved or any(not component for component in resolved.split('/')):
        raise IsoError('Unsafe or empty boot ELF path')
    return resolved


def read_entry(f, entry, maximum):
    block, size, is_dir = entry
    if is_dir or size <= 0 or size > maximum:
        raise IsoError('Missing, empty, or oversized ISO file')
    f.seek(block * BLOCK)
    content = f.read(size)
    if len(content) != size:
        raise IsoError('Truncated ISO entry')
    return content


def inspect_elf(blob):
    if len(blob) < 52 or blob[:4] != b'\x7fELF':
        raise IsoError('Boot file is not an ELF')
    if blob[4] != 1 or blob[5] != 1:
        raise IsoError('Expected 32-bit little-endian PS2 ELF')
    machine = struct.unpack_from('<H', blob, 18)[0]
    if machine != 8:
        raise IsoError(f'Expected MIPS ELF (e_machine=8), got {machine}')
    return {'class': 'ELF32', 'endianness': 'little', 'architecture': 'MIPS',
            'entry_point': f'0x{struct.unpack_from("<I", blob, 24)[0]:08X}',
            'sha256': hashlib.sha256(blob).hexdigest(), 'bytes': len(blob)}


def process(iso, out, extract):
    with open(iso, 'rb') as f:
        index = iso_entries(f)
        cnf_item = index.get('SYSTEM.CNF')
        if not cnf_item:
            raise IsoError('SYSTEM.CNF not found in ISO root')
        path = boot_elf_path(read_entry(f, cnf_item, 1024 * 1024))
        if path not in index:
            raise IsoError(f'BOOT ELF {path} not found in ISO')
        elf = read_entry(f, index[path], MAX_BOOT_BYTES)
    report = {'iso_file_count': sum(not row[2] for row in index.values()),
              'boot_file': path, 'boot_elf': inspect_elf(elf),
              'aot_ready': False, 'game_playable': False}
    out.mkdir(parents=True, exist_ok=True)
    (out / 'iso_report.json').write_text(json.dumps(report, indent=2) + '\n')
    if extract:
        (out / 'boot.elf').write_bytes(elf)
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--iso', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--extract-elf', action='store_true')
    args = parser.parse_args()
    try:
        print(json.dumps(process(args.iso, args.out, args.extract_elf), indent=2))
    except (OSError, IsoError) as ex:
        print(f'ISO inspection failed: {ex}', file=sys.stderr)
        return 1
    return 0


if __name__ == '__main__':
    sys.exit(main())
