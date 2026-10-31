# GodHand_Recomp CP02 authoritative handoff

Continue HodgkinsStudios/GodHand_Recomp on GitHub `main`. Source originally derives from https://github.com/ran-j/PS2Recomp (retain GPL-3.0 and credit). Do not restart the project. Check latest `main` and latest GitHub-hosted Actions runs before edits.

Latest work: CP01 launcher installed user-supplied ISO into ~/.local/share/HodgkinsStudios/GodHand_Recomp/GodHand.iso and Extracted_Iso; CP02 adds Settings keyboard/gamepad rebind, SDL2 hotplug, screen mode configuration; custom ISO9660 boot ELF scanner; CI launcher build and ISO/AOT workflows. `built/` holds tested development outputs after CI. Game not booting yet.

The ISO is tracked as Git LFS chunks `iso/GodHand.iso.part-000` and `part-001`; cloud Actions checkout must set `lfs: true` to get actual bytes. Run the workflow `.github/workflows/godhand-aot.yml` on hosted Ubuntu only, inspect `built/reports/` results, and continue analysis and runtime implementation. Do not claim playability from generated C++ alone.

Important: temporary repository is public and contains retail-game-derived data. User says they have permissions and plans a clean repo at release. Do not introduce extra proprietary files without considering exposure; do not commit the uncompressed ISO. Push changes to `main` when verified, maintain handoff notes, checkpoint.num and implementation history.

## CP02 verified first-run results

GitHub-hosted AOT workflow 41239765051 completed. Detected ISO boot file `SLUS_215.03`, MIPS ELF32, entry 0x00100008, 3,607,008 bytes. Analyzer 127 library stubs, 266 unresolved library-like functions, 26 patches, 7 jump tables. Recompiler exit 0 generated 9,231 .cpp files, archived under `built/aot/` LFS. Recompiler diagnostics contain unhandled instruction spans in `sub_003C01D8` and elsewhere: **do not confuse exit 0 with code correctness**. See `docs/CP02_REAL_ISO_REPORT.md`; next analyze function boundaries (prefer authoritative Ghidra export), then C++ compile/runtime linking.

Hosted Linux launcher and SDL input/display diagnostic both compile and pass CI, binaries tracked at `built/linux-x86_64/`, see `godhandLauncher/install_prebuilt.sh`.


## CP03 work and next actions
CP03 adds ps2xRuntime/CMakeLists.txt `PS2X_AOT_SOURCE_DIR` to compile generated .cpp plus function registry into native ps2EntryRunner; updates launcher Play to pass `Extracted_Iso/SLUS_215.03` as guest ELF argv[1]; fixes AOT tar to contain generated headers; adds `aot_diagnostics.py` for detailed opcode errors and `aot_compile_probe.py` for real compiler experiments, plus tests. GitHub-hosted AOT workflow on push reruns analysis, native runtime configure and entry/registration compilation probes. Read `built/reports/native_configure.txt`, `native_compile_probe.json` and `unhandled_instructions.json` and FIX the actual blockers. When feasible trigger `workflow_dispatch` `attempt_full_native_build=true`; build full AOT game under built/linux-x86_64 only if linked successfully. Do not claim real frames until captured and inspected.


## CP04 frame evidence
Optional real GS capture now exists (`ps2_frame_capture.cpp` called after GS copy succeeded, from `UploadFrame`); launcher env `GODHAND_GS_FRAME_PPM` points at the local install folder. Synthetic sample test passes, but no God Hand frame has been observed. Continue prioritizing the hosted `built/reports/native_configure.txt` and `native_compile_probe.json` errors; do not confuse this capture feature with actual guest rendering. CP04 also turned runner PCH off for a compiler probe that runs individual commands.


## CP05 native boot loop
`godhand-aot.yml` now has a guarded Xvfb native boot step following an optional full compile (`workflow_dispatch` input `attempt_full_native_build=true`). If full link succeeds, it runs the real boot ELF for <=75 seconds and logs runtime output, saving the first non-uniform actual guest GS PPM at built/frames/ and `built/reports/native_frame_status.txt`. A missing frame means no verified game drawing; check runtime logs/PC and unsupported ops. AOT workflow concurrency avoids overlapping writes to main.

## Latest CP05 measured blockers
Probe from CP03 hosted run: 2/3 C++ compile units pass, remaining entry_3b6990 failed due missing SSE4.1. CP05 workflow adds `-DCMAKE_C_FLAGS=-msse4.1 -DCMAKE_CXX_FLAGS=-msse4.1` and PCH off; read next `built/reports/native_compile_probe.json` to verify 3/3. If passed, CI attempts full native link automatically on workflow push; inspect built/reports/native_link_status.txt. Unhandled report: 2963 events, 1735 in sub_003C01D8 and 813 in sub_003FDD40; these appear to span non-code/data. Prioritize robust CFG/function-boundary analysis, not unsupported-opcode stubs. Once binary links, Xvfb test checks for real nonuniform GS frame. Do not claim frames from pipeline infrastructure.


## CP06 GitHub CI race repair
An earlier AOT job failed publishing reports due an add/add conflict while rebasing `built/aot/generated_cpp.tar.gz` and built/reports files. CP06 CI publish script stages only built outputs outside Git tree, resets to fresh origin/main, reapplies and retries push without rewriting source. Wait for current hosted work to complete, then read next reports. Never force push or revert current source to older CI snapshot.

## CP07 — Full native compiler error visibility
CP05 Actions run `41241502970` compiled all three selected real AOT translation units (`native_compile_probe.json`: 3/3). The first full native `ps2EntryRunner` build step failed, **without** producing a game executable or any frames. The failure's redirected `native-build.log` was lost because GitHub's bash shell inherits `-e` and the nonzero build command aborted before writing `native_link_status.txt`. CP07 fixes the error-handling path, retains the exit status and compiler error lines, and checks the same pattern for configure/boot probes. After the latest AOT workflow completes, read `built/reports/native_link_status.txt`, fix the concrete first compiler errors, repeat until successfully linking. Do not infer boot success from `3/3` probes.
