#!/usr/bin/env python3
"""Prepare genuine ELF-validated entry hints from executed missing-target traces.

Only actual missing guest calls are used; never fabricate or patch guest code.
"""
import argparse
import json
from pathlib import Path
import re
import sys
from seed_aot_entry import validate_and_seed_entries

BRANCH = re.compile(r'\[guest-branch:missing-target\][^\n]*')
TARGET = re.compile(r'(?<!\w)target=0x([0-9a-fA-F]{1,8})(?![0-9a-fA-F])')
SOURCE = re.compile(r'(?<!\w)source=0x([0-9a-fA-F]{1,8})(?![0-9a-fA-F])')


def collect_observed_targets(log: str, max_entries: int = 16):
    if not 1 <= max_entries <= 128:
        raise ValueError('max_entries must be 1..128')
    found, seen = [], set()
    for matched in BRANCH.finditer(log):
        line = matched.group()
        target, source = TARGET.search(line), SOURCE.search(line)
        if target is None or source is None or 'codeRegion=yes' not in line:
            continue
        address, caller = int(target.group(1), 16), int(source.group(1), 16)
        if (address | caller) & 3 or not address or address == caller:
            continue
        if address not in seen:
            found.append({'source': f'0x{caller:08X}', 'target': f'0x{address:08X}'})
            seen.add(address)
            if len(found) >= max_entries:
                break
    return found


def recovery(elf: bytes, config: str, runtime_log: str, limit: int = 16):
    observed = collect_observed_targets(runtime_log, limit)
    if not observed:
        raise ValueError('No observed executable missing-target addresses')
    addresses = [int(row['target'], 16) for row in observed]
    updated, verified = validate_and_seed_entries(elf, config, addresses)
    return updated, {'observed_missing_targets': observed,
                     'verified_elf_targets': verified,
                     'guest_instruction_bytes_patched': False,
                     'config_changed': updated != config}


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--config', type=Path, required=True)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--max-entries', type=int, default=16)
    args = parser.parse_args(argv)
    try:
        original = args.config.read_text()
        updated, report = recovery(args.elf.read_bytes(), original,
                                   args.log.read_text(), args.max_entries)
        args.report.parent.mkdir(parents=True, exist_ok=True)
        if updated != original:
            args.config.write_text(updated)
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print(json.dumps(report, indent=2))
    except (OSError, ValueError, KeyError) as exc:
        parser.exit(1, f'Refusing unverified guest entry recovery: {exc}\n')
    return 0


if __name__ == '__main__':
    sys.exit(main())
