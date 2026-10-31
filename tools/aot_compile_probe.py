#!/usr/bin/env python3
"""Compile a few real PS2Recomp-generated TUs using CMake's exact flags.

Reports source-level compiler blockers without conflating syntax success with
complete AOT linkage, execution, or rendered frames.
"""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import time


def choose_commands(commands: list[dict], generated_dir: Path, limit: int = 3) -> list[dict]:
    root = generated_dir.resolve()
    eligible = []
    for command in commands:
        source = Path(command['file']).resolve()
        try:
            source.relative_to(root)
        except ValueError:
            continue
        if source.suffix == '.cpp':
            eligible.append(command)
    eligible.sort(key=lambda c: (0 if Path(c['file']).name == 'sub_00100008_0x100008.cpp' else
                                 1 if Path(c['file']).name == 'register_functions.cpp' else 2,
                                 Path(c['file']).name))
    return eligible[:limit]


def compile_probe(commands: list[dict], generated_dir: Path, max_files: int = 3, timeout: int = 120) -> dict:
    selected = choose_commands(commands, generated_dir, max_files)
    records = []
    for item in selected:
        cwd = item.get('directory', '.')
        args = item.get('arguments') or shlex.split(item['command'])
        try:
            completed = subprocess.run(args, cwd=cwd, capture_output=True, text=True,
                                       errors='replace', timeout=timeout, check=False)
            records.append({'source': Path(item['file']).name, 'return_code': completed.returncode,
                            'diagnostic': (completed.stderr + '\n' + completed.stdout)[-16000:]})
        except (OSError, subprocess.TimeoutExpired) as exc:
            records.append({'source': Path(item['file']).name, 'return_code': -1,
                            'diagnostic': str(exc)})
    return {'selected_translation_units': len(records),
            'successful_translation_units': sum(r['return_code'] == 0 for r in records),
            'failed_translation_units': sum(r['return_code'] != 0 for r in records),
            'records': records,
            'native_game_linked': False,
            'real_game_frames_confirmed': False}


def main() -> int:
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--compile-commands', type=Path, required=True)
    p.add_argument('--generated-dir', type=Path, required=True)
    p.add_argument('--output', type=Path, required=True)
    p.add_argument('--limit', type=int, default=3)
    args = p.parse_args()
    result = compile_probe(json.loads(args.compile_commands.read_text()), args.generated_dir,
                           max_files=max(1, min(20, args.limit)))
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + '\n')
    for row in result['records']:
        print(f"{row['source']}: {'PASS' if row['return_code'] == 0 else 'FAIL'}")
        if row['return_code'] != 0:
            print(row['diagnostic'][-2200:])
    # Diagnostic probe does not suppress writing the report on compile failure.
    return 0 if result['selected_translation_units'] else 2


if __name__ == '__main__':
    raise SystemExit(main())
