# CP47 — God Hand authentic rendering blocker status and continuation

**Objective:** obtain a genuine nonuniform PS2 GS frame from original game execution; no fake frame, game-specific execution shortcuts, skipped MIPS opcodes, invented return values, or synthetic disc responses.

## Confirmed progress

- The original ISO's entire `_DATA.AFS` (1,445 top-level original entries) is extracted and verified by the native launcher C++17 implementation; deletion recovery was successfully exercised on GitHub Actions run `41370833796`.
- The authentic EE startup progressed through original module initialization after missing original guest AOT entrypoints `0x003364F8`, `0x00334230`, `0x003329D0` were safely restored to existing translated MIPS instructions. This is preserved in `built/reports/aot_promoted_entries.json`.
- CP41 proved the next bug is not a missing ordinary AOT function. The guest dispatcher resumed at original code `0x002BE014`, which restores `$ra` from guest stack, returning to **unaligned address 0x00000F82**. Measured post-epilogue guest SP is `0x01FFFF30`; the saved return slot was `0x01FFFF10`. The scheduler correctly rejects it.
- CP45 original generated-code cross-reference (successful run `41374071884`, report `built/reports/ee_return_callers_2bdff8.txt`) proves the original caller uses `jal` at `0x0030B384` and sets the legitimate return address **0x0030B38C**. Exactly one direct original AOT caller was found; the original generated AOT source contains no literal `0xF82`.
- CP46 correct original ISO headless prebuilt replay (run `41374504004`, report `built/reports/prebuilt_smoke_status.txt`) reproduced the unaligned return and recorded **`nonuniform_guest_gs_frame=no`**, although GS register traffic is observable. The run's red result reflects the intended strict no-frame test failure, not compiler failure.
- CP43 (native workflow run `41371344944`) is compiling the additional original EE `WRITE8/16/32/64/128` watchpoint instrumented in the AOT header, gated by `PS2X_GUEST_WATCH_ADDR=0x1ffff10`. It is the first test that can directly record original guest writes to the saved return stack slot.
- CP44 (run `41372647716`) is queued behind CP43; it additionally compares the same eight guest RAM bytes before and after every native guest dispatcher invocation, from scheduler .cpp, to catch DMA or other mutations bypassing generated AOT memory macros.
- CP46 updated `.github/workflows/godhand-prebuilt-smoke.yml` to replay an authentic original ISO with `PS2X_CD_IMAGE` and relevant diagnostics automatically after successful native link, without needing a fresh recompile.

## Necessary next actions

1. Verify whether CP43 and CP44 workflows finished. Read actual `built/reports/link_ci_frame.txt` and the original full log artifacts, especially `[EE:stack-write]` and `[EE:stack-dispatch-delta]`.
2. Compare saved slot writes with **expected 0x0030B38C** and erroneous **0x00000F82**. A normal `WRITE64` at original callee `0x002BE000` should save the caller's RA. Determine whether the value is wrong on entry, clobbered by a guest store, or modified by native/IOP DMA.
3. Correct the **generic** EE register, stack, memory or dispatch implementation implicated by the actual write provenance. Do **not** patch the original game to force `$ra=0x30B38C`, skip a call, or fabricate GS rendering.
4. Rebuild affected source, rerun genuine ISO under Xvfb, verify no old regressions, and require an actual `first_real_gs_frame.ppm` before declaring graphical success.
5. If execution advances and hits further authentic AOT entrypoints, use existing verified cumulative promotion tool rather than ad hoc generated instruction stubs.

## Build caution

CP43 changed a shared header imported by ~9,362 guest AOT C++ translation units, forcing a costly native recompile (the runner was on compilation batch 2/4 at the prior status check). CP44's additional read-only instrumentation is confined to a scheduler implementation file to avoid repeating the expensive invalidation. The native link workflow retains compiler cache across batches. Do not cancel a compiling checkpoint merely because subsequent source-only changes are ready.

## Ground truth

The **last verified display outcome** is `nonuniform_guest_gs_frame=no`. No real game image has been produced or claimed. Preserve the source and checkpoints on the existing `HodgkinsStudios/GodHand_Recomp` main branch; do not restart or use older AOT outputs.
