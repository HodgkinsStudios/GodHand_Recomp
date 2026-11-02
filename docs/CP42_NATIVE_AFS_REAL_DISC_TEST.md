# CP42 — Native C++ extractor real-disc verification

The GTK launcher now uses the standalone C++17 CRI AFS installer for first-run extraction, subsequent readiness checks, and resumable repairs. Synthetic ISO/AFS fixture tests have passed in the launcher CI, and the earlier Python archive inspector had independently extracted original ISO game assets.

This checkpoint adds a real original-disc end-to-end test to the existing AFS inspection GitHub Actions workflow. It extracts exactly the original ISO9660 extent of the real `_DATA.AFS` into an isolated runner, builds the same C++ installer library and command-line harness used by the launcher, and checks:

- A missing initial extraction is reported incomplete.
- All original **1,445** top-level files are extracted from the real CRI AFS data without fabricating content.
- `--verify-afs` detects a complete archive through its completion marker and every extracted entry size.
- Deleting the first extracted asset causes verification to fail.
- `--ensure-afs` restores the missing asset and verification passes again.

Only diagnostic metadata is committed to `built/reports/afs_analysis_summary.txt`, never original retail assets or the copied ISO. The test reads the original ISO supplied to the private runner via repository LFS, not any user's system files.

This extraction feature is independent from the current EE program counter / GS framebuffer problem. No first rendered frame is claimed without an actual guest GS capture.
