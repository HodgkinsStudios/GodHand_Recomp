# CP28 — Load original IOP IRX from the mounted PS2 ISO

## Evidence
CP26 native build linked successfully and executed the original guest under Xvfb for 90 seconds. The report repeatedly showed a failed CDROM module open: MODULES/SDRDRV.IRX;1. No nonuniform GS framebuffer was captured (exit code 124, timeout).

## System-level fix
IopModuleLoader::readWholeHostFile tries the extracted/host filesystem path first, then, only for CDROM device paths, resolves the actual ISO9660 entry using the shared physical-ISO parser and reads its exact bytes using IopHost file access. It validates the physical sector extent, bounds and size. Missing, truncated and oversized files are rejected. It does not synthesize IRX contents or report loading success on missing data.

The existing physical IOP emulator still validates and loads the real MIPS IRX image and executes its original entry point. Repeated missing-file error logs are bounded per module path, without changing retry or return semantics.

Regression tests cover byte-for-byte ISO file access, absent assets, truncated sector extents and non-CDROM fallbacks. Native first-frame CI now stores last sampled guest ticks and IOP module events separately from the raw log tail.

## Validation boundary
Standalone IOP tests and full AOT native execution are required. Until a real frame capture exists, no visual game screen is claimed.
