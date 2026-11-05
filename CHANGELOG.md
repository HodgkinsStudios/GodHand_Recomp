# GodHand_Recomp — Change history

## 2026-10-31 — Initial documentation and repository preparation

- Imported the supplied PS2Recomp-based source archive as the starting point for `GodHand_Recomp`.
- Rewrote the repository root README with project-specific identity, goals, baseline instructions and explicit upstream credit.
- Added project status, local game file isolation plan, and upstream/legal attribution documentation.
- Added ignore patterns for common disc images, extracted game files and private local outputs.
- Kept the source code, GPL-3.0 `LICENSE`, build files, platform modules, and inherited GitHub Actions CI workflow unchanged.
- No game ISO was included, no God Hand recompilation was performed, and no build or game testing was claimed.

## 2026-10-31 — GodHand launcher implementation

- Added GTK3/C++17 `GodHand:Recomp` launcher branded HodgkinsStudios.
- Implemented local ISO copy/extract, safe traversal blocking, persistent install state and placeholder AOT game launch with explicit missing-executable error.
- Added Ubuntu install script and desktop app entry, standalone CMake and synthetic ISO integration tests; preserved original PS2Recomp source/credits.
- Added independent cloud CI launcher build/test; local runner launcher tests require ISO to remain on the user's machine.

## CP02 — GitHub-hosted development, launcher input/settings, ISO AOT bring-up (2026-10-31)
- New GTK3 launcher Settings dialog and persistent keyboard/gamepad mappings; SDL2 game-controller discovery and hotplug polling.
- Windowed, fullscreen, stretched mode setting persisted and conveyed to prospective native game host.
- Python ISO9660 boot ELF inspector with synthetic MIPS ELF tests.
- GitHub-hosted workflows for launcher binary in `built/` and ISO analysis/recompilation attempt. No gameplay claims.
- Added checksum-verified `godhandLauncher/install_prebuilt.sh` for direct install of hosted launcher binary, without local compilation.
- Added independent SDL2 input/display diagnostic with Esc mode cycling and gamepad hotplug verification; hosted build publishes it under `built/`.

- First GitHub-hosted real ISO boot ELF scan and recompilation attempt passed: `SLUS_215.03`, ELF32 entry 0x00100008, 9,231 generated C++ files. Documented unsupported instructions/function-boundary risks.
- Linux launcher and SDL input diagnostic compiled in GitHub-hosted Actions and pushed to `built/linux-x86_64/`.

## CP03 — Native AOT runtime integration (2026-10-31)
- Added `PS2X_AOT_SOURCE_DIR` to link generated C++ function implementations and registration table into `ps2EntryRunner` without modifying tracked runtime headers.
- Added AOT compiler probe on actual generated boot-entry and function-table translation units; full native compile is available as an opt-in manual Actions input.
- Fixed AOT artifact packaging to include required generated headers (prior archive contained only `.cpp` files).
- Added structured unhandled instruction diagnostics to report translation blockers instead of treating process exit 0 as full correctness.
- Launcher now passes the actual installed `SLUS_215.03` ELF to the native runtime as an argument.
- Added six Python diagnostics tests (total eight ISO/diagnostic tests). Native game executable and real frame rendering not yet verified.

## CP04 — Real-frame capture plumbing (2026-10-31)
- Added optional host capture path `GODHAND_GS_FRAME_PPM`, saving the first non-uniform RGBA image returned from the PS2 GS presentation backend as PPM. Blank fallback images are explicitly rejected.
- GodHand launcher passes a location for this capture in the locally installed game directory.
- Added standalone native C++ synthetic-frame test to hosted launcher workflow.
- Disabled runner PCH during the AOT native-compile probe (compiler commands run individually before a complete native build).
- Frame capture is diagnostic plumbing, **not evidence that God Hand has rendered a frame yet**.

## CP05 — Hosted native boot/frame probe
- Added a guarded Xvfb runtime-execution step for fully linked native builds. Launches the original user-supplied boot ELF, records guest runtime logs and checks for first non-uniform GS pixels.
- If an actual frame is generated, stores it under `built/frames/` and records its status in `built/reports/native_frame_status.txt`.
- Serialized AOT workflow runs to avoid simultaneous source/archive update races.
- This is future execution infrastructure; a native God Hand executable and real frame have **not** been verified yet.
- The first real runtime compile probe passed 2/3 generated files; corrected missing `-msse4.1` for native builds. Added gated full native link attempt on push after the probe passes, followed by Xvfb frame capture if linked. AOT diagnostics identified 2,963 unsupported events, mostly in two likely overextended function spans.

## CP06 — Reliable hosted AOT artifact publishing
- Fixed Git LFS rebase conflicts when earlier hosted jobs concurrently updated `built/aot/generated_cpp.tar.gz` and diagnostic JSON: isolate only the new build outputs, fetch/reset clean latest main, copy staged outputs back, commit, and retry on a racing push.
- Continued to serialize hosted AOT jobs to prevent concurrent writes to `main`.
- Verified revised CI publishing shell syntax locally; no game frame confirmed.

## CP07 — Preserve the first full native game build failure
- CP05's hosted run confirmed **3/3** real generated entry/registration translation units compile with SSE4.1, then the full `ps2EntryRunner` build failed. No native game executable or GS frame has been demonstrated.
- The `Attempt full native executable` step previously inherited the GitHub shell's `-e` flag and exited before recording the redirected `native-build.log`. Corrected to capture nonzero exit codes explicitly and publish the first compiler errors and the final log tail to `built/reports/native_link_status.txt`.
- Applied the same exit-code preservation to native CMake configuration and Xvfb game execution. Added focused regression tests for inherited `bash -e` behavior.

## CP08 — Restore cross-platform runtime compilation
- Investigated GitHub-hosted `build` failure on Linux GCC, Linux Clang, Windows MSVC: all 3 failed to compile `ps2xRuntime/src/lib/ps2_runtime.cpp` due a literal containing a raw newline in the CP04 guest GS capture print statement.
- Fixed to a C++ `\n` escape; a C++17 syntax-only regression test extracts and compiles the exact log statement. Eleven pipeline Python tests pass.
- CP07 hosted full-game failure diagnostics still need to finish; **no God Hand game frames confirmed**.

## CP09 — Native rebuild triggers
- Expanded AOT workflow push filters to cover changes in ps2xRuntime, ps2xRecomp, ps2xAnalyzer, ps2xIOP, root CMake and all pipeline tests. Previously a core runtime fix did not trigger ISO-native validation automatically.
- Added workflow regression guard; 12 Python tests pass. Next hosted run must exercise the corrected GS capture C++ source against the full generated God Hand AOT code.

## CP10 — Broader AOT compiler validation (2026-10-31)
- Improved real-code compilation probe from 3 adjacent function files to up to 24 distributed samples, including entry, registry, generated entry, and three high-error inferred functions.
- Added two-worker bounded parallel compilation and 90-second per-TU timeout to isolate generated-code compile errors before full 9,231-unit compilation.
- Added unit tests for wide range sampling, suspect inclusion, deduplication, parallel status reporting; 14 local Python tests passed.
- No gameplay or guest framebuffer confirmed in CP10; pending hosted workflow verification.

## CP11 — Faster first-boot AOT compilation (2026-10-31)
- Added opt-in `PS2X_AOT_FAST_COMPILE=ON` for generated guest source files, leaving normal builds unmodified; preserves entire generated AOT source list.
- GitHub-hosted native-game workflow enables fast compile while retaining SSE4.1 runtime flags and complete native linkage.
- Verified in local isolated CMake/Ninja build that source `-O0 -g0 -fno-lto` overrides Release `-O3 -flto`, and binary links. Added regression tests.
- No real frame demonstrated; pending hosted native recompilation.

## CP12 — Launcher sibling binary lookup / native link retry
- Resolve game executable next to launcher and in checkout `built/linux-x86_64`; legacy fallback retained.
- Prebuilt installer copies game next to launcher in `~/.local/bin` when it is genuinely built.
- Added separate hosted AOT link workflow using generated C++ LFS archive; ISO fetched only after game successfully links.
- Added locator unit test. Play remains unavailable until an actual native game executable exists.

## CP14 — Restartable AOT native compile
- Split 9,231 generated guest C++ files across four deterministic Ninja object batches. No code omitted.
- Added `ccache` restore/save checkpoints to retain completed compilation work across fresh hosted runners, including canceled runs after a checkpoint.
- Enabled streaming compilation progress and downloadable logs; preserved full native link and GS validation gates.
- Passed seven focused tests and Bash syntax checks for 13 workflow shell steps. Native build and game-frame result still unverified.

## CP16 — Reuse successful AOT, bound runtime testing
- Confirmed CP15 native build 9,231 generated guest C++ translation units compiled and linked, 98.92% ccache hit rate, 209,103,376-byte x86-64 executable published to main through LFS.
- Verified CP16 prebuilt-only 20-second real boot probe: original ELF loaded; OpenGL initialized; missing indirect JALR target `0x003BAFB8` (caller `0x0031DB2C`); no game GS frame; exit 124.
- Added bounded `.github/workflows/godhand-prebuilt-smoke.yml` without recompilation, published diagnostic report, and made both costly AOT workflows manual-only. Restricted analyzer publishing to its own files so stale `link_ci_*` diagnostics cannot overwrite the successful native-link result.


## CP18 — Second observed MIPS call recovery
- Verified 209,109,192-byte CP17 native executable and 25,001,106-byte generated AOT archive in Git LFS on main.
- Real CP17 boot reaches past 0x003BAFB8; next guest JALR missing target is 0x0038F200. No guest GS drawing yet.
- Added atomic multi-target ELF validation and generated-registry checks; no guest instruction replacement or synthetic rendering.


## CP19 — Recover third genuine MIPS function entry
- CP18 native game linked and published, boot passed 0x003BAFB8 and 0x0038F200; next missing target 0x0038B6E0.
- Added a third original-ELF-validated entry metadata hint and required it in both generated and native registry checks.
- Restricted auto analysis to explicit seed script or workflow changes. All guest functions and real frame validation remain mandatory.

## CP20 — Additional authentic callback entries
- CP19 native link run 41323942723 succeeded; original guest ELF advanced past 0x0038B6E0 to missing JALR target 0x0031DA28. No guest GS activity or actual game frame yet.
- Add original ELF-backed entry discovery for observed target 0x0031DA28 and callback table candidate 0x00375840, preserving 0x003BAFB8, 0x0038F200 and 0x0038B6E0.
- Analyzer and linker workflows refuse updated output unless all five authentic MIPS addresses are validated and registered. No guest instructions patched.

## CP21 — Audit true embedded ELF callback tables
- Added independent MIPS ELF32 pointer-group audit, including callback tables in executable LOAD segments, reporting original-file-backed candidates without changing code or automatically treating data as functions.
- Added six synthetic-ELF tests and an inexpensive, bounded prebuilt-only workflow report to investigate the remaining indirect-call coverage systematically.
- Not a game-specific runtime hook; no artificial graphics and no AOT binary replacement from this audit.

## CP22 — Recover authentic functions from executable-embedded pointer tables
- Real original ELF audit found 358 executable-region pointer groups, with a strongly supported 71-pointer callback table containing multiple previously missing guest call targets.
- Added generic conservative function start discovery for groups of four or more consecutive executable pointers, all validated by existing MIPS callable-entry heuristics; no names/addresses hardcoded in the recompiler.
- This is a function-boundary analysis improvement, not an instruction patch or high-level game shortcut. Validate Linux/Windows compiler and subsequent full AOT analysis before replacing the linked executable.

## CP24 — Original guest countdown diagnosis (2026-11-01)
- CP22 AOT native binary published to main, authentic GIF/DMA/GS transfer traffic observed; game has no nonuniform guest framebuffer yet.
- Inspected exact AOT host function `sub_00398890_0x398890.cpp` after preserved boot was stuck at 0x00398930: guest loops while decrementing `$v0` until it reaches `$v1=-1`, loaded from original guest global at 0x003F868C.
- Added prebuilt-only AOT source inspection and bounded original ELF boot PC inspection workflows; preserved reports without guest patching.
- Added thread-safe `$v0/$v1` debug snapshots on scheduler checkpoints and log output; triggers an explicit native link with cached 9k+ compilation results, no gameplay modification.

## CP48 — EE interrupt handlers nest on the interrupted stack (2026-11-04)
- Measured the unaligned `0x00000F82` return to a live-frame overwrite: original `sd $s0,($sp)` at `0x003B6C44` stores a DMA byte count at `0x01FFFF10`, where the interrupted frame's saved `$ra` (`0x0030B38C`) still lived.
- Reserved invocation stacks started at `PS2_RAM_SIZE - 0x10`, which is also the EE main thread's initial `$sp`, so interrupt frames were guaranteed to overlap live main-thread frames.
- The EE scheduler now seeds interrupt and async invocations that have no explicit stack from the interrupted guest `$sp`, so handler frames nest below live frames like the real EE exception handler; the reserved-stack fallback is retained for invocations with no live guest context.
- Added the `IRQ handler nests below the interrupted EE stack pointer` regression test; the existing isolated-stack and sequential-callback expectations are preserved.
- `EeScheduler.cpp` and the interrupt tests compile in this environment; no native rebuild, guest boot or GS frame has been verified yet.

## CP49 — Native build verifies the interrupt-stack fix (2026-11-04)
- Reproduced the project's native build without root (PyPI cmake/ninja, Ubuntu `.deb` X11/GL sysroot, Xvfb from the same sysroot) and compiled all 9,362 generated guest TUs into a 211 MB `ps2EntryRunner`.
- Confirmed the CP48 repair on a real disc boot: the `0x00000F82` return-address corruption and the resulting Dormant main thread are gone, nine IOP modules load, and the guest registers `sceGsSyncVCallback`.
- Recovered 978 ELF-validated AOT entries (observed indirect targets plus pointer-table function starts) with the existing tools; the run no longer reports missing recompiled functions.
- Scoped the repair to guest interrupt invocations only after `ps2xTest` showed host-driven GS/async callbacks must keep their reserved pool; the full suite passes 447/447.
- Remaining blocker measured: the main thread loops at `0x001FFCD0` polling a status byte until 3, with no GIF/DMA submission, so no GS frame exists yet.

## CP50 — AOT audit, ROM-resident CDVDFSV, and the CRI ADXI blocker (2026-11-04)
- Audited AOT coverage independently of the generator: a Capstone sweep of the real `.text` (`0x00100000`–`0x003BBF20`, 716,744 instructions) found 6,774 distinct `jal` targets, **all** of them registered; no direct call is missing. 1,662 unregistered stack prologues are indirect/alias entries, and the real boot logs zero unresolved dispatches.
- Diagnosed the CP49 hang: `adxf_Init` ran `adxf_CreateAdxFs`, failed the file-open state machine with `E02110501 adxstmf_stat_exec: can't open`, and destroyed the FS, leaving a NULL handle for `adxf_GetStat` to poll forever.
- CDVDFSV is an IOP-kernel ROM service, so libcdvd binds SID `0x80000597` without loading an IRX; the runtime only routed core services after an IRX load. Added `IopService::romResident()` and marked `cdvdfsv` ROM-resident, exposing its genuine ISO9660 search (still returns "not found" for absent files).
- Fixed the search packet layout: the EE libcdvd header is 36 bytes, not 32, so the service had always searched for an empty name. Both observed searches now resolve real entries (`\MODULES\IRX.IMS;1`, `\0FLISTDV.DIR;1`).
- `sceSifDmaStat` returned `-1` unconditionally, so the guest wait at `0x002BE3D8` (`bltz` on the result) could never end. It now reports `0` for a transfer id this runtime issued (transfers are applied inside `sceSifSetDma`) and `-1` only for ids never issued; the DMA test records the new contract and the reasoning.
- Boot now leaves the `0x001FFCD0` poll, loads six IOP modules (`sio2man`, `padman`, `libsd`, `sdrdrv`, `sdhd`, `snddrv`) and stops at `0x003758B8`, rebinding CRI ADXI SID `0x19720317` while `client->server` stays NULL because `cri_adxi.irx` is never loaded on this path. Full suite: 447/447 passing. No GS frame yet.
- Follow-up on the ADXI stall: the bind loop waits on global `0x76A980`, not a client field, and the only writer in a 40 s boot is the `memset` at `0x003A533C`/`0x003A5388` (from `0x00377C50`) that clears it — nothing ever sets it. The `cri_adxi.irx` loader sits immediately *after* that wait (`0x002C9E38`). Disc inventory corrected: the ISO has a real `\MODULES\` directory with `IOPRP310.IMG` (16 embedded ELFs, no ADXI) and `IRX.IMS` (nine entries); `sceSifRebootIop` is never called, and the module loader now records the guest `ra`, the requested path and every refused load.

## CP51 - CRI ADXI bind semantics and the IOP heap blocker
- `0x003758B8` is `client->server` (client `0x76A95C`, field at `+0x24`, i.e. `0x76A980`), not a separate flag: ps2sdk's bind always replies and reports a miss as `sd = NULL`, so the ADXI loop can only finish once the IOP has registered SID `0x19720317`. Refused binds are now logged once per SID (`[BindRpc] no server for sid=...`) instead of silently parking a boot.
- Added the diagnostic switch `PS2X_BIND_ANY` (off by default, never a fix) which publishes a client descriptor for every SID. With it the boot loads a 7th module (`cri_adxi.irx`) and reaches a new, precisely identified blocker at `0x0033B2B0`: `E0100301: SJX_Init can't allocate IOP Heap`, because the IOP heap is exhausted by 163 leaked `0x1480` staging allocations from the guest's archive loader.
- Recorded that emulated IRX modules perform zero `sceSifRegisterRpc` and zero `CreateThread`, so no IOP RPC server can ever be bound; `docs/CP51_ADXI_BIND_AND_IOP_HEAP.md` has the full analysis, the reproduction commands and the rootless toolchain restore recipe. 447/447 tests still pass; no GS frame yet.
## CP52 - The IOP-heap leak mechanism fully identified

- Added temporary `PS2X_SYSCALL_TRACE`, `PS2X_IOPHEAP_TRACE` and `PS2X_MMIO_TRACE` traces
  (all removed again; the tracked tree is clean), which pinned the 163x module-load retry:
  each attempt allocates 0x1480 IOP bytes, SifSetDma's it into IOP RAM, then runs the game's
  own libk SIF send wrapper (pure guest code, no syscalls) whose ack gate is an IOP-written
  word that never updates. The load only completes via the alloc-failure fallback at
  `0x2BE420 -> sceSifLoadModule` syscall, so every success costs 163 leaked
  blocks and the whole 0x120000-0x1F0000 IOP heap.
- Captured the unhandled SIF traffic precisely: 16385 CHCR=0x101 writes to DMA channel
  `0x1000A000` (MADR 0x7867C0, QWC 0x46) during boot - SIF cmd batch transfers our DMAC
  model never delivers to the IOP; the in-lib bind gate binds SID `0x80000006` at `0x3B5A80`.
- Recorded the fix plan in `docs/CP52_SIF_CMD_LEAK.md`: implement EE->IOP SIF cmd delivery
  plus a rom-resident IOP SIF cmd dispatcher service that executes the genuine module-load
  semantics and acks into EE RAM; expected to let all 9 modules load leak-free so
  `cri_adxi.irx` can reach `sceSifRegisterRpc`/`CreateThread` and clear the ADXI bind stall.
  Tests: 447/447 ps2x_tests; no GS frame yet.
## CP53 - Correcting the CP52 SIF diagnosis

- A dumped DMA payload proves the 16385 `CHCR=0x101` writes to channel `0x1000A000` are
  ordinary GIF (D2) transfers of a small GS register list (regs `0x50`-`0x53` plus a
  per-frame counter at `+0x14`) - the game's loading-spinner redraw loop - not SIF command
  batches. GIF completion is correct (`CHCR` reads back `0x00000001`) and the 16387
  `FlushCache` syscalls belong to the same loop, so the CP52 "unimplemented SIF send" story
  must not be implemented against that channel.
- Read the module-load decision path end to end: `0x3B5C48` prepares via `0x3B5A80` /
  `0x3B5B80`, then issues RPC function 6 on the `SID 0x80000006` client at `0x781B80`
  through `0x3B3128`, whose kick is `sceSifSetDma` (syscall `0x77`). A full boot issues
  **zero** `0x77` syscalls, so the retry loop is rejected in the prepare prologue, not by a
  missing DMA acknowledgement - that is the next thing to instrument.
- Verified tree state: temporary probes removed, 447/447 `ps2x_tests` pass, no frame yet.
