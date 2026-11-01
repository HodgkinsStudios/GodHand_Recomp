# CP22 — General PS2 MIPS callback discovery from executable data

## Grounded evidence from the original game ELF

The CP21 nonmutating pointer audit, published at `built/reports/code_pointer_audit.json`, found **358** groups of >=3 adjacent original guest-code pointers whose storage locations belong to an executable PT_LOAD segment. Some game objects place a genuine vtable inside an executable segment rather than a separately marked data section.

A 71-pointer consecutive group at `0x003FC884` contains the genuine observed startup targets and other recognizable MIPS prologues. The original fallback routine `ScanDataFunctionPointerTables` explicitly skips sections marked `isCode`, so these callback entries were not discovered by that pass. This is a generic stripped-ELF analysis limitation; no God Hand-specific guest runtime workaround is needed.

## Source change

The new `ScanEmbeddedExecutablePointerTables` in `ps2xRecomp/src/lib/elf_parser.cpp` runs after the existing materialized-address and data-only pointer scanners. It considers executable sections, requires groups of **at least four** contiguous aligned pointers to guest executable code, at least two distinct target addresses, and validates **every target** using the existing conservative `LooksLikeCallableEntry` MIPS disassembly heuristic before recording real function starts.

The method never patches original guest instructions, never adds host stubs, never bypasses calls, and never invents rendered frames. It is intentionally stricter than the diagnostic audit (which also reports weaker candidates for human review). It can increase legitimate translated function counts and so requires build validation and boot regression testing before declaring improvement.

## Validation and next steps

- CP21's six synthetic-ELF pointer audit tests pass locally.
- Hosted general C++ build must compile the updated parser.
- After build validation, reanalyze original ELF to generate a new AOT archive, then use the existing four cached native compilation batches and link the game. Do not remove or overwrite the last successful executable if any step fails.
- Under Xvfb run actual `SLUS_215.03` from the ISO, inspect missing-target trace, guest DMA/GIF/VIF/GS writes, and a real nonuniform PPM guest framebuffer. Host OpenGL initialization or synthetic images are **not** proof of game rendering.
