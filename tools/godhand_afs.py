#!/usr/bin/env python3
"""Inspect or selectively extract a CRI AFS archive from a file or original PS2 ISO.

Standard-library only; uses bounded random reads, not a 1.5 GiB RAM buffer.
Inspect/report modes never write original game assets. Extraction is opt-in.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re
import struct
import sys

SECTOR = 2048
LIMIT = 300000
MAGIC = b'AFS\x00'


class ArchiveError(Exception):
    pass


def u32(buf, at=0):
    return struct.unpack_from('<I', buf, at)[0]


def check(cond, message):
    if not cond:
        raise ArchiveError(message)


def iso_archive(stream):
    from godhand_iso_pipeline import iso_entries
    entries = iso_entries(stream)
    if '_DATA.AFS' not in entries:
        raise ArchiveError('_DATA.AFS missing from ISO root')
    lsn, size, is_directory = entries['_DATA.AFS']
    check(not is_directory, 'AFS entry is a directory')
    return lsn * SECTOR, size


def at(stream, base, extent, offset, length):
    check(offset >= 0 and length >= 0 and offset <= extent and
          length <= extent - offset, 'archive read outside physical extent')
    stream.seek(base + offset)
    block = stream.read(length)
    check(len(block) == length, 'short archive read')
    return block


def archive_index(stream, base, extent):
    header = at(stream, base, extent, 0, 8)
    check(header[:4] == MAGIC, 'not CRI AFS (missing AFS magic)')
    count = u32(header, 4)
    check(0 < count <= LIMIT, 'AFS entry count out of bounds')
    table_bytes = count * 8
    check(8 + table_bytes + 8 <= extent, 'AFS entry table truncated')
    table = at(stream, base, extent, 8, table_bytes + 8)
    entries = []
    for n in range(count):
        offset, size = struct.unpack_from('<II', table, n * 8)
        check(offset <= extent and size <= extent - offset,
              f'AFS entry {n} extends outside archive')
        entries.append({'index': n, 'offset': offset, 'size': size})
    toc_offset, toc_bytes = struct.unpack_from('<II', table, table_bytes)
    names = []
    if toc_offset and toc_bytes >= count * 48 and toc_offset <= extent and toc_bytes <= extent - toc_offset:
        for n in range(count):
            raw = at(stream, base, extent, toc_offset + 48 * n, 48)
            name = raw[:32].split(b'\0', 1)[0].decode('latin-1', 'replace')
            names.append(name)
    else:
        names = [''] * count
    for row, name in zip(entries, names):
        row['name'] = name
    return entries, {'entries': count, 'archive_bytes': extent,
                     'filename_toc_present': bool(toc_offset and toc_bytes >= count * 48 and
                                                  toc_offset + toc_bytes <= extent),
                     'filename_toc_offset': toc_offset,
                     'filename_toc_bytes': toc_bytes}


def signature(data):
    if data.startswith(b'\x7fELF'): return 'MIPS/ELF executable candidate'
    if data.startswith(b'AFS\x00'): return 'nested AFS archive'
    if data.startswith(b'AFS2'): return 'AFS2 audio bank'
    if data.startswith(b'ADX') or data[:2] == b'\x80\x00': return 'ADX audio candidate'
    if data.startswith(b'SShd'): return 'PS2 sound bank header'
    if data.startswith(b'TM3\x00') or data.startswith(b'TIM3'): return 'PS2 TIM3 texture'
    if data.startswith(b'TIM2'): return 'PS2 TIM2 texture'
    if data.startswith(b'\x00\x00\x01\xba'): return 'MPEG program stream/PSS candidate'
    if data.startswith(b'CRILAYLA'): return 'CRILAYLA compressed asset'
    if data.startswith(b'@UTF'): return 'CRI UTF table'
    if data.startswith(b'OggS'): return 'Ogg audio'
    if data.startswith(b'RIFF'): return 'RIFF/WAV/other'
    if data.startswith(b'PK\x03\x04'): return 'ZIP archive'
    if data.startswith(b'\x89PNG\r\n\x1a\n'): return 'PNG'
    return 'unknown'


def safe_output_name(record):
    original = record['name']
    # Never trust names from an archive as output filesystem paths.
    valid = re.sub(r'[^A-Za-z0-9._-]', '_', original.split('/')[-1].split('\\')[-1])
    valid = valid.lstrip('.')
    if not valid or valid in ('.', '..'):
        valid = 'unnamed.bin'
    return f"{record['index']:06d}_{valid[:90]}"


def inspect(stream, base, extent, entries, summary, limit):
    counts = Counter()
    examples = []
    total = 0
    for e in entries:
        size = e['size']
        total += size
        first = at(stream, base, extent, e['offset'], min(64, size))
        kind = signature(first)
        ext = Path(e['name']).suffix.lower() if e['name'] else '(unnamed)'
        counts[kind] += 1
        if len(examples) < limit or kind != 'unknown' and sum(k == kind for k in
                (p['signature'] for p in examples)) < 5:
            if len(examples) < limit + 64:
                examples.append({'index':e['index'], 'name':e['name'],
                                 'offset':e['offset'], 'size':size,
                                 'signature':kind, 'header_hex':first[:16].hex()})
    summary['sum_entry_bytes'] = total
    summary['signatures'] = dict(sorted(counts.items()))
    summary['extensions'] = dict(Counter(Path(e['name']).suffix.lower() if e['name']
                                         else '(unnamed)' for e in entries).most_common(30))
    summary['examples'] = examples
    summary['first_entries'] = entries[:min(30, len(entries))]
    return summary


def inspect_fst(stream, iso_info):
    from godhand_iso_pipeline import iso_entries
    directory = iso_entries(stream)
    result = {}
    for name in ['GODHAND.FST', '0FLISTDV.DIR']:
        entry = directory.get(name)
        if not entry: continue
        lsn, size, isdir = entry
        check(not isdir and size <= 4 * 1024 * 1024, 'invalid FST/index extent')
        stream.seek(lsn * SECTOR)
        data = stream.read(size)
        check(len(data) == size, 'truncated FST file')
        strings = [m.group().decode('latin-1', 'replace')
                   for m in re.finditer(rb'[\x20-\x7e]{5,}', data)]
        result[name] = {'bytes':size, 'header32_hex':data[:32].hex(),
                        'strings_sample':strings[:80], 'strings_count':len(strings)}
    return result


def extract(stream, base, extent, entries, outdir, indices, max_total, max_entry):
    outdir.mkdir(parents=True, exist_ok=True)
    total = 0
    records = []
    for n in sorted(set(indices)):
        check(0 <= n < len(entries), f'index {n} out of range')
        e = entries[n]
        size = e['size']
        check(size <= max_entry and total + size <= max_total,
              f'extraction limit hit before entry {n}')
        name = safe_output_name(e)
        path = outdir / name
        # Refuse overwriting a previous extraction, and copy streaming.
        with path.open('xb') as output:
            left = size
            stream.seek(base + e['offset'])
            while left:
                block = stream.read(min(left, 1024 * 1024))
                check(bool(block), f'truncated entry {n}')
                output.write(block)
                left -= len(block)
        total += size
        records.append({'index':n, 'name':name, 'bytes':size})
    return {'extracted_count':len(records), 'extracted_bytes':total,
            'entries':records}


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    src = ap.add_mutually_exclusive_group(required=True)
    src.add_argument('--iso', type=Path, help='original PS2 ISO containing _DATA.AFS')
    src.add_argument('--afs', type=Path, help='direct extracted _DATA.AFS')
    ap.add_argument('--report', type=Path, help='write metadata-only JSON report')
    ap.add_argument('--examples', type=int, default=45)
    ap.add_argument('--extract-dir', type=Path, help='opt-in local extraction directory')
    ap.add_argument('--ids', help='comma-separated 0-based entry indices to extract')
    ap.add_argument('--all', action='store_true', help='extract all entries when --extract-dir set')
    ap.add_argument('--max-total-mib', type=int, default=128)
    ap.add_argument('--max-entry-mib', type=int, default=64)
    args = ap.parse_args()
    try:
        if args.iso:
            stream = args.iso.open('rb')
        else:
            stream = args.afs.open('rb')
        with stream:
            if args.iso:
                base, extent = iso_archive(stream)
                source = str(args.iso)
            else:
                base, extent = 0, args.afs.stat().st_size
                source = str(args.afs)
            entries, meta = archive_index(stream, base, extent)
            meta['source'] = source
            meta['archive_offset_bytes'] = base
            if args.iso:
                meta['iso_sidecar_indexes'] = inspect_fst(stream, None)
            inspect(stream, base, extent, entries, meta, max(0, min(200, args.examples)))
            print(json.dumps(meta, indent=2))
            if args.report:
                args.report.parent.mkdir(parents=True, exist_ok=True)
                args.report.write_text(json.dumps(meta, indent=2) + '\n')
            if args.extract_dir:
                check(args.all or args.ids is not None,
                      '--extract-dir requires --ids or --all')
                ids = range(len(entries)) if args.all else [
                    int(i.strip()) for i in args.ids.split(',') if i.strip()]
                extracted = extract(stream, base, extent, entries, args.extract_dir,
                                    ids, args.max_total_mib * 1024 * 1024,
                                    args.max_entry_mib * 1024 * 1024)
                print('EXTRACTION', json.dumps(extracted))
            elif args.ids is not None or args.all:
                raise ArchiveError('--ids/--all requires --extract-dir')
        return 0
    except (OSError, ValueError, ArchiveError) as e:
        print(f'AFS inspector error: {e}', file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
