# GodHand_Recomp — Project status

**Checkpoint:** Initial project documentation / baseline import (2026-10-31)  
**Phase:** Foundation, prior to God Hand-specific implementation  
**Verification:** Source archive inspected; no compiler build or game execution carried out for this checkpoint.

## What exists now

The supplied source archive was the PS2Recomp toolchain snapshot (`PS2Recomp-main.zip`), including the upstream analyzer, R5900 recompiler, runtime, IOP subsystem, studio, tests, and existing CI workflow. It has been copied as the **working baseline** and wrapped in project-specific documentation. This checkpoint also adds Git ignore rules for common game-disc and private output files.

No God Hand-specific source files, local disc image, extracted ELF, per-game recompiler configuration, generated game code, or runtime implementation are claimed to exist in this archive.

## Status by area

| Area | Baseline | God Hand-specific progress |
| --- | --- | --- |
| Source repository | PS2Recomp-derived source included | Project documentation established |
| Analyzer / recompiler | Upstream components present | No God Hand analysis verified |
| EE code / overlays | Game input not provided in this archive | Not started |
| Host runtime (GS/VU/IOP/audio/input) | Upstream implementation present | No game-specific readiness verified |
| Builds / automated tests | Upstream CI workflow and tests present | Not run in this pass |
| Game boot, visuals, audio, controls | No game integrated | Not tested |
| Local-only game file workflow | Documented and ignore rules added | Runner not configured |

## Planned next development stages

1. Initialize the dedicated `GodHand_Recomp` Git repository and push this source baseline.
2. Record a successful baseline local toolchain build and full existing test result (or failures) without game data.
3. Configure a dedicated, restricted **self-hosted** GitHub Actions runner, requiring manual approval/dispatch for jobs that need the local ISO.
4. Verify a locally stored, user-supplied disc and identify the exact game build/ELF, recording hashes **locally**.
5. Generate function maps/configuration, recompile the game executable, and identify overlay and runtime blockers.
6. Implement and test genuine runtime compatibility improvements; save reproducible status logs with no game asset leakage.

Use status descriptions supported by actual test data. Do not mark the game playable or complete based only on a successful compiler build. For project policy and ISO isolation, see [LOCAL_ISO_WORKFLOW.md](LOCAL_ISO_WORKFLOW.md).
