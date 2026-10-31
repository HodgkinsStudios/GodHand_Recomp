# CP10 AOT compiler triage

**Purpose:** Find early generated-C++ compilation blockers without conflating compiler success with real guest execution.

## Verified before upload

- `python3 -m unittest discover -s tools/tests -v`: **14 passing tests**.
- `tools/aot_compile_probe.py` checks boot entry, function registration, synthetic/auxiliary entry, and high-error inferred functions `sub_003C01D8`, `sub_003FDD40`, `sub_003BC628` when present. Remaining slots are selected at evenly spaced guest addresses, not just earliest alphabetically.
- GitHub-hosted default sample size: 24 generated C++ translation units, max two concurrent compiler processes, 90s timeout per TU. Uses the actual CMake `compile_commands.json` entries.
- Probe runs before optional full `ps2EntryRunner` link. Each failed sample has its compilation diagnostics retained in `built/reports/native_compile_probe.json`.

## Interpretation

The initial CP09 reports had 3/3 selected samples compiling, 2,963 unhandled instruction diagnostics, and no successful linked native game or captured frame. CP10 does not claim these issues solved. It tests representative and unusually suspicious code regions to inform the next targeted recompiler pass. A failed sample blocks a wasteful full build; a passed sample does not guarantee the remainder will compile.

## Next

1. Inspect the newest GitHub-hosted run results and the saved failure reports.
2. Correct the first real compiler/source-generation failure, not merely a workflow error.
3. Once full binary links, run under Xvfb and inspect `native_frame_status.txt` and actual PPM pixels.
