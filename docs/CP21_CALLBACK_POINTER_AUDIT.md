# CP21 — Function-pointer tables inside executable PS2 ELF regions

The game's live indirect JALR calls repeatedly land at executable locations that were omitted from the coarse inferred AOT function map. The first few target addresses were successfully recovered as genuine guest code through original-ELF-backed metadata, but this one-at-a-time recovery is not scalable.

The upstream fallback analyzer can discover code pointers in data-only sections, but retail PS2 ELF load segments may also carry pointer arrays alongside executable code. A pointer table with several consecutive genuine MIPS function addresses may therefore be missed by a section-class-only scan.

## Nonmutating audit

`tools/scan_guest_pointer_tables.py` inspects the original ELF32 little-endian MIPS PT_LOAD segments, including executable segments, and reports clusters of 3 or more contiguous file-backed executable pointers. It labels rough first-instruction shapes (stack prologue, saved RA, JR RA, LUI preamble, direct tail jump, other), identifies whether source pointers were stored in an executable segment, and preserves every original byte. False positives remain possible; a cluster is a *research candidate*, not proof of a callable function. No addresses from this report are automatically injected into AOT.

Unit tests use synthetic ELF files covering executable/data source tables, nonpointers, misaligned values, insufficient clusters, and invalid ELF headers (6 passing tests).

The short `.github/workflows/godhand-prebuilt-smoke.yml` job extracts the actual boot ELF from LFS chunks, runs the audit **without recompiling AOT**, and publishes `built/reports/code_pointer_audit.json` to `main` for review. The existing linked native game executable remains unchanged. Analyze the report before considering a generic importer or improved ELF parser boundary recovery.

## Real graphical goal

Continue authentic EE guest execution and IOP/GIF/VIF/GS processing until `built/reports/link_ci_frame.txt` demonstrates guest GS writes and `built/frames/first_real_gs_frame.ppm` contains a genuine nonuniform guest frame. Do not substitute host placeholder windows, fabricated images, or game-specific function stubs. Verify the CP20 native link and boot results separately.
