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
