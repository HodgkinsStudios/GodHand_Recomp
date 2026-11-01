# CP18 — Second genuine MIPS indirect-call entry

## Existing authoritative built files on main

- `built/linux-x86_64/GodHand_Recomp_Game`: CP17 native AOT executable, **209,109,192 bytes**, LFS SHA-256 `abc377f6d91f3a3767cd31662c814ae1f6c2fff91ab782d6504d6d2987297971`.
- `built/aot/generated_cpp.tar.gz`: complete CP17 generated C++ source archive, **25,001,106 bytes**, LFS SHA-256 `cbcd5bd370339e62971d20aa05ce021804d27cda9ecafb735be1ce6cea866423`.
- CP17 native link run `41266978792` succeeded. Its Xvfb run loaded original PS2 ELF, initialized OpenGL, and executed game code.
- Original missing indirect target `0x003BAFB8` now executes; next missing target at `0x0038F200`, called from `0x0031DB2C`. Boot still has no DMA/GIF/GS/VIF traffic or genuine frame.

## Targeted correction

`0x0038F200` lies inside the address interval bounded by generated routine starts `0x0038F1A0` and `0x0038F3D8`. This is a **function entry-boundary discovery** problem, not evidence that fake code should be inserted.

The existing CLI `tools/seed_aot_entry.py` now accepts repeated `--entry` options, validates **all** original executable MIPS words before updating any TOML metadata, and produces a combined audit report. The two entries to preserve and add are `0x003BAFB8` and `0x0038F200`.

The recompiler emits genuine code and may map an interior target to a resumable owner function; a dedicated `sub_<addr>.cpp` is not required. The analyzer must prove **both** entries exist in its generated `register_functions.cpp` before replacing the old AOT archive. The link workflow verifies both before compiling and only publishes a replacement game executable after a successful full link.

No guest instruction modification, HLE, game-specific runtime stub or synthetic frame is permitted. The running game has **not** rendered a real GS frame yet.

## Follow-up

Inspect new `built/reports/link_ci_frame.txt` and whether real DMA/GIF/GS/VIF counters advance. Do not call a linked native binary playable until it visibly renders guest graphics. If the next indirect target is absent, audit its actual ELF instructions and function boundaries.
