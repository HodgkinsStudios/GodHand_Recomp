# CP43 — Diagnose the malformed EE return to 0x00000F82

The CP41 real original-ISO run confirmed the exact sequence:

- Original guest dispatcher resumed function owner at 0x002BE014.
- Its original MIPS epilogue loads `$ra` from 0($sp), executes `jr $ra`, and increments $sp by 0x20.
- It returned to 0x00000F82 (unaligned MIPS PC, outside valid executable code). The EE scheduler correctly refused to execute it.
- At the resulting SP 0x01FFFF30, the stack slot the original epilogue read was 0x01FFFF10.
- The original game still did not produce a nonuniform guest GS framebuffer.

The generic runtime write trace `ps2TraceGuestWrite` now supports an **opt-in read-only watchpoint** set through `PS2X_GUEST_WATCH_ADDR`. It compares the actual physical EE RAM address (including kseg aliases), logs a bounded number of intersecting 1-, 2-, 4-, 8- and 16-byte writes, and includes original guest PC, RA, SP, old memory contents and the value about to be written. The default is disabled. It does not change register values, original instructions, RAM contents, syscall return values, or graphics.

The native Xvfb regression workflow enables `PS2X_GUEST_WATCH_ADDR=0x1ffff10`, a location derived from the previous measured runtime stack; it preserves the write provenance alongside `[EE:invalid-return]`. The next goal is to establish whether the original caller saved an already-invalid return value, or whether a later write corrupted the saved address. This will distinguish generated AOT call/return semantics from unrelated guest memory corruption. On unsupported targets, the watchpoint does not invent a fix.

The C++ test build and genuine original-ISO boot are required before claiming any success in resolving graphics.
