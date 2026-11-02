# CP46 — Fast real-ISO replay automatically after native AOT link

The existing prebuilt native smoke workflow was manually triggered and did not set the real-disc environment `PS2X_CD_IMAGE`, so genuine CD/DVD behavior in the smoke could differ from a full native link run. It also did not preserve newly added original EE return-stack diagnostics.

Changes:

- Set `PS2X_CD_IMAGE` to the same reconstructed, real original ISO path used by the full native test (no synthesized disc data).
- Enable opt-in `PS2X_GUEST_WATCH_ADDR=0x1ffff10`, derived from CP41's observed original EE return stack, when replaying a built executable with the CP43 instrumentation.
- Include bounded first-occurrence `EE:stack-write`, `EE:stack-dispatch-delta`, `EE:invalid-return` and `EE:main-dormant-cause` diagnostics in the existing prebuilt smoke report.
- Automatically invoke this **without any AOT recompilation** after each successful `GodHand - Link AOT and test first frame` Action completes. Manual dispatch remains supported.
- Retain the existing strict test that fails when no nonuniform original guest GS frame exists; test failure must never be mistaken for an artificial frame.

The next native checkpoint therefore has a fast follow-up authentic ISO boot/GS verification run. A genuine rendered frame still has not been demonstrated. 