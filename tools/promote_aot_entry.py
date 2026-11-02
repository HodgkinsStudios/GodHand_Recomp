#!/usr/bin/env python3
"""Promote verified original MIPS instruction boundaries to native AOT entries.

Reads real missing-function diagnostics; never synthesizes or skips instructions.
"""
import argparse
import bisect
import json
from pathlib import Path
import re
import sys

MISSING = re.compile(r'(?:\[guest-branch:missing-target\].*?target=0x|\[EE:main-dormant-cause\] missing_recompiled_function pc=0x)([0-9a-fA-F]+)')
OWNER = re.compile(r'// Address: 0x([0-9a-fA-F]+) - 0x([0-9a-fA-F]+)')
FUNCTION = re.compile(r'\bvoid\s+([A-Za-z_][A-Za-z_0-9]*)\s*\(uint8_t\*\s*rdram')
BASE = re.compile(r'g_ps2RecompiledFunctionTableBase\s*=\s*0x([0-9a-fA-F]+)u;')
END = re.compile(r'g_ps2RecompiledFunctionTableEnd\s*=\s*0x([0-9a-fA-F]+)u;')
ASSIGN = re.compile(r'^(\s*)g_ps2RecompiledFunctionTable\[(\d+)\]\s*=\s*([A-Za-z_][A-Za-z_0-9]*);\s*//\s*0x([0-9a-fA-F]+)', re.M)

class InvalidPromotion(ValueError):
    pass

def targets(text):
    return sorted({int(m.group(1), 16) for m in MISSING.finditer(text)})

def candidates(folder):
    result = []
    for path in folder.rglob('sub_*_0x*.cpp'):
        m = re.search(r'_0x([0-9a-fA-F]+)\.cpp$', path.name)
        if not m:
            continue
        with path.open('r', encoding='utf-8', errors='replace') as f:
            first = ''.join(f.readline() for _ in range(30))
        match = OWNER.search(first)
        if match:
            start, end = (int(v, 16) for v in match.groups())
            if start < end and start == int(m.group(1), 16):
                result.append((start, end, path))
    return sorted(result)

def promote_one(source, addr):
    pc = f'{addr:x}'
    header = OWNER.search(source)
    func = FUNCTION.search(source)
    if not header or not func:
        raise InvalidPromotion('missing owner header or C++ function symbol')
    start, end = (int(x, 16) for x in header.groups())
    if not start < addr < end or (addr & 3):
        raise InvalidPromotion('target not a four-byte-aligned internal function address')
    instruction = re.compile(r'^\s*// 0x0*'+pc+r':\s*0x([0-9a-fA-F]+)\b([^\n]*)$', re.M | re.I)
    matches = list(instruction.finditer(source))
    if len(matches) != 1:
        raise InvalidPromotion('target has no unique decoded original MIPS instruction')
    if 'Delay Slot' in matches[0].group(2):
        raise InvalidPromotion('target is a delay-slot instruction; needs separate analysis')
    if not re.search(r'\bctx->pc\s*=\s*0x0*'+pc+r'u\s*;', source, re.I):
        raise InvalidPromotion('target has no corresponding generated PC assignment')
    label = f'label_{pc}'
    if not re.search(r'^'+label+r':', source, re.M | re.I):
        pos = matches[0].start()
        source = source[:pos] + label + ':\n' + source[pos:]
    if not re.search(r'\bcase\s+0x0*'+pc+r'u:', source, re.I):
        if '    switch (ctx->pc) {' in source:
            pos = source.index('        default: break;', source.index('    switch (ctx->pc) {'))
            source = source[:pos] + '        case 0x'+pc+'u: goto '+label+';\n' + source[pos:]
        else:
            entry = re.search(r'^\s*ctx->pc\s*=\s*0x0*'+f'{start:x}'+r'u;', source, re.M | re.I)
            if not entry:
                raise InvalidPromotion('no owner entry PC reset to install resume dispatch')
            pos = entry.start()
            source = source[:pos] + '    if (ctx->pc == 0x'+pc+'u) goto '+label+';\n' + source[pos:]
    return source, func.group(1)

def promote_table(source, addr, function):
    start, end = BASE.search(source), END.search(source)
    if not start or not end:
        raise InvalidPromotion('native AOT table missing base/end metadata')
    base, limit = int(start.group(1), 16), int(end.group(1), 16)
    if addr < base or addr >= limit:
        raise InvalidPromotion('entry outside allocated native function table')
    slot = (addr - base) // 4
    assignments = list(ASSIGN.finditer(source))
    if not assignments:
        raise InvalidPromotion('function table has no assignments')
    if any(int(m.group(2)) == slot for m in assignments):
        raise InvalidPromotion('table slot already registered')
    indent = assignments[-1].group(1)
    addition = f'{indent}g_ps2RecompiledFunctionTable[{slot}] = {function}; // 0x{addr:x} promoted original entry\n'
    pos = source.index('\n', assignments[-1].end()) + 1
    return source[:pos] + addition + source[pos:]

def run(folder, diagnostic, report=None, previous_report=None):
    folder = Path(folder)
    reg = next(folder.rglob('register_functions.cpp'), None)
    if reg is None:
        raise InvalidPromotion('register_functions.cpp missing')
    requested = set(targets(Path(diagnostic).read_text(encoding='utf-8', errors='replace')))
    if previous_report and Path(previous_report).is_file():
        previous = json.loads(Path(previous_report).read_text(encoding='utf-8'))
        for entry in previous.get('promoted', []):
            requested.add(int(entry['pc'], 0))
    requested = sorted(requested)
    owners = candidates(folder)
    starts = [entry[0] for entry in owners]
    registration = reg.read_text(encoding='utf-8', errors='replace')
    updates = {}
    promoted, rejected = [], []
    for pc in requested:
        index = bisect.bisect_right(starts, pc) - 1
        if index < 0 or pc >= owners[index][1]:
            rejected.append({'pc':hex(pc), 'reason':'no exact AOT owner'})
            continue
        path = owners[index][2]
        try:
            current = updates.get(path, path.read_text(encoding='utf-8', errors='replace'))
            edited, symbol = promote_one(current, pc)
            next_registration = promote_table(registration, pc, symbol)
        except InvalidPromotion as exc:
            rejected.append({'pc':hex(pc),'reason':str(exc)})
            continue
        updates[path] = edited
        registration = next_registration
        promoted.append({'pc':hex(pc),'owner':path.name,'native_symbol':symbol})
    for path, source in updates.items():
        path.write_text(source, encoding='utf-8')
    if updates:
        reg.write_text(registration, encoding='utf-8')
    result = {'requested':[hex(v) for v in requested], 'promoted':promoted, 'rejected':rejected}
    if report:
        Path(report).parent.mkdir(parents=True, exist_ok=True)
        Path(report).write_text(json.dumps(result, indent=2)+'\n')
    return result

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--source-dir', required=True, type=Path)
    parser.add_argument('--diagnostic', required=True, type=Path)
    parser.add_argument('--report', type=Path)
    parser.add_argument('--previous-report', type=Path,
                        help='verified entries promoted on an earlier native run')
    parser.add_argument('--require-promoted', action='store_true')
    args = parser.parse_args()
    try:
        result = run(args.source_dir, args.diagnostic, args.report, args.previous_report)
        print(json.dumps(result, indent=2))
        return 1 if args.require_promoted and not result['promoted'] else 0
    except (OSError, InvalidPromotion) as exc:
        print(f'cannot promote original AOT entry: {exc}', file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main())
