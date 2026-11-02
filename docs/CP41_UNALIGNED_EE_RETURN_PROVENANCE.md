# CP41 — Trace unaligned original EE return without changing control flow

## Verified progression
The CP37 native Xvfb run compiled and executed the genuine original ISO. The iterative AOT registration repaired three authentic original indirect-call targets 0x003364F8, 0x00334230 and 0x003329D0. This progressed through substantially more startup and IOP module initialization code. No genuine GS framebuffer appeared.

The new blocker is a malformed guest EE PC 0x00000F82 (not aligned to a MIPS instruction, outside original executable code), with RA=0xF82. The scheduler makes the main thread Dormant because the target has no AOT function. This must not be 'repaired' by registering a fake instruction or skipping the transfer.

## Read-only instrumentation
The EE scheduler now logs `[EE:invalid-return]` immediately after a guest AOT invocation produces a nonzero, non-word-aligned PC. It includes the **last dispatched real guest entry**, resulting invalid PC, current RA, SP, four adjacent guest stack words and EE cycle. This is used to locate the real branch/return provenance and determine whether guest register restoration, stack damage, DMA writes, or a recompiled control-flow bug is responsible.

The source inspection of the final original guest call sequence is preserved in `built/reports/guest_invalid_0xf82_aot_inspection.txt`; a routine at 0x002BE01C executes `jr $ra` after loading the saved return address from the guest stack, but that is not proof yet that this exact instruction produced 0xF82.

No generated guest instruction patches, synthetic success codes, fake GS frames or game-specific runtime shortcuts were added. A native recompile and real original-ISO regression run are required to determine the precise cause.
