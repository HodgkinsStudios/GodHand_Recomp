# CP35 — Cumulative promotion of authentic indirect-call AOT entry points

## Verified prior progress
The CP34 original-ISO native execution fixed the missing 0x003364F8 entry by connecting its existing translated original MIPS code to the exact native AOT dispatch table slot. The next authentic indirect call from the same EE caller at 0x00330394 targets 0x00334230, which has codeRegion=yes but lacks an AOT dispatch registration. The EE scheduler again marks the main thread Dormant; nonuniform_guest_gs_frame=no.

## Correction
The CP34 tool originally read only the most recent missing-target diagnostic, whereas every CI execution unpacks the untouched original generated source archive. CP35 adds --previous-report so each AOT build promotes the union of all earlier verified entries and the latest true runtime missing target. The original instruction boundary and owning function are independently validated in each run.

A regression test checks that two distinct authentic instruction addresses in a synthetic original-code owner are both registered, and that no prior entry is lost. This general repair does not change the MIPS operations or guest control-flow semantics.

## Verification
Inspect built/reports/aot_promoted_entries.json to confirm both 0x3364f8 and 0x334230 are present as promoted entries. Re-run native ISO boot; require nonuniform real GS framebuffer for a visual-screen claim.
