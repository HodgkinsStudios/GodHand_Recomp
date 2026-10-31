# CP13 — Local Ubuntu native build

The hosted AOT job compiles approximately 9,231 generated guest translation units, with additional PS2Recomp host runtime and libraries. This is a full native executable build, not the small GTK launcher build. The hosted job uses four parallel compiler processes and allows a lengthy build window. Repeated fresh Actions runners discard compiler object caches. Its build step redirects Ninja output to a diagnostic log, so the web view may show no incremental output.

`bash ./build_godhand.sh` in the source root reuses `built/aot/generated_cpp.tar.gz`, the exact AOT archive previously generated from the ISO. Use `--regenerate` only if you want to rerun the analyzer/recompiler from the ISO. Use `--jobs N` to tune CPU/RAM pressure. The script writes a full compiler transcript to `built/reports/local_build.log`, and on successful link copies the native executable to `built/linux-x86_64/GodHand_Recomp_Game`.

The local default ISO path contains spaces and is properly quoted. The ISO is used with `--regenerate` or `--run`, not for the normal cached native build. The GTK launcher (CP12+) searches for the AOT executable in the same directory.

Prerequisites: `sudo apt install -y build-essential cmake ninja-build git git-lfs python3 pkg-config libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev`.

The native game binary is not yet verified playable and no genuine game frames have been reported. Compiler errors and incomplete recompiled functions still need investigation.
