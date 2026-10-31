# CP14 — Restartable hosted native AOT build

## Why the previous Actions run was slow

The generated God Hand executable consists of approximately 9,231 separate C++ translation units. A fresh GitHub runner starts without any compiled objects, so canceling it discards all work. The previous workflow also redirected Ninja's entire build log to a file instead of showing progress while compiling.

## Changes

- **Four deterministic, lossless batches** selected from CMake's own `compile_commands.json`. Each generated `*.cpp` belongs to exactly one batch; no functions are omitted, changed or replaced.
- **GitHub Actions ccache persistence** via `actions/cache/restore@v4` and **four checkpoints** (`actions/cache/save@v4`) after batches. A new run restores the latest available cache with the same generated archive/compiler prefix. Ccache verifies input/compiler/flags before using objects. The archive LFS pointer content hash scopes the cache.
- **No automatic cancellation:** `cancel-in-progress: false` queues newer source changes rather than silently throwing away a running compiler job. Manual cancellation remains possible; completed cache snapshots should remain available.
- **Live Ninja output** in each compile batch, plus full downloadable compiler logs as a GitHub Actions artifact, and a concise report in `built/reports/link_ci_batches.txt` on main.
- **Final native link unchanged:** all generated object files and host runtime objects still must compile and link. Nothing is marked playable simply because a CMake or compiler test passes.
- **Guest frame test unchanged:** Xvfb frame capture only runs when the real native executable is present. A real GS frame has not yet been verified.

## Validation

Seven focused tests pass, including a synthetic five-object CMake/Ninja project built in multiple batches. All 13 embedded shell scripts in the workflow passed Bash syntax parsing. The complete God Hand AOT build itself remains subject to the new GitHub-hosted run; CMake/Ninja may expose real generated-code errors, not merely performance issues.

## Where to check

Workflow: `.github/workflows/godhand-link.yml` — "GodHand - Link AOT and test first frame".

Progress: four `Compile generated C++ batch N of 4` stages, each followed by `Save compiled C++ cache after batch N`.

Diagnostics: `built/reports/link_ci_batches.txt`, `built/reports/link_ci_status.txt`, `built/reports/link_ci_frame.txt`. Raw logs remain available in the short-lived Actions artifact `godhand-compiler-logs-<run id>`.

Successful game binary (only after complete link): `built/linux-x86_64/GodHand_Recomp_Game`, alongside `GodHand_Recomp_Launcher`.

## Limitations

The initial run still has to compile thousands of units, so first-run speed will not instantly improve by the cache alone. Cache storage/upload and the compiler's most complex generated functions may still create bottlenecks. A canceled run saves only the completed batch caches, not the currently compiling batch. GitHub cache quotas/evictions can remove older entries. This process improves restartability, reporting and future compilation reuse; it does not resolve missing PS2 opcodes, incorrect function boundaries or rendering defects.
