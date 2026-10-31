# Built binaries and diagnostics (development)

GitHub Actions publishes verified launcher builds to `built/linux-x86_64/GodHand_Recomp_Launcher` and, **only if the full native AOT build links**, `built/linux-x86_64/GodHand_Recomp_Game` alongside it. The real game must not be confused with the SDL input diagnostic or the original PS2Recomp runner without generated guest functions.

`godhandLauncher/install_prebuilt.sh` copies the launcher and any actual native game build together into `~/.local/bin/`. The launcher also searches for the game binary next to itself when launched from `built/linux-x86_64/`, as requested. The local ISO remains under `~/.local/share/HodgkinsStudios/GodHand_Recomp/`.

The `GodHand - Link AOT and test first frame` workflow reuses the already-generated `built/aot/generated_cpp.tar.gz` Git LFS object so it does not redownload the ISO unless a linked executable exists and the guest framebuffer test can run. Full build failures are written into `built/reports/link_ci_status.txt` and result in a failed job rather than false success.

**No playable native game or real rendered frame should be claimed until the hosted link and runtime tests actually pass.**
