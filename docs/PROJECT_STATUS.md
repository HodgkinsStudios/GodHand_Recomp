# GodHand_Recomp — CP02 system status (2026-10-31)

PS2Recomp toolchain foundation and game ISO LFS parts present in GitHub (two LFS pointers verified from repository metadata). The ISO was inaccessible directly in the editing environment but **the GitHub-hosted AOT workflow has now verified the real boot ELF and generated C++**. It identified `SLUS_215.03` (MIPS ELF32, entry `0x00100008`), generated 9,231 C++ files, and returned recompiler code 0. This is **not** a linked game executable: the logs include numerous unhandled-instruction diagnostics and the game has not booted. See [real ISO report](CP02_REAL_ISO_REPORT.md).

## Implemented in CP01/CP02 source
- GTK3 GodHand:Recomp launcher, local ISO copy and `Extracted_Iso` extraction, Load ISO → Play transition (Play requires an actual AOT game executable).
- GTK Settings page with keyboard/gamepad remap, SDL2 hot-plug polling and persisted window mode selection. Native game Esc cycling still needs wiring into the future game renderer.
- ISO9660 `SYSTEM.CNF` and MIPS ELF32 scanner, test coverage with generated synthetic disc data.
- GitHub-hosted Linux launcher build/test workflow, compiled executable committed under `built/linux-x86_64/` when CI succeeds.
- GitHub-hosted ISO AOT investigation pipeline that reconstructs LFS ISO, reports boot ELF, builds analyzer/recompiler and attempts static source generation. Diagnostic `built/reports/` and generated source tarball `built/aot/` are committed when available.

## Limitations
- Native God Hand executable is not yet assembled or linked; no gameplay/rendering/audio fidelity.
- PS2 runtime integration, VU/GS/IOP, assets, gameplay and controller binding parity not verified.
- C++ GTK/SDL2 UI compile awaits GitHub-hosted runner with appropriate development packages; local execution environment lacks these headers. Installer and settings libraries were compiled and their tests passed locally.
- The first ISO/AOT workflow has completed and reports are now in `built/reports/`; verify function boundaries and unsupported instructions before any claims of a native build.

## Next checkpoint tasks
1. Review cloud CI results in Actions, correct launcher compiler errors, verify Linux binary in `built/`.
2. Inspect `built/reports/iso_report.json`, analyzer/recompiler logs and generated C++ size.
3. Use Ghidra function maps where analyzer heuristics are insufficient; identify overlays and runtime blockers.
4. Implement a real AOT game entry point, load-time asset mapping, video/input/audio, and integrate Esc display cycling into the game loop.

## CP03: native AOT integration pass
The previous runtime did not include generated function code when building `ps2EntryRunner`. This checkpoint wires a configurable generated source tree into the native runner (`-DPS2X_AOT_SOURCE_DIR=/absolute/path/to/generated/output`), including the required guest registration function table. The launcher now passes the installed boot ELF (`SLUS_215.03`) when the future executable is launched. Game code is not built from the raw ISO at launcher startup; this is a compile-time integration step on hosted CI.

The updated AOT workflow now archives generated headers and C++, reports unsupported instruction counts/functions, configures the native runtime and probes actual compiler commands. For a full native attempt, manually run `godhand-aot.yml` with `attempt_full_native_build=true`. The workflow can only publish a game executable if compilation and linking actually succeed. No real game frames are confirmed by this checkpoint.
