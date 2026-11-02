# CP32 — Diagnose post-IRX startup without guessing guest state

Real-disc CP29/CP30 AOT boot advances past missing SDRDRV.IRX retry and now repeatedly samples PC 0x003364F8 (RA 0x0033039C), with no genuine GS frame. The code at 0x003364F8 is part of an original guest function-pointer target; repeated sampled PC is not sufficient evidence that the function itself is an infinite loop.

This diagnostic pass adds normal debug logs for the existing EE scheduler's immutable snapshot: snapshot sequence, EE cycle count, running EE thread, next event deadline, per-thread saved PC/RA/SP, execution state, wait reason, wait ID and wakeup count. It does not mutate guest execution, fabricate a frame, modify instruction semantics, or skip blocked system calls.

The native AOT Xvfb workflow now includes these records in its bounded first-frame report. With data from multiple samples we can decide if the next issue is a blocked EE thread, scheduler starvation, IOP RPC completion, a vblank wait, or missing game asset dispatch. No on-screen success is claimed without a nonuniform actual guest GS frame.
