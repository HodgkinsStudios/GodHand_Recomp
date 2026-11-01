# CP24 — Why boot pauses before graphics

## Proven existing binary and guest progress

Full AOT game executable is successfully linked and published under `built/linux-x86_64/GodHand_Recomp_Game` via Git LFS. All 9,362 generated guest translation units are included. The original guest boot reaches actual DMA and GIF transfer traffic (16,384 each), with GS register writes for BITBLTBUF/TRXPOS/TRXREG/TRXDIR. **No real framebuffer has been captured**. GS transfer registers do not by themselves prove guest triangles or an image.

## First confirmed long-running guest countdown

The generated routine `sub_00398890_0x398890.cpp`, preserved verbatim in the `built/aot/generated_cpp.tar.gz` archive, shows a busy loop at original PS2 address `0x00398930`:

- At `0x0039890C`: load `$v0` from original guest RAM address `0x003F868C`.
- At `0x00398910`: signed `bgez $v0` bypasses the loop if nonnegative.
- At `0x00398928`: set `$v1=-1`.
- At `0x00398930`: `addiu $v0,$v0,-1`.
- At `0x00398944`: `bne $v0,$v1,0x00398930` repeats.
- Following this, the original code calls `sceSifBindRpc` (from `0x0039895C`). Prior call is `sceSifInitRpc` at `0x00398904`.

This is **original guest code**, not a missing AOT target. It could be a long intentional delay, a wrong guest global initial value, or a host scheduling/timer problem; no conclusion until live register snapshots.

## CP24 debug-only change

At each scheduler checkpoint where a guest context is available, sample `$v0` and `$v1` into two thread-safe host atomics and include them in the existing `[run:tick]` log. The sampling does not alter the guest context, program counter, branch decision, GS, IOP, timing, or registers. A native build is triggered only via an explicit workflow-file edit.

The short prebuilt-only smoke and AOT-source inspection workflows are retained. A 20-second boot probe may not reach the loop on a slower hosted runner; the native-link CI uses a longer execution window.

## Next exact test

Review the new `built/reports/link_ci_frame.txt`, inspect the two register values, estimate the true remaining countdown iterations, and then determine whether the loaded global is expected by the original program or should be updated by a correctly implemented PS2 subsystem. Do NOT skip the loop, inject fake guest results, or advertise a rendered frame without nonuniform guest GS capture.
