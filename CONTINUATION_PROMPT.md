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

## CP08 — Cross-platform build blocker
A GitHub-hosted `build` run 41241717761 failed all three jobs: Linux GCC, Clang, and Windows MSVC. Root cause was malformed raw newline in CP04's new frame-capture `std::cout` string literal in `ps2xRuntime/src/lib/ps2_runtime.cpp` at line 446. CP08 fixes this syntax error and adds `test_frame_capture_build_syntax.py` running a C++17 syntax-only check on the exact statement. Verify fresh hosted `build` CI passes before diagnosing further native AOT failures. CP07 collects redirected full native compiler errors under `built/reports/native_link_status.txt` on its next run. **Do not claim frames without evidence.**

## CP09 — Required AOT verification after core edits
The GitHub AOT workflow previously only ran for changes to the analysis script/workflow, so CP08 C++ fixes were not automatically native-tested. CP09 widens push paths for runtime/recompiler/analyzer/IOP/CMake. Watch the new hosted AOT run and check `built/reports/native_link_status.txt` (new CP07 log-capture fix). Native game link and frame generation are unverified until new run demonstrates both.

## CP10 — Stratified generated-code compiler triage
`tools/aot_compile_probe.py` now checks 24 translation units spread across ELF addresses and 3 high-error suspects with bounded 2-worker compilation; run `python3 -m unittest discover -s tools/tests -v`. Compare `built/reports/native_compile_probe.json` and `native_link_status.txt`; do not assume game works from compiler success. Hosted CI re-downloads large ISO LFS parts and may take time. No confirmed game frames.

## CP11 — AOT fast development compilation
AOT compile is extremely large (9,231 guest C++ sources). The hosted workflow sets `-DPS2X_AOT_FAST_COMPILE=ON`; only generated translation units receive unoptimized compiler options (GCC/Clang `-O0 -g0 -fno-lto`, MSVC `/Od /GL-`). This does not skip or stub any function. Plain Release builds remain unchanged. Monitor full-link failure report after the most recent hosted build; no verified boot or frames yet.

## CP12 continuation
Check workflow `GodHand - Link AOT and test first frame` first, inspect `built/reports/link_ci_status.txt` and `link_ci_compile_probe.json`, fix the **first true compiler/linker error** and repeat. The native output `built/linux-x86_64/GodHand_Recomp_Game` is only published after successful full linking; launcher resolves sibling binary first. Do not claim real frames without captured guest GS data.

## CP14 — Native AOT build optimization handoff
Follow `docs/CP14_NATIVE_AOT_COMPILE_CHECKPOINTS.md`. Four batch steps stream progress and save caches. Check `built/reports/link_ci_batches.txt`, `link_ci_status.txt`, and `link_ci_frame.txt` after the fresh workflow. If any batch fails, inspect the archived `godhand-compiler-logs-<run id>` artifact and fix the first actual compiler failure. Never claim a game executable or real frame until verified.

## CP16 authoritative handoff — real game executable linked, boot blocked
Use latest GitHub main; do **not** claim the game isn't built. The real AOT binary `built/linux-x86_64/GodHand_Recomp_Game` is tracked in LFS (209,103,376 bytes, SHA-256 `84bbaee605ceef244de11bf6dac0654ecf416f879a2094cc1059e64107867eda`). Restore it with `git lfs pull --include='built/linux-x86_64/GodHand_Recomp_Game' --exclude=''`. The hosted native link completed successfully on run `41259018043`, with 9,231 compiler-cache hits. Real execution entered guest MIPS ELF but failed on indirect call at `0x0031DB2C` to missing target `0x003BAFB8`; no genuine GS frame. Independent short smoke `41264810842` reproduced it in 20 seconds, recorded `built/reports/prebuilt_smoke_status.txt`. Near-target registered generated files are `sub_003BAD80_0x3bad80.cpp` and `sub_003BAFF8_0x3baff8.cpp`; there is no exact `0x003BAFB8` entry. Investigate MIPS function boundaries, dynamic targets, and authoritative instruction decoding. Do not fake graphics, create game-specific stubs, or rebuild 9,231 TUs for diagnostic-only changes. Both expensive workflow types are now manual-only; use prebuilt smoke for short regression checks. The successful `link_ci_*` reports from `e6676428afdf` have been restored to avoid stale results.


## CP18 authoritative handoff
Use the newest GitHub main. CP17 linked real AOT game (209,109,192 bytes, SHA-256 abc377f6d91f3a3767cd31662c814ae1f6c2fff91ab782d6504d6d2987297971) and generated AOT archive (25,001,106 bytes, SHA-256 cbcd5bd370339e62971d20aa05ce021804d27cda9ecafb735be1ce6cea866423); both tracked through Git LFS. CP17 boot progressed past 0x003BAFB8 but stalled at the next missing guest JALR target 0x0038F200 (called at 0x0031DB2C); zero GS traffic, no verified frame. CP18 passes both verified MIPS entries to the analyzer/recompiler and checks both in its generated function registry before publishing. The native link workflow relinks cached objects automatically after successful AOT analysis; do not replace the working old binary on failure. Inspect fresh link_ci_frame.txt to find next authentic guest target; do not create fake game code or GS frames.


## CP19 authoritative handoff
The CP18 real game executable is published as built/linux-x86_64/GodHand_Recomp_Game (Git LFS SHA-256 40f776e1f95b76723d74ce745d7c668f3865d7472a986b1129cfef26e3e11058, 209,109,192 bytes). Its generated AOT archive is built/aot/generated_cpp.tar.gz (LFS SHA-256 9c140fcb3992aa2eae2f3bba7d3e36d354be71112abb4c9c695de20fa3ae7d74, 25,005,158 bytes). CP18 native link succeeded, boot reaches past the first two previously absent JALR targets but stalls on 0x0038B6E0, near the 0x0038B6E8 generated function start. CP19 requests an ELF-backed genuine guest entry (three observed targets total) and validates all entries in the generated registry before publication. Inspect the current native link and frame run. Do not regress to CP17, discard built LFS artifacts, add fake graphics, or patch game instructions.

## CP20 authoritative handoff
Start from GitHub main and the newest published AOT archive and native executable, not earlier checkpoints. CP19 linked 209,109,192-byte AOT native binary SHA-256 6532ea3b9b66eb06b6cece1523c9b9f9fe9a7e75a4be3352bf364f99878f795b, in built/linux-x86_64/GodHand_Recomp_Game via LFS. CP19 frame log shows original guest code passing three former missing indirect calls, now stalled at JALR source 0x0031DB2C to 0x0031DA28, with callback data s0[8]=0x00375840. CP20 requests original ELF-backed entry discovery for these two additional targets (all five retained), generated registry check, cached native relink and real Xvfb GS test. Verify exact new link_ci_frame.txt and real guest GS writes; no fake frame or guest stub.

## CP21 continuation — generic callback table research
Review `built/reports/code_pointer_audit.json` published by prebuilt smoke CI for original ELF PT_LOAD pointer groups. Tool `tools/scan_guest_pointer_tables.py` runs six unit tests and reports pointer clusters in executable and data segments, **not automatic AOT hints**; validate actual control flow before promoting candidate entries. CP20 separately seeds actual JALR at 0x0031DA28 and callback candidate 0x00375840 and is rebuilding. Check current GitHub main and the latest native link/frame reports; keep AOT artifacts in Git LFS, no synthetic frames.

## CP22 continuation — general MIPS function discovery
The original ELF callback-pointer audit is available in `built/reports/code_pointer_audit.json` (1.24MB, 358 groups, all within executable PT_LOAD), and a 71-pointer group starting at 0x003FC884 contains several addresses previously missing from guest dispatch. CP22 updates `ps2xRecomp/src/lib/elf_parser.cpp` to detect 4+ consecutive file-backed executable pointers whose targets ALL have conservative MIPS callable shapes; candidate functions are original guest instructions only. Build CI must confirm C++ syntax before native AOT reanalysis. Preserve CP20 binary until new AOT is linked, then inspect `built/reports/link_ci_frame.txt` for GS writes and actual frame. No guest patching, stubs or artificial graphics.
