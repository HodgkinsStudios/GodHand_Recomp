#!/usr/bin/env python3
"""Compile generated AOT translation units in deterministic Ninja batches.

Each batch is independently cacheable in GitHub Actions. No guest code is skipped:
final link still includes all CMake target sources, including the host runtime.
"""
import argparse
import json
import os
from pathlib import Path
import re
import shlex
import subprocess
import sys


def generated_object_targets(compdb, source_root, build_dir):
    source_root = Path(source_root).resolve()
    build_dir = Path(build_dir).resolve()
    commands = json.loads(Path(compdb).read_text())
    targets = []
    for entry in commands:
        directory = Path(entry['directory']).resolve()
        source = Path(entry['file'])
        if not source.is_absolute():
            source = directory / source
        source = source.resolve()
        if not source.is_relative_to(source_root) or source.suffix != '.cpp':
            continue
        args = entry.get('arguments') or shlex.split(entry['command'])
        if '-o' in args:
            index = args.index('-o')
            if index + 1 >= len(args):
                raise ValueError(f'Missing output after -o for {source}')
            output = args[index + 1]
        else:
            matches = [v[2:] for v in args if v.startswith('-o') and len(v) > 2]
            if len(matches) != 1:
                raise ValueError(f'Cannot identify Ninja object for {source}')
            output = matches[0]
        object_path = Path(output)
        if not object_path.is_absolute():
            object_path = directory / object_path
        object_path = object_path.resolve()
        if not object_path.is_relative_to(build_dir):
            raise ValueError(f'Object path outside build directory: {object_path}')
        targets.append((source.relative_to(source_root).as_posix(),
                        object_path.relative_to(build_dir).as_posix()))
    if not targets:
        raise ValueError('No generated C++ translation units found in compile_commands.json')
    if len(targets) != len(set(s for s, _ in targets)):
        raise ValueError('Duplicate generated source entries in compile_commands.json')
    if len(targets) != len(set(t for _, t in targets)):
        raise ValueError('Duplicate Ninja object targets in compile_commands.json')
    return [target for _, target in sorted(targets)]


def partition(items, batch_index, batches):
    if batches <= 0 or not (1 <= batch_index <= batches):
        raise ValueError('Batch index must be between 1 and batches, inclusive')
    size, remainder = divmod(len(items), batches)
    offset = (batch_index - 1) * size + min(batch_index - 1, remainder)
    n = size + int(batch_index <= remainder)
    return items[offset:offset + n]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--compile-commands', type=Path, required=True)
    parser.add_argument('--source-dir', type=Path, required=True)
    parser.add_argument('--build-dir', type=Path, required=True)
    parser.add_argument('--batch', type=int, required=True)
    parser.add_argument('--batches', type=int, default=4)
    parser.add_argument('--jobs', type=int, default=4)
    parser.add_argument('--log', type=Path, required=True)
    args = parser.parse_args()
    if args.jobs <= 0 or args.jobs > 64:
        parser.error('--jobs must be between 1 and 64')
    try:
        targets = generated_object_targets(args.compile_commands,
                                           args.source_dir, args.build_dir)
        selection = partition(targets, args.batch, args.batches)
    except (OSError, KeyError, json.JSONDecodeError, ValueError) as exc:
        print(f'AOT batch setup failed: {exc}', file=sys.stderr)
        return 2
    args.log.parent.mkdir(parents=True, exist_ok=True)
    cmd = ['ninja', '-C', str(args.build_dir.resolve()), '-j', str(args.jobs),
           '-k', '1', *selection]
    print(f'Batch {args.batch}/{args.batches}: {len(selection)} of {len(targets)} real AOT C++ units', flush=True)
    print(f'Compiling with {args.jobs} workers. Full log: {args.log}', flush=True)
    status = 0
    try:
        with args.log.open('w', encoding='utf-8') as log:
            with subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  text=True, bufsize=1) as child:
                for line in child.stdout:
                    log.write(line)
                    print(line, end='', flush=True)
                status = child.wait()
    except OSError as exc:
        print(f'Unable to run Ninja: {exc}', file=sys.stderr)
        status = 2
    print(f'Batch {args.batch}/{args.batches} finished: exit code {status}', flush=True)
    return status


if __name__ == '__main__':
    sys.exit(main())
