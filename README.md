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

Instructions for safe local paths, checks and the planned self-hosted Actions runner are in [docs/LOCAL_ISO_WORKFLOW.md](docs/LOCAL_ISO_WORKFLOW.md). **No self-hosted runner has been configured by this checkpoint.**

## Development documentation

- [Project status and roadmap](docs/PROJECT_STATUS.md)
- [Local ISO and future GitHub Actions setup](docs/LOCAL_ISO_WORKFLOW.md)
- [Upstream origins, credits and license](docs/UPSTREAM_AND_LICENSE.md)
- [Change history](CHANGELOG.md)

## License and legal notes

This repository retains the upstream **GPL-3.0** [LICENSE](LICENSE) and the required source-level notices. Separate dependencies can have their own licenses; review them before distributing builds. This repository does **not** grant rights to the *God Hand* game, its assets or PS2 firmware. Users are responsible for obtaining and using their own game files lawfully. No copyrighted game data is distributed here.
