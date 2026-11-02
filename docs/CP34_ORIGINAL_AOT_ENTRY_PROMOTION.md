# CP34 — Restore real omitted native AOT call entries from verified guest execution

## Verified root cause
CP33 original-ISO native runtime reproduced the precise source of the post-IMS hang:
- Guest indirect call at 0x00330394 (JALR) targets 0x003364F8, returning to 0x0033039C.
- Runtime report: guest-branch:missing-target, target=0x3364f8, codeRegion=yes.
- EE scheduler then reports missing_recompiled_function pc=0x3364f8, and correctly marks the main EE thread Dormant.
- The original 0x3364f8 instruction is **already translated** in generated sub_00336498_0x336498.cpp but not exposed as an exact entry in register_functions.cpp.

## Implementation
tools/promote_aot_entry.py scans real missing-target diagnostics, locates the existing AOT source owner by its original guest address range, verifies that the target is an aligned decoded MIPS instruction (not an unverified address or branch delay slot), and inserts a resume label plus switch case in that original generated translation. It registers the exact guest address in the existing native function pointer table.

This is an AOT link-time entry-point restoration using already translated original instructions, not an instruction patch, artificial success return, HLE shortcut, or game-specific emulation branch. An input address is derived from observed guest execution, not a hardcoded game filename or opcode sequence.

tools/tests/test_promote_aot_entry.py includes four isolated fixture tests covering exact entry registration, repeated-run idempotency, rejection of nonexistent instruction boundaries, delay-slot rejection, and no-op when no missing targets are observed. The native link workflow checks these tests and applies the promotion to fresh AOT source extraction before compiler batch planning. It publishes built/reports/aot_promoted_entries.json.

## Verification boundary
Compilation success and exact AOT entry restoration do not establish a visual game screen. Run the native executable with the genuine mounted ISO under Xvfb and require a real nonuniform GS framebuffer; investigate newly observed missing targets by the same reproducible workflow.
