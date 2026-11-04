# CP48 — EE interrupt handlers now nest on the interrupted guest stack

**Result: the measured cause of the unaligned `0x00000F82` return is fixed and now verified on a real native build and disc boot (see CP49). No GS frame exists yet; boot stops later, in a guest status-poll loop.**

## Measured evidence chain

1. CP45 showed the original caller of `0x002BDFF8` is `jal` at `0x0030B384`, so the legitimate saved return address is **`0x0030B38C`**.
2. CP43's opt-in watchpoint at `0x01FFFF10` recorded `old=0x000000000030B38C` followed by `valueLo=0x0000000000000782`, then `0xF82`. The store comes from `pc=0x003B6C44`, which the original MIPS disassembly shows as `sd $s0,($sp)` inside `0x003B6C28` (`<sp>=0x01FFFF10`). `$s0` there holds a DMA byte count, not a return address.
3. `[EE:stack-dispatch-delta] dispatched=0x3B6E78 ... sp=0x1FFFFF0` places the top of that nested chain at `0x01FFFFF0`.
4. `0x01FFFFF0` is exactly what `EeScheduler::invocationStackTop()` returns for the first reserved invocation stack (`PS2_RAM_SIZE - 0x10`), and it is also exactly the EE main thread's initial `$sp` (`PS2Runtime` sets `m_cpuContext.r[29] = PS2_RAM_SIZE - 0x10`).

So interrupt handlers were started at the top of RDRAM, immediately above the live main-thread frame at `0x01FFFF10`. A handler deeper than `0xE0` bytes necessarily overwrote that frame's saved `$ra`, and the resume at `0x002BE014` then executed `ld $ra,($sp)` on clobbered data.

## Change

Real EE exception handling pushes handler frames *below* the interrupted `$sp`; there is no separate hardware interrupt stack at the top of RAM.

- `EeScheduler::nestedInvocationSp()` returns the interrupted guest `$sp` when it is a plausible guest stack pointer, otherwise `0` so existing callers keep their reserved-stack fallback.
- `EeScheduler::liveGuestContext()` returns that context only for a thread that is actually `Running`; a blocked thread has a stack image, not a live frame.
- `EeScheduler::dispatchIrq()` seeds Interrupt invocations with the nested `$sp`.
- Host-driven invocations are deliberately unchanged: GS VSync callbacks and pending async callbacks still use the reserved async stack pool, as `sceGsSyncVCallback runs as a scheduler invocation on its callback stack` and `pending async callbacks execute sequentially on a reusable invocation stack` require. An earlier draft also moved those paths and was reverted after the test suite caught it.
- The stack pointer captured at `sceKernelRegisterIntrHandler` time is still deliberately ignored: it is normally a live caller frame, and the existing `"IRQ callbacks use an isolated invocation stack"` test requires that.
- `reserveAsyncCallbackStack()` is unchanged and still serves invocations with no live guest stack.

No guest instruction, generated AOT translation, syscall return value, GS register or frame is modified.

## Tests

- New: `IRQ handler nests below the interrupted EE stack pointer` — reproduces the real layout (live frame at `0x01FFFF10`, handler that stores below `$sp`) and asserts the handler starts at the interrupted `$sp` and that the saved `0x0030B38C` survives.
- Existing interrupt tests keep their expectations: handlers are still not seeded from the registration-time `sp`, and 96 queued callbacks still reuse one stack depth.

## Verification

Both changed files compile, and the full `ps2xTest` suite now passes **447/447**, including the new nesting test and the callback-stack isolation tests. On a native build of all 9,362 generated guest TUs with the real disc, the `0x00000F82` return corruption and the dormant main thread are gone, and boot proceeds to nine IOP module loads and `sceGsSyncVCallback` registration (CP49). The remaining stall is a guest status-poll loop, not this defect.
