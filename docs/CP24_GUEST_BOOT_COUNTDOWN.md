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

## CP24 follow-up — CDVDFSV SearchFile binding evidence

The original generated function at `0x00398890` calls `sceSifInitRpc` and later `sceSifBindRpc` for SID `0x80000597`, PS2's **CDVDFSV SearchFile** RPC service (see PS2SDK `ee/rpc/cdvd/src/libcdvd.c` and the PS2tek IOP RPC server list). The original guest global at `0x003F868C` is loaded before the delay/retry loop and is written with zero at `0x003989C0` in a branch delay slot. A raw `0xFFFFFFFF` at this address was also discovered in the generated source; that memory lies in an executable ELF load segment but appears to be initialized data, not an actual EE function. This may explain why decoding code and data indiscriminately creates spurious AOT functions.

The current IOP routing source `ps2xIOP/src/iop_subsystem.cpp` registers only `MCSERV`, `DBCMAN`, and `LIBSD` built-in services; `IopModuleManager` knows `cdvdfsv` as a module key, but does not itself create a CDVD server. `canBindRpc()` additionally asks whether the physical IOP emulator registered the SID. Thus a physically registered CDVDFSV server could still work, but merely claiming an HLE module loaded is **not sufficient** to make SID `0x80000597` bindable. Diagnose the actual IOP module and RPC server state before making any behavioral change. A correct fix must implement genuine CDVD/IOP RPC semantics or execute the real physical IRX where available; do not create a dummy successful server or fabricate file results.

CI reference artifact: `built/reports/guest_countdown_global_references.txt`; generated code inspection: `built/reports/guest_stall_aot_inspection.txt`. The new CP24 native-link job records `$v0/$v1` at scheduler checkpoints once compiled.
