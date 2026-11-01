#!/usr/bin/env python3
"""Validate an actual MIPS code address and add a non-patching AOT entry hint.

The generated registry may bind an interior entry to an existing function body;
a separate guest translation unit is not necessary in that case.
"""
import argparse
import json
from pathlib import Path
import re
import struct
import tomllib

def inspect_elf_target(elf: bytes, address: int):
    if len(elf) < 52 or elf[:4] != b'\x7fELF' or elf[4:6] != b'\x01\x01':
        raise ValueError('Expected ELF32 little-endian')
    if struct.unpack_from('<H', elf, 18)[0] != 8:
        raise ValueError('Expected MIPS ELF')
    if address & 3:
        raise ValueError('Entry not four-byte aligned')
    phoff = struct.unpack_from('<I', elf, 28)[0]
    entsize, count = struct.unpack_from('<HH', elf, 42)
    if entsize < 32 or count == 0 or phoff + count * entsize > len(elf):
        raise ValueError('ELF program header table truncated')
    for index in range(count):
        pos = phoff + entsize * index
        kind, file_offset, vaddr, _, file_size, mem_size, flags, _ = struct.unpack_from('<IIIIIIII', elf, pos)
        if kind != 1 or (flags & 1) == 0 or not vaddr <= address < vaddr + mem_size:
            continue
        delta = address - vaddr
        if delta + 16 > file_size or file_offset + delta + 16 > len(elf):
            raise ValueError('Executable target is not backed by 16 bytes in original ELF')
        words = struct.unpack_from('<IIII', elf, file_offset + delta)
        if all(x in (0, 0xFFFFFFFF) for x in words):
            raise ValueError('The target contains only fill/zero instructions')
        return {
            'entry_address': f'0x{address:08X}',
            'executable_segment_start': f'0x{vaddr:08X}',
            'executable_segment_end': f'0x{vaddr + mem_size:08X}',
            'first_four_mips_words': [f'0x{x:08X}' for x in words],
            'first_instruction_lui': words[0] >> 26 == 0x0F,
        }
    raise ValueError('Target is not inside an executable PT_LOAD')

def add_entry_hint(toml_text: str, address: int):
    config = tomllib.loads(toml_text)
    general = config.get('general')
    if not isinstance(general, dict):
        raise ValueError('TOML [general] section missing')
    existing = general.get('entry_points', [])
    if not isinstance(existing, list) or any(not isinstance(s, str) for s in existing):
        raise ValueError('entry_points must be an array of strings')
    canonical = f'0x{address:08X}'
    if any(re.fullmatch(r'0[xX][0-9a-fA-F]+', x) and int(x, 16) == address for x in existing):
        return toml_text, False
    headings = list(re.finditer(r'^\s*\[([^\]\r\n]+)\]\s*$', toml_text, re.M))
    index = next((i for i, heading in enumerate(headings) if heading.group(1).strip() == 'general'), None)
    if index is None:
        raise ValueError('Cannot locate general TOML section')
    start = headings[index].end()
    end = headings[index + 1].start() if index + 1 < len(headings) else len(toml_text)
    section = toml_text[start:end]
    entry = re.search(r'(?m)^entry_points\s*=\s*\[', section)
    if entry:
        closing = section.find(']', entry.end())
        if closing < 0:
            raise ValueError('Unterminated entry_points array')
        section = section[:closing] + f'  "{canonical}",\n' + section[closing:]
    else:
        section += f'\nentry_points = ["{canonical}"]\n'
    result = toml_text[:start] + section + toml_text[end:]
    if canonical not in tomllib.loads(result)['general']['entry_points']:
        raise ValueError('Entry point did not survive TOML round trip')
    return result, True

def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--elf', type=Path, required=True)
    p.add_argument('--config', type=Path, required=True)
    p.add_argument('--entry', type=lambda s: int(s, 0), required=True)
    p.add_argument('--report', type=Path, required=True)
    args = p.parse_args()
    try:
        details = inspect_elf_target(args.elf.read_bytes(), args.entry)
        next_config, changed = add_entry_hint(args.config.read_text(), args.entry)
        if changed:
            args.config.write_text(next_config)
        details.update(entry_hint_added=changed, guest_instruction_bytes_patched=False)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        args.report.write_text(json.dumps(details, indent=2) + '\n')
        print(json.dumps(details, indent=2))
    except (OSError, ValueError, KeyError, tomllib.TOMLDecodeError) as ex:
        p.exit(1, f'Unverified executable entry; refusing injection: {ex}\n')

if __name__ == '__main__':
    main()
