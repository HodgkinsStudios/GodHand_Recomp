# GodHand_Recomp CP02 authoritative handoff

Continue HodgkinsStudios/GodHand_Recomp on GitHub `main`. Source originally derives from https://github.com/ran-j/PS2Recomp (retain GPL-3.0 and credit). Do not restart the project. Check latest `main` and latest GitHub-hosted Actions runs before edits.

Latest work: CP01 launcher installed user-supplied ISO into ~/.local/share/HodgkinsStudios/GodHand_Recomp/GodHand.iso and Extracted_Iso; CP02 adds Settings keyboard/gamepad rebind, SDL2 hotplug, screen mode configuration; custom ISO9660 boot ELF scanner; CI launcher build and ISO/AOT workflows. `built/` holds tested development outputs after CI. Game not booting yet.

The ISO is tracked as Git LFS chunks `iso/GodHand.iso.part-000` and `part-001`; cloud Actions checkout must set `lfs: true` to get actual bytes. Run the workflow `.github/workflows/godhand-aot.yml` on hosted Ubuntu only, inspect `built/reports/` results, and continue analysis and runtime implementation. Do not claim playability from generated C++ alone.

Important: temporary repository is public and contains retail-game-derived data. User says they have permissions and plans a clean repo at release. Do not introduce extra proprietary files without considering exposure; do not commit the uncompressed ISO. Push changes to `main` when verified, maintain handoff notes, checkpoint.num and implementation history.
