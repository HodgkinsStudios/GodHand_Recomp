# CP25 — Native launch crash diagnostics (not a frame milestone)

The GTK3 launcher's prior Play path used GLib's default detached child mode, reported "Started" immediately, and did not check exit status. A game process that exited before creating a window was indistinguishable from a running game.

## Implemented
- Validate the selected native game file starts with the Linux ELF magic, and explicitly detect Git LFS pointer text (the tracked game is an LFS object). Users are instructed to run `git lfs pull` rather than launch the pointer.
- On Play, create `~/.local/share/HodgkinsStudios/GodHand_Recomp/logs/last_launch.log`; capture native stdout and stderr via the child process's standard descriptors.
- Use `G_SPAWN_DO_NOT_REAP_CHILD` and a GLib child watch to report the native process's termination, preserve the launcher window, and re-enable Play.
- Prevent duplicate launches while one native child is running. Preserve boot ELF argument, environment, and installed assets unchanged.
- Extend locator unit tests for invalid executables, LFS pointer detection, and ELF headers. The prebuilt installer now skips unresolved LFS pointers.

## Verification boundary
This fixes **silent launch failure reporting**, not the underlying guest startup or rendering. The last hosted native-link test reached guest loop 0x00398930 for its allotted time and captured no nonuniform guest GS framebuffer. Investigate the real CDVDFSV SearchFile RPC service binding (SID 0x80000597) using measured IOP server state; avoid dummy success responses, fabricated guest pixels or game-specific skips. The logfile and process exit status should first distinguish the user's immediate-exit case from the hosted guest loop.

## Reproduction
Install the launcher using `bash godhandLauncher/install_prebuilt.sh` after `git lfs pull`. Click Play with the previously extracted ISO. Check the new status or error dialog and `logs/last_launch.log` under the user data folder. The log may contain game file paths and should be reviewed before sharing publicly.
