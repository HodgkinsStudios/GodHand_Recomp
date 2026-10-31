# GodHand_Recomp — First real ISO / AOT analysis results

GitHub-hosted Actions workflow **GodHand - ISO boot ELF and AOT analysis**, run `41239765051` (2026-10-31) completed successfully. **This records toolchain execution, not a runnable game.**

## Source-disc identification

| Field | Actual result |
| --- | --- |
| ISO root regular file count | 7 |
| `SYSTEM.CNF` selected boot ELF | `SLUS_215.03` |
| ELF size | 3,607,008 bytes |
| ELF format | 32-bit little-endian MIPS |
| ELF program entry | `0x00100008` |
| Boot ELF SHA-256 | `1742f95bef65bdb2aa57b7a77df4ac7619a092e9646e1ea325bc32ec8a64f3cd` |

Analyzer logs report **127 SDK/library functions to stub**, **266 library-looking functions without runtime handlers**, **26 candidate patches**, and **7 jump tables**. The heuristic analyzer generated a TOML config; the static recompiler returned exit code `0` and generated **9,231 C++ files**, archived in `built/aot/generated_cpp.tar.gz` via Git LFS.

## Blocking issues discovered

The recompiler diagnostic tail reports numerous unhandled-instruction records in `sub_003C01D8`, including `SPECIAL` and `REGIMM` opcodes in the high-address span around `0x003F4B4C` and later. This may reflect unknown real opcodes, data misidentified as code, or invalid/overextended function boundaries. It **must not** be treated as successful instruction coverage until function maps and instruction decoding are validated. The original toolchain's JAL heuristic is a fallback for retail games; its documentation prefers Ghidra-derived function maps.

Generated C++ has **not** been compiled into a native God Hand executable; no game boot, visuals, audio or controller tests have passed. No `built/linux-x86_64/GodHand_Recomp_Game` is available.

## Next focused passes

1. Build a verified Ghidra function map of the boot ELF and compare to 9,231 heuristically generated C++ files; especially investigate the oversized `sub_003C01D8` span.
2. Separate false-positive data segments from code and address real unhandled PS2 R5900 instructions in reusable recompiler logic.
3. Build generated functions against the existing PS2 runtime, log missing symbol/runtime handlers, and work toward a genuine linked native executable.
4. Wire the already-built launcher and reusable SDL2 input/display layer into the actual game entry point; validate ISO assets from the local installed directory.

Source reports: [`built/reports/`](../built/reports/). Workflow: `https://github.com/HodgkinsStudios/GodHand_Recomp/actions/runs/41239765051`.
