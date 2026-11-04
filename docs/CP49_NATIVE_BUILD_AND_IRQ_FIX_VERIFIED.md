# CP49 — Native build reproduces the CP48 fix; boot now reaches the GS-sync wait loop

**Result: the CP48 interrupt-stack repair is confirmed on a real native build and real disc boot. The guest now advances ~3.4x further, loads nine IOP modules and registers `sceGsSyncVCallback`. No GS frame is produced yet: the EE main thread settles into a guest status-poll loop at `0x001FFCD0`.**

## How the native build was produced here (no root)

Neither cmake, ninja, raylib, FFmpeg, nor the X11 development files exist in this container, and the account cannot use `sudo`. The build was still reproduced with the project's own CI configuration:

- `cmake` and `ninja` from PyPI into a private prefix (`python3 -m pip install --target /tmp/tooling cmake ninja`).
- X11/GL headers and libs, plus Xvfb and Mesa, from official Ubuntu noble `.deb` archives unpacked into `/tmp/sysroot` (dependency closure resolved from the suite `Packages.gz` indices, `dpkg-deb -x`, no system modification).
- `cmake -S . -B build-game -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_C_FLAGS=-msse4.1 -DCMAKE_CXX_FLAGS=-msse4.1 -DPS2X_BUILD_RECOMP=OFF -DPS2X_BUILD_ANALYZER=OFF -DPS2X_BUILD_RUNTIME=ON -DPS2X_BUILD_STUDIO=OFF -DPS2X_BUILD_TEST=OFF -DPS2X_ENABLE_DEBUG_UI=OFF -DPS2X_ENABLE_FFMPEG=OFF -DPS2X_ENABLE_RUNNER_UNITY_BUILD=OFF -DPS2X_ENABLE_RUNNER_PCH=OFF -DPS2X_AOT_FAST_COMPILE=ON -DPS2X_AOT_SOURCE_DIR=<generated output>`, with raylib fetched from its pinned upstream tag.
- All 9,362 generated guest translation units compiled (46 minutes at `--parallel 8`), producing a 211,920,552-byte `ps2EntryRunner`.
- The distribution Xvfb has a compiled-in `/usr/bin/xkbcomp` path; running it without root needs a private copy with that one 8-byte literal redirected (`/usr/bin` → `/tmp/x/`), plus the extracted `xkbcomp`. Mesa llvmpipe then provides GL 4.5 through GLX on `:99`.

This is the same configuration `godhand-aot.yml` / `godhand-link.yml` use, so the result is a genuine native build, not a host-side simulation.

## Measured outcome of CP48

Real disc (`PS2X_CD_IMAGE`), real `SLUS_215.03` boot ELF, 240 s wall clock under Xvfb:

| Stage | Before CP48 | After CP48 |
| --- | --- | --- |
| Boot stall | `missing_recompiled_function pc=0x00000F82`, main thread Dormant, 26.2M EE cycles | no `0x00000F82`; thread keeps running |
| IOP modules | stalled before `SifBindRpc` | 9 IRX modules loaded (`SIO2MAN`, `PADMAN`, `LIBSD`, `SDRDRV`, `SDHD`, `SNDDRV`, `CRI_ADXI`, `MCMAN`, `MCSERV`) |
| GS | stall before any `sceGs*` use | `[sceGsSyncVCallback:set] new=0x30B148` |

The measured corruption chain (interrupt `sd $s0,($sp)` at `0x003B6C44` writing a DMA byte count over the saved `$ra` at `0x01FFFF10`) no longer occurs, which is exactly what the CP48 change predicted.

## AOT entry recovery performed to get past the next wall

With the corruption gone, execution died on indirect calls to real functions that the analyzer's static discovery had missed (they are only reachable through data pointers):

1. `0x003364F8` via `jalr` at `0x00330394` — a leaf that stores two globals.
2. `0x00334230` via the same walker after seeding the first.

Both were seeded with the project's own tools rather than hand-written code: `tools/observed_entry_recovery.py` / `tools/seed_aot_entry.py` validated each address against the original ELF (executable PT_LOAD, 16 file-backed bytes, non-fill, `lui` preamble), then `ps2_recomp` regenerated the AOT set. Seeding bound them as resume entries inside the existing `sub_001FFC60`/`sub_00336498` owners, so no instruction was invented and no guest byte was patched.

Because the walkers read callbacks from static data tables, the next run was pre-empted with the project's conservative discovery: `tools/scan_guest_pointer_tables.py` (358 groups) plus an ELF-wide scan of data words that point at genuine function starts (`addiu $sp,$sp,-N` or `lui` first instruction, 208 more). That produced **978 validated entry hints** total. With them compiled in, the run no longer reports any `missing_recompiled_function`, `main-dormant` or `invalid-return` — the guest executes continuously until it blocks itself.

## Current blocker (no graphics yet)

After `MCMAN`/`MCSERV` load and the GS VSync callback registration, the main thread runs this guest loop:

```
0x001FFCC8  jal 0x00395398        ; sync helper
0x001FFCCC  move $a0, $zero
0x001FFCD0  jal 0x003244C8        ; status query
0x001FFCD4  move $a0, $s1
0x001FFCD8  bne $v0, $s3, 0x001FFCC8   ; $s3 == 3
```

`0x003244C8` masks interrupts, reads a status byte (`lb $v0, 1($a0)` in `0x00324500`) and unmasks again; the loop repeats until that byte is `3`. In every 120-tick sample the thread is in `EeWaitReason::VSync` (`waitReason=4`) at this same PC, `eeSequence` advances (the wait itself completes each VBlank), but the status never becomes `3`, so no GIF/DMA is ever submitted and `GODHAND_GS_FRAME_PPM` is never written.

This is therefore not a scheduler or stack defect: it is a guest-visible status/completion path (the read/RPC sequence `0x00323618` → `0x003243C8` → `0x0030EE08` → `0x00323E80` that precedes the poll) that never reports completion. The next investigation belongs there, not in `EeScheduler`.

## Reproduce

```
# build (after the sysroot/PyPI toolchain above)
cmake --build build-game --target ps2EntryRunner --parallel 8
# run
DISPLAY=:99 LIBGL_ALWAYS_SOFTWARE=1 GODHAND_GS_FRAME_PPM=/tmp/frame.ppm \
  PS2X_CD_IMAGE=<GodHand.iso> build-game/ps2xRuntime/ps2EntryRunner <boot.elf>
```

`built/reports/local_native_irq_fix.txt` holds the condensed evidence from the runs described above.

## Tests

`ps2xTest` was built against this code in a separate tree and run headless: **447/447 passed**, including `IRQ handler nests below the interrupted EE stack pointer`, `IRQ callbacks use an isolated invocation stack`, `sceGsSyncVCallback runs as a scheduler invocation on its callback stack` and `pending async callbacks execute sequentially on a reusable invocation stack`. The GS-callback and async-callback tests are what narrowed the repair to interrupt dispatch only.
