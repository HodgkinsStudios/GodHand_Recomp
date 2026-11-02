# CP33 — Trace why the EE main thread becomes Dormant

CP32 showed that the original God Hand EE thread (id 1) is Dormant (status 5), with no pending wait condition, even though the EE clock and VBlank event scheduler continue to advance. The saved PC is 0x003364F8, with RA 0x0033039C; the AOT source inspection shows 0x3364F8 is an indirect-call target in original code.

This change records the actual exit cause before state is cleared: main-thread PC zero, missing registered AOT function, explicit ExitThread, or termination by another thread. It also logs the original PC, RA, SP and EE cycle. The diagnostic is read-only; no guest instructions, registers, or thread semantics are altered. The full native AOT workflow saves the diagnostic even if it appears near the beginning of the trace.

Next criterion: inspect the CP33 run to determine exactly how the main thread became Dormant, then fix the generic underlying AOT dispatch / EE scheduler issue and require a genuine nonuniform GS framebuffer before claiming a visible game screen.
