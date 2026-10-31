#!/usr/bin/env python3
"""Summarize PS2Recomp diagnostics without hiding unhandled guest opcodes.

The recompiler may exit 0 while emitting error events. This tool tracks the
actual guest-translation blockers for the next PS2 runtime/AOT pass.
"""
import argparse
from collections import Counter
import json
from pathlib import Path
import re

UNHANDLED = re.compile(
    r'\[error\]\s+unhandled-instruction\s+function=(\S+)\s+'
    r'addr=0x([0-9a-fA-F]+)\s+-\s+(.*?)\s+raw=0x([0-9a-fA-F]+)'
)
SUMMARY = re.compile(r'^([^\n:]+):\s*([0-9]+)\s*$', re.MULTILINE)


def analyze(log: str, generated_names: list[str] | None = None) -> dict:
    counts = Counter()
    function_errors: dict[str, dict] = {}
    samples = []
    for line in log.splitlines():
        match = UNHANDLED.search(line)
        if not match:
            continue
        function, pc, reason, raw = match.groups()
        counts[reason] += 1
        func = function_errors.setdefault(function, {'count': 0, 'first_pc': None, 'last_pc': None, 'samples': []})
        address = int(pc, 16)
        func['count'] += 1
        if func['first_pc'] is None or address < int(func['first_pc'], 16):
            func['first_pc'] = f'0x{address:08X}'
        if func['last_pc'] is None or address > int(func['last_pc'], 16):
            func['last_pc'] = f'0x{address:08X}'
        if len(func['samples']) < 5:
            func['samples'].append({'pc': f'0x{address:08X}', 'raw': f'0x{int(raw, 16):08X}', 'reason': reason})
        if len(samples) < 16:
            samples.append({'function': function, 'pc': f'0x{address:08X}', 'raw': f'0x{int(raw, 16):08X}', 'reason': reason})
    summaries = dict(SUMMARY.findall(log))
    generated_count = len(generated_names) if generated_names is not None else None
    top = sorted(function_errors.items(), key=lambda pair: (-pair[1]['count'], pair[0]))
    return {
        'generated_translation_units': generated_count,
        'recompiler_report_unhandled_count': int(summaries['Unhandled instructions']) if 'Unhandled instructions' in summaries else None,
        'unhandled_events_observed': sum(counts.values()),
        'has_unhandled_instructions': bool(counts),
        'error_messages_by_type': [{'reason': k, 'count': v} for k, v in counts.most_common(20)],
        'most_affected_functions': [{'function': k, **v} for k, v in top[:30]],
        'samples': samples,
        'limits': ('Counts only events present in the captured complete log; '
                   'a 0 process exit status does not imply correct guest translation.'),
    }


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--log', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--generated-list', type=Path)
    args = p.parse_args()
    generated = args.generated_list.read_text().splitlines() if args.generated_list and args.generated_list.exists() else None
    result = analyze(args.log.read_text(errors='replace'), generated)
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    print(f"Unhandled guest instructions: {result['unhandled_events_observed']}")
    if result['most_affected_functions']:
        worst = result['most_affected_functions'][0]
        print(f"Worst affected function: {worst['function']} ({worst['count']} unhandled)")
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
