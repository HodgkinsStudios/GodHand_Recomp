# CP37 — Continue through genuine native AOT indirect calls without rebuilding the entire game each pass

The CP35 full native original ISO first-frame test restored two previously missing guest AOT callbacks 0x3364F8 and 0x334230, then reached 0x3329D0 through the same genuine indirect-call source 0x330394. The EE main thread went Dormant due to the next absent registered original MIPS entry; nonuniform_guest_gs_frame=no.

The full native compile already provides a runnable binary and all original guest instruction translations. The updated native link Action now iterates up to **four additional times** inside one runner:
1. Examine the exact genuine `[guest-branch:missing-target]` diagnostic in the last native boot.
2. Use the generic strict `tools/promote_aot_entry.py` to validate that its address is a decoded original MIPS instruction, not a delay slot or fabricated target, and restore its exact AOT owner/dispatch registration.
3. Merge newly registered entries with the earlier exact entries for reproducible future builds.
4. Perform an **incremental C++ relink** of the changed translation units, start the real game ISO under Xvfb again, and inspect its actual GS capture. Stop if a genuine frame is captured or no valid next missing AOT entry exists.

No game-specific simulation, skip or fake rendering is introduced. This avoids the previous expensive repeated 9,362-unit recompile for each single missing indirect-call target. Reports contain the resulting cumulative promoted addresses and the final authentic boot diagnostics.

As of this document's creation, the new iterative job has not yet completed, so rendered gameplay remains unverified.
