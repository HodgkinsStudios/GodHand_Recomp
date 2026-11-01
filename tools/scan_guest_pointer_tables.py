#!/usr/bin/env python3
"""Audit code pointers stored in an original MIPS ELF without modifying it.

Retail games sometimes store callback tables in load segments marked executable.
Conservative groups of adjacent file-backed executable addresses are surfaced as
candidates, never automatically marked as AOT entry points. An analyst must
confirm candidates by checking the original guest control flow and table users.
"""
import argparse
import json
import struct
from pathlib import Path


def load_segments(elf: bytes):
    if len(elf) < 52 or elf[:6] != b'\x7fELF\x01\x01' or struct.unpack_from('<H', elf, 18)[0] != 8:
        raise ValueError('Expected ELF32 little-endian MIPS executable')
    phoff = struct.unpack_from('<I', elf, 28)[0]
    phentsize, count = struct.unpack_from('<HH', elf, 42)
    if not count or phentsize < 32 or phoff + phentsize * count > len(elf):
        raise ValueError('Invalid ELF program headers')
    segments = []
    for i in range(count):
        typ, off, base, _, sz, mem, flags, _ = struct.unpack_from('<IIIIIIII', elf, phoff + phentsize*i)
        if typ != 1 or off + sz > len(elf) or sz > mem:
            continue
        segments.append(dict(offset=off, address=base, size=sz, flags=flags))
    if not segments:
        raise ValueError('No valid load segments')
    return segments


def callable_shape(elf: bytes, segments, address):
    seg = next((s for s in segments if (s['flags'] & 1) and
                s['address'] <= address and address + 16 <= s['address']+s['size']), None)
    if seg is None:
        return None
    off = seg['offset'] + address - seg['address']
    words = struct.unpack_from('<IIII', elf, off)
    if all(w in (0, 0xffffffff) for w in words):
        return None
    # Heuristics only; not proof of a function boundary.
    if words[0] == 0x03e00008:
        return 'jr_ra_leaf'
    if (words[0] & 0xffff0000) in (0x27bd0000, 0x67bd0000) and (words[0] & 0x8000):
        return 'stack_prologue'
    if any((w & 0xffff0000) in (0xafbf0000, 0xffbf0000) for w in words[:4]):
        return 'saved_ra'
    if words[0] >> 26 == 2:
        return 'direct_tail_jump'
    if words[0] >> 26 == 15:
        return 'lui_preamble'
    return 'other_code'


def find_pointer_groups(elf: bytes, min_pointers=3, min_anchors=1):
    segments=load_segments(elf)
    groups=[]
    def record(seg, pointers, initial_offset):
        if len(pointers) < min_pointers or len({p[0] for p in pointers}) < 2:
            return
        anchors=sum(shape!='other_code' for _,shape in pointers)
        if anchors < min_anchors:
            return
        groups.append({
            'source_address': f"0x{seg['address'] + initial_offset:08X}",
            'source_segment_executable': bool(seg['flags'] & 1),
            'pointer_count': len(pointers),
            'heuristic_anchor_count': anchors,
            'pointers': [{'address': f'0x{p:08X}', 'shape': shape} for p,shape in pointers]
        })
    for seg in segments:
        # File-backed contents only. Scan embedded tables even in executable LOAD
        # segments; ignore BSS, zero-fill and unaligned source slots.
        group=[]
        first=0
        for i in range(0, seg['size']-3, 4):
            value=struct.unpack_from('<I', elf, seg['offset']+i)[0]
            shape=callable_shape(elf, segments, value) if (value & 3)==0 else None
            if shape is not None:
                if not group:
                    first=i
                group.append((value, shape))
            elif group:
                record(seg, group, first)
                group=[]
        if group:
            record(seg, group, first)
    return groups


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--elf', type=Path, required=True)
    ap.add_argument('--report', type=Path, required=True)
    ap.add_argument('--min-pointers', type=int, default=3)
    args=ap.parse_args()
    if args.min_pointers < 3:
        ap.error('--min-pointers cannot be less than three')
    groups=find_pointer_groups(args.elf.read_bytes(), args.min_pointers)
    report={'pointer_groups':len(groups),
            'groups_with_embedded_code_data':sum(1 for g in groups if g['source_segment_executable']),
            'groups':groups}
    args.report.parent.mkdir(parents=True, exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f"Potential guest pointer groups: {len(groups)} (manual verification required)")

if __name__=='__main__':
    main()
