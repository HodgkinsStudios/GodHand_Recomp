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
