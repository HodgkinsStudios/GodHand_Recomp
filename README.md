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
