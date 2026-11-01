# GodHand_Recomp

**God Hand — PlayStation 2 static recompilation project**

A work-in-progress project intended to explore a native PC port of *God Hand* using the open-source [PS2Recomp](https://github.com/ran-j/PS2Recomp) static-recompilation toolchain.

**Current status: project foundation / toolchain import.** This repository currently contains a baseline PS2Recomp source tree and the documentation needed to begin game-specific development. **It does not yet contain a working God Hand recompilation, a verified game boot, or a playable port.** No God Hand ISO, original executable, or extracted game assets are included.

The objective is to analyze a user-supplied, legally obtained PlayStation 2 disc image *locally*, generate or adapt recompiled C++ code, and extend the host runtime as needed. Progress and compatibility will be reported only after they have been measured.

## Project ownership and upstream credit

**GodHand_Recomp** is a separate, game-focused development project maintained by **HodgkinsStudios**. It is **based on PS2Recomp by ran-j and its contributors**, rather than being a new recompiler written from scratch.

- Original PS2Recomp source: **https://github.com/ran-j/PS2Recomp**
- Original PS2Recomp wiki: https://github.com/ran-j/PS2Recomp/wiki
- Original project and contributor credit, license and provenance: [docs/UPSTREAM_AND_LICENSE.md](docs/UPSTREAM_AND_LICENSE.md)
- The included baseline runtime, analyzer, recompiler, studio and tests are upstream-derived unless later explicitly documented as changed here.

The bundled `LICENSE` file is retained unchanged and the repository is distributed under the **GNU General Public License v3.0** consistent with the upstream code. Attribution does not imply endorsement, sponsorship or affiliation by upstream authors.

*God Hand* was developed by Clover Studio and published by Capcom. This unofficial project is not affiliated with or endorsed by Capcom, Clover Studio, or the creators of PS2Recomp.

## Goals

1. Maintain an editable, version-controlled PS2Recomp-based toolchain, building on upstream work with clear change tracking.
2. Perform local analysis and static recompilation of the game's original PS2 executable and any relevant modules or overlays.
3. Extend the host-side runtime and hardware/system interfaces as required, favoring reusable implementations and reproducible diagnostics.
4. Establish deterministic tests, collect boot and performance evidence, and advance toward a stable native build.
5. Keep copyrighted retail game data out of GitHub and out of public CI artifacts.

These are **planned goals**, not claims of completed functionality. See [docs/PROJECT_STATUS.md](docs/PROJECT_STATUS.md) for the current baseline and next steps.

## What's in this repository?

This source snapshot includes these **PS2Recomp-derived** components:

| Directory | Purpose |
| --- | --- |
| `ps2xAnalyzer/` | PS2 ELF analysis, function discovery and TOML configuration generation. |
| `ps2xRecomp/` | R5900 static recompiler that generates C++ from ELF/configuration input. |
| `ps2xRuntime/` | Host runtime, PS2 execution environment, GS/VU/audio and host integration code. |
| `ps2xIOP/` | IOP/IRX emulation and related system services. |
| `ps2xStudio/` | Desktop development/debugging tools. |
| `ps2xTest/` | Existing PS2Recomp test programs and fixtures. |
| `android/`, `vita/` | Existing upstream platform-specific scaffolding. |
| `.github/workflows/build.yml` | Existing **upstream** hosted build/test workflow; not a local-ISO runner. |
| `docs/` | GodHand_Recomp-specific project documentation. |

The module names, CMake project names and program names are intentionally left as upstream identifiers in this initial documentation-only import. This avoids presenting a cosmetic rename as an engine rewrite or breaking build targets.

## Requirements and baseline build

The inherited toolchain uses **C++20** (not C++17), **CMake 3.21+** at the repository root, and external dependencies fetched during CMake configuration. Some x86 runtime paths depend on SIMD support. Linux builds also use OpenGL, X11 and FFmpeg-related libraries.

For Ubuntu, an example dependency setup based on the included upstream CI workflow is:

```bash
sudo apt update
sudo apt install -y git build-essential cmake ninja-build pkg-config \
  libgl-dev libx11-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev \
  libavcodec-dev libavformat-dev libavutil-dev \
  libswresample-dev libswscale-dev
```

Configure, compile and run the inherited test executable:

```bash
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS=-msse4.1 -DCMAKE_CXX_FLAGS=-msse4.1
cmake --build build --parallel 2
./build/ps2xTest/ps2x_tests
```

These commands describe the upstream build path, **not a build verified during this documentation pass**. Only use the SIMD flags on compatible x86/x64 CPUs. The source may require additional dependencies or fixes on a particular host. The test suite exercises toolchain components; it does not demonstrate that *God Hand* is playable.

For tools/configuration and the Ghidra function export workflow, consult the retained upstream docs: [PS2 ELF Analyzer](ps2xAnalyzer/Readme.md), [IOP subsystem](ps2xIOP/README.md), and [Runtime library](ps2xRuntime/Readme.md). See upstream's [wiki](https://github.com/ran-j/PS2Recomp/wiki) for broader usage reference.

## GodHand:Recomp launcher — first implementation pass

The new [`godhandLauncher/`](godhandLauncher/README.md) component is a standalone C++17/GTK3 Ubuntu launcher developed for this project (the inherited PS2Recomp toolchain itself still uses C++20).

- Displays **GodHand:Recomp** with **HodgkinsStudios** branding.
- **Load God Hand ISO** opens the Linux native file picker.
- Copies the selected ISO to `~/.local/share/HodgkinsStudios/GodHand_Recomp/GodHand.iso` and extracts its ISO9660 contents to the sibling `Extracted_Iso/` directory. Both remain outside the repository.
- Shows **Play** after installation and searches for a future `bin/GodHand_Recomp_Game` under the install root. Until an AOT native game executable has been generated, Play displays an explicit error; it **does not** run the retail ISO in an emulator.
- The installer is tested using a generated *synthetic* ISO; no retail game data is bundled or used in CI.

For a single-command local install (including an application-menu entry), run `bash godhandLauncher/install_launcher.sh`. To compile the standalone launcher manually on Ubuntu:

```bash
sudo apt install -y cmake g++ pkg-config libgtk-3-dev libarchive-dev
cmake -S godhandLauncher -B build-godhand-launcher -DCMAKE_BUILD_TYPE=Release
cmake --build build-godhand-launcher --parallel 2
ctest --test-dir build-godhand-launcher --output-on-failure
./build-godhand-launcher/GodHand_Recomp_Launcher
```

This first-pass launcher does not imply game compatibility or completion. See the [launcher technical notes](godhandLauncher/README.md).

## Local game files, not on GitHub

The goal is to keep your own God Hand ISO on your PC. It should **never** be committed to this repository or included in GitHub Actions artifacts.

For example, keep game data in a separate path **outside this repository**:

```text
~/PS2GameData/GodHand/
    GodHand.iso
    extracted/
    work/
```

Avoid copying ISO contents, game executables, generated proprietary code or media into tracked source folders. `.gitignore` covers common accidental file extensions and private directories, but **does not replace checking `git status` before every push**.

Instructions for safe local paths, checks and the planned self-hosted Actions runner are in [docs/LOCAL_ISO_WORKFLOW.md](docs/LOCAL_ISO_WORKFLOW.md). **The local-runner workflow exists, but runner availability and the local ISO have not been verified in this development environment.**

## Development documentation

- [Project status and roadmap](docs/PROJECT_STATUS.md)
- [Local ISO and future GitHub Actions setup](docs/LOCAL_ISO_WORKFLOW.md)
- [Upstream origins, credits and license](docs/UPSTREAM_AND_LICENSE.md)
- [Change history](CHANGELOG.md)

## License and legal notes

This repository retains the upstream **GPL-3.0** [LICENSE](LICENSE) and the required source-level notices. Separate dependencies can have their own licenses; review them before distributing builds. This repository does **not** grant rights to the *God Hand* game, its assets or PS2 firmware. Users are responsible for obtaining and using their own game files lawfully. No copyrighted game data is distributed here.

## CP02: launcher Settings and GitHub-hosted AOT bring-up

The `godhandLauncher/` GTK3 frontend now provides a keyboard/controller Settings dialog with SDL2 hotplug enumeration and persistent screen mode settings. The launcher copies/extracts the selected ISO to the local user data directory and changes Load ISO to Play; Play continues to require a real native game binary. See [input and display status](docs/INPUT_AND_DISPLAY.md).

GitHub-hosted Actions builds and tests the launcher, then commits the verified Linux executable under `built/linux-x86_64/`. The `godhand-aot.yml` workflow reconstructs the existing `iso/GodHand.iso.part-*` LFS files on an ephemeral runner, locates the ISO9660 `SYSTEM.CNF` boot ELF, checks its MIPS ELF header, and attempts upstream analysis/static recompilation. Reports and any generated C++ archive go in `built/`, not a pretend playable game. [Built outputs](built/README.md).

This is experimental; the public ISO and any AOT derivatives are visible to anyone with repository access. Only distribute material you are authorized to share. Making the repository private later does not undo previous public exposure.

**Prebuilt Ubuntu launcher:** After pulling `main` and installing the GTK3, SDL2 and libarchive runtime libraries, run `bash godhandLauncher/install_prebuilt.sh`. No compilation is needed. This is a launcher build, not a playable God Hand AOT game.

**Verified CP02 ISO analysis:** The GitHub-hosted ISO workflow found `SLUS_215.03` (MIPS ELF32, entry `0x00100008`) and emitted 9,231 C++ files. Recompiler logs contain unsupported-instruction diagnostics and **no game executable has been linked**. Full report: [CP02 real-disc results](docs/CP02_REAL_ISO_REPORT.md).

## CP03 native AOT integration
After the hosted ISO recompiler generates its `output/` directory, `ps2EntryRunner` can now include the generated translation units and registration table through `-DPS2X_AOT_SOURCE_DIR=/absolute/path/to/output`. The hosted AOT workflow attempts runtime configuration and a compiler probe and writes reports under `built/reports/`. A full native build is opt-in through the `attempt_full_native_build` workflow-dispatch input. Successful C++ generation does **not** mean the game is booting or rendering frames; unsupported R5900 instructions and function-boundary problems are still under investigation.

## Real guest frame verification (CP04)
The native runtime optionally writes a first **non-uniform guest GS framebuffer** (not a launcher/host placeholder) as a PPM image when `GODHAND_GS_FRAME_PPM` is set to an absolute writable path. The launcher sets it to `~/.local/share/HodgkinsStudios/GodHand_Recomp/first_real_gs_frame.ppm`. The capture will not be produced unless native execution reaches the GS presentation backend and yields varied pixels. It does not imply accurate graphics or a successful game boot. No such frame is verified yet.

## First native execution/frame probe (CP05)
When the generated AOT C++ compiles correctly, start the hosted `GodHand - ISO boot ELF and AOT analysis` workflow manually with `attempt_full_native_build=true`. This enables a full `ps2EntryRunner` link and, only if linked, a 75-second virtual-display run against the real boot ELF. The runner records exit codes, runtime diagnostics and whether a non-uniform GS framebuffer was captured (`built/reports/native_frame_status.txt`). Any frame is stored as `built/frames/first_real_gs_frame.ppm`. As of this checkpoint, **no God Hand-native execution or actual frames have been validated**; compiler and recompiler errors remain outstanding.

**Latest native compiler probe:** Boot-entry and registration-table C++ compiled on hosted Ubuntu (2/3 tested units). The third generated unit initially failed due to missing SSE4.1 flags; subsequent workflow now supplies `-msse4.1`, disables PCH for the isolated probe, and automatically attempts full linking on push only if those initial compile probes pass. No executable or game frame has been confirmed. The unhandled guest-instruction report counts 2,963 events, mostly in inferred high-memory functions that require boundary verification.

### CP10 generated-code compiler triage
The hosted AOT workflow tests 24 generated translation units (boot entry, function table, suspicious function regions and address-stratified samples) before attempting the full runtime link. Its `built/reports/native_compile_probe.json` is diagnostic only; a successful sample does **not** mean all 9,231 units compile, that God Hand boots, or that GS frames have rendered. See `docs/CP10_BUILD_PROBE.md`.

### CP11 native first-boot build mode
The AOT-hosted workflow uses `PS2X_AOT_FAST_COMPILE=ON` to avoid expensive optimization and per-file LTO on thousands of generated guest translation units during first-frame debugging. It still compiles all generated functions, and keeps normal Release behavior for toolchain and launcher builds. The generated C++ is incomplete until all invalid guest code analysis and native runtime issues are resolved.

## CP12 — Native game build and launcher layout

The desktop launcher resolves `GodHand_Recomp_Game` **next to its own executable first**, then from the project checkout's `built/linux-x86_64/` folder, with a fallback for older `.local/share` installations. The ISO installation remains entirely separate and does not need to be repeated when installing updated binaries.

`GodHand - Link AOT and test first frame` is a distinct GitHub-hosted workflow that restores only the existing LFS generated-C++ archive for the native build. It saves failures under `built/reports/link_ci_*.txt` and only pushes `built/linux-x86_64/GodHand_Recomp_Game` after the full host executable compiles and links. After successful linkage, it fetches the ISO for the real GS framebuffer probe. This pipeline does **not** claim playability until guest execution produces a real frame.

Install the most recent prebuilt launcher and (when it exists) game with `bash godhandLauncher/install_prebuilt.sh` after `git pull origin main`.

## CP13 — Build the native AOT game on your own Ubuntu PC

From the project root, install build dependencies (once):

```bash
sudo apt update && sudo apt install -y build-essential cmake ninja-build git git-lfs python3 pkg-config libgl-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev
```

For normal incremental compilation using the already-generated 25 MB AOT C++ archive in `built/aot/`:

```bash
bash ./build_godhand.sh
```

The script fetches the AOT archive via Git LFS if necessary. Source ZIP downloads may only contain a Git LFS pointer; use a real `git clone`, or regenerate directly from your local ISO:

```bash
bash ./build_godhand.sh --regenerate
```

The default ISO path is `/home/hodgkinsstudios/Desktop/Roms/PS2/God Hand (USA).iso`; override with `--iso "/path/to/game.iso"`. The default is 2 parallel compiler jobs to keep RAM use manageable; set `--jobs 4` on faster computers. `--run` starts the built game with a locally extracted boot ELF (not necessarily playable yet). `--push` commits only a successfully linked binary and its checksum to GitHub main; do not publish game-derived binaries publicly without appropriate distribution rights.

Successful output goes to `built/linux-x86_64/GodHand_Recomp_Game` beside the launcher. Errors and progress go to `built/reports/local_build.log`. Rerunning without deleting `build-game-local/` performs an incremental build rather than starting 9,231 guest C++ files from scratch.

This is a development native build. Generating/linking guest code does **not** demonstrate that the game boots or produces frames.

## CP14 — Hosted AOT compilation checkpoints and caching

The native God Hand build now compiles all generated C++ in four deterministic Ninja batches using `tools/aot_batch_build.py`, with a GitHub Actions `ccache` checkpoint saved after each completed batch and restored on future runs. Batch commands show live compiler output rather than hiding it until the job exits. The full `ps2EntryRunner` link and genuine GS first-frame test remain mandatory. See `docs/CP14_NATIVE_AOT_COMPILE_CHECKPOINTS.md` for details, cache limitations and diagnostics. A fresh run must still compile previously uncached code; this does not imply the game is playable.

## CP16 — Safe native runtime investigation
God Hand's real generated AOT executable is **already linked** (209,103,376 bytes) and stored through Git LFS in `built/linux-x86_64/GodHand_Recomp_Game`. The launcher finds a sibling game binary; download the LFS payload with `git lfs pull` after pulling main. The game is **not yet playable**: both a 90-second test and an independent bounded 20-second prebuilt-only smoke test entered the guest at `0x00100008` but stalled on a missing `JALR` target `0x003BAFB8` called at `0x0031DB2C`; no genuine GS frame was produced. The function list lacks an entry at the target (neighboring starts `0x003BAD80` and `0x003BAFF8`). Both heavyweight full AOT analysis and native-link workflows are now manual-only, avoiding accidental large rebuilds. The short smoke workflow writes `built/reports/prebuilt_smoke_status.txt`, and its last report is on main.


### CP18 current native AOT state
The real CP17 native AOT executable (209,109,192 bytes) and generated C++ archive (25,001,106 bytes) are on GitHub main as Git LFS objects. The executable loads God Hand's original ELF but has no verified guest-rendered frame. A new ELF-backed guest entry recovery addresses the next observed JALR target 0x0038F200, preserving 0x003BAFB8. See docs/CP18_SECOND_GUEST_ENTRY.md.


### CP19 current boot-development checkpoint
CP18 generated AOT source archive and native game executable are both published on GitHub main, via Git LFS. Original ELF execution now passes two previously missing indirect-call targets; next blocked guest entry is 0x0038B6E0. CP19 validates and registers this genuine MIPS entry, then uses cached native recompilation. See docs/CP19_THIRD_GUEST_ENTRY.md.
