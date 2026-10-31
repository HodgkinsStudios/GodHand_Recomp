#!/usr/bin/env python3
"""Compiler smoke test over a stratified sample of real AOT translation units.

The probe uses CMake's compile_commands.json, not guessed flags. This is a
syntax/compilation check only; it does not establish runtime correctness.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
from pathlib import Path
import re
import shlex
import subprocess

_ADDRESS = re.compile(r'_0x([0-9a-fA-F]+)\.cpp$')
# These unusually large inferred functions generated many invalid guest
# instruction diagnostics. Probe them; do NOT silently skip them or alter the
# game's guest semantics.
_SUSPECTS = ('sub_003C01D8', 'sub_003FDD40', 'sub_003BC628')


def choose_commands(commands: list[dict], generated_dir: Path, limit: int = 3) -> list[dict]:
    root = generated_dir.resolve()
    eligible: dict[str, dict] = {}
    for command in commands:
        source = Path(command['file']).resolve()
        try:
            source.relative_to(root)
        except ValueError:
            continue
        if source.suffix == '.cpp':
            # Deduplicate alternate compiler database entries deterministically.
            eligible.setdefault(str(source), command)
    ordered = sorted(eligible.values(), key=lambda c: Path(c['file']).name)
    if limit <= 0 or not ordered:
        return []

    names = {Path(c['file']).name: c for c in ordered}
    priority: list[dict] = []
    priority_names = ('sub_00100008_0x100008.cpp', 'register_functions.cpp')
    for name in priority_names:
        if name in names:
            priority.append(names[name])
    if limit > len(priority):
        for c in ordered:
            name = Path(c['file']).name
            if name.startswith('entry_') and c not in priority:
                priority.append(c)
                break
    if limit > len(priority):
        for prefix in _SUSPECTS:
            match = next((c for c in ordered if Path(c['file']).name.startswith(prefix + '_')), None)
            if match is not None and match not in priority:
                priority.append(match)
            if len(priority) >= limit:
                break
    if len(priority) >= limit:
        return priority[:limit]

    remaining = [c for c in ordered if c not in priority]
    # Stratified sampling across all generated function addresses is much more
    # informative than selecting the first N alphabetical source files.
    remaining.sort(key=lambda c: (int(m.group(1), 16) if (m := _ADDRESS.search(Path(c['file']).name)) else -1,
                                  Path(c['file']).name))
    slots = min(limit - len(priority), len(remaining))
    if slots:
        indices = [(i * len(remaining) + len(remaining) // 2) // slots for i in range(slots)]
        priority.extend(remaining[min(index, len(remaining) - 1)] for index in indices)
    return priority


def compile_probe(commands: list[dict], generated_dir: Path, max_files: int = 3,
                  timeout: int = 120, workers: int = 1) -> dict:
    selected = choose_commands(commands, generated_dir, max_files)

    def compile_one(item: dict) -> dict:
        cwd = item.get('directory', '.')
        args = item.get('arguments') or shlex.split(item['command'])
        try:
            completed = subprocess.run(args, cwd=cwd, capture_output=True, text=True,
                                       errors='replace', timeout=timeout, check=False)
            return {'source': Path(item['file']).name, 'return_code': completed.returncode,
                    'diagnostic': (completed.stderr + '\n' + completed.stdout)[-16000:]}
        except (OSError, subprocess.TimeoutExpired) as exc:
            return {'source': Path(item['file']).name, 'return_code': -1,
                    'diagnostic': str(exc)}

    # Preserve source/result order. Restrict parallelism to avoid exhausting
    # hosted-runner RAM on large generated function bodies.
    with ThreadPoolExecutor(max_workers=max(1, workers)) as pool:
        records = list(pool.map(compile_one, selected))
    return {'selected_translation_units': len(records),
            'successful_translation_units': sum(r['return_code'] == 0 for r in records),
            'failed_translation_units': sum(r['return_code'] != 0 for r in records),
            'selection_strategy': 'entry, registration, suspected invalid regions, stratified code addresses',
            'records': records,
            'native_game_linked': False,
            'real_game_frames_confirmed': False}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-commands', type=Path, required=True)
    p.add_argument('--generated-dir', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--limit', type=int, default=3)
    p.add_argument('--timeout', type=int, default=120)
    p.add_argument('--workers', type=int, default=1)
    args = p.parse_args()
    result = compile_probe(json.loads(args.compile_commands.read_text()), args.generated_dir,
                           max_files=max(1, min(64, args.limit)),
                           timeout=max(1, args.timeout), workers=max(1, min(4, args.workers)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    for row in result['records']:
        print(f"{row['source']}: {'PASS' if row['return_code'] == 0 else 'FAIL'}")
        if row['return_code'] != 0:
            print(row['diagnostic'][-2200:])
    return 0 if result['selected_translation_units'] else 2


if __name__ == '__main__':
    raise SystemExit(main())
