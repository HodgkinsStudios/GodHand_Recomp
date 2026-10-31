# GodHand_Recomp — CP02 system status (2026-10-31)

PS2Recomp toolchain foundation and game ISO LFS parts present in GitHub (two LFS pointers verified from repository metadata). The game image was not directly accessible in the editing environment; **real God Hand ELF analysis, generated code, and game boot have not yet been verified**. A successful existing hosted toolchain build does not imply gameplay.

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
- GitHub-hosted jobs must run and produce reports to identify the actual ISO's boot ELF; not invented.

## Next checkpoint tasks
1. Review cloud CI results in Actions, correct launcher compiler errors, verify Linux binary in `built/`.
2. Inspect `built/reports/iso_report.json`, analyzer/recompiler logs and generated C++ size.
3. Use Ghidra function maps where analyzer heuristics are insufficient; identify overlays and runtime blockers.
4. Implement a real AOT game entry point, load-time asset mapping, video/input/audio, and integrate Esc display cycling into the game loop.
