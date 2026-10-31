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

## CP11 — First-boot AOT compile speed (2026-10-31)
- Added opt-in `PS2X_AOT_FAST_COMPILE` to compile all generated guest functions at -O0/-g0/-fno-lto (MSVC /Od /GL-) in the hosted game workflow; default remains OFF.
- Verified GCC flags order and successful miniature CMake/Ninja link; 16 local Python tests pass. Full God Hand native link and guest frames not yet verified.
