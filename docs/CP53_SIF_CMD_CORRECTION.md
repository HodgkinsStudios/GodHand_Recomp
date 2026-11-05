# CP53 — Correcting CP52: the 16k DMA kicks are the loading-screen draw loop, not SIF

Status: **no GS frame yet** (`GODHAND_GS_FRAME_PPM` still never written) and **the SIF cmd fix
has not landed**. This checkpoint records what a fresh instrumented boot actually proved, and
corrects one conclusion of CP52 that would have sent the next implementation down the wrong
path. All instrumentation used here was temporary and is removed again; the tracked tree is at
`37ec348` plus this document.

## 1. What was re-verified from a cold VM

Toolchain, ISO, guest ELF and the AOT cache were all rebuilt from scratch (see CP52 §5); the
9362 recompiled translation units stayed cached, so a rebuild is `ninja -C build-game
ps2EntryRunner` (~55 s). Baseline boot is unchanged: 6 modules load, the ADXI bind at
`0x3758B8` never resolves, `pc` eventually spins, rc=124, no frame.

## 2. Correction to CP52 §2

CP52 interpreted the 16 385 `CHCR=0x101` writes to DMA channel `0x1000A000`
(MADR `0x7867C0`, QWC `0x46`) as the guest's failed SIF command sends. A dump of the
transferred buffer shows that is wrong:

```
+000: 04 80 00 00 00 00 00 10  0e 00 00 00 00 00 00 00
+010: 00 00 01 00 <counter>    50 00 00 00 00 00 00 00
+020: 00 .. 00 00 00 00 51 ..  10 00 00 00 10 00 00 00
+030: 00 .. 52 .. 00 .. 53 ..  40 80 00 00 00 00 00 08
```

- `0x7867C0` is the **EE heap** base (`[SetupHeap] base=0x7867b4 alignedBase=0x7867c0
  size=0xf000`), not a SIF buffer.
- Channel `0x1000A000` is `D2_CHCR` = **GIF** in both real hardware and this runtime's own
  channel table (`kDmaChannelBases`). CHCR `0x101` is `STR|DIR`, MODE=0, i.e. an ordinary GIF
  transfer of 70 qwords.
- The payload is a small GS register write list: the runtime parses it and emits
  `[gs:reg] reg=0x50 value=0x4000000010000`, `reg=0x51`, `reg=0x52 value=0x1000000010`,
  `reg=0x53`, and a per-kick counter at `+0x14` (`0x40000`, `0x40020`, `0x40040`, …) —
  a redrawn loading spinner, not a SIF command batch.
- The transfer **completes correctly**: `readIORegister(0x1000A000)` returns `0x00000001`
  (STR cleared) on the guest's very next poll, so GIF DMA completion is not the blocker.
- The 16 387 `FlushCache` syscalls (`id=0x64`, trampoline `0x3AFF20`) are that same redraw
  loop, not a SIF timeout.

So: the runtime renders the game's early GIF traffic, the DMA model is fine, and CP52's
"unimplemented SIF cmd send" story must **not** be implemented against channel `0x1000A000`.

## 3. What is actually still missing

The module-load retry is real and still costs the whole IOP heap. Its decision point is the
guest's own `libk` wrapper, read end to end this session:

```
0x3B5C48  jal 0x3B5A80          ; prepare/bind the command channel (SID 0x80000006)
0x3B5C80  bltz $2, 0x3B5E28     ; prepare failed -> return error
0x3B5C88  jal 0x3B5B80          ; second prepare step; non-zero -> return 0xFFFEFFFC
0x3B5CA4  $18 = 0x781980        ; per-call command state block
0x3B5CAC  beqz $16, 0x3B5DD8    ; no payload: store 0 at +4
0x3B5CB4  slti $2, $17, 253     ; small payload: memcpy ($0x3A5148), sw size,4($18)
0x3B5DC4  ...                   ; large payload: 32-byte copy loop to 0x781A84
0x3B5DDC  $4 = 0x781B80         ; SifRpcClientData bound to SID 0x80000006
0x3B5DEC  $5 = 6                ; RPC function number 6
0x3B5E04  jal 0x3B3128          ; send + wait (alloc slot 0x3B2AC8, memcpy 0x3B2850,
0x3B5E08                        ;   kick via syscall 0x77 0x3B0060 inside 0x3B2550)
0x3B5E0C  bgezl $2, 0x3B5E20    ; success only when the call returns >= 0
0x3B5E10  lw $3, 4($16)         ; otherwise return 0xFFFEFFFF
0x3B5E20  lw $2, 6528($20)      ; success: result word from 0x781980, store to out ptr
```

Note the kick path uses **`sceSifSetDma` (syscall 0x77)** inside `0x3B2550`
(`0x3B2640` tail: `$19&1` selects `0x3B0070` = `iSifSetDma` / `0x3B0060` = `sceSifSetDma`).
The syscall census for a full 20 s boot is: `0x64` ×16387, `0x76` ×162, `0x74` ×10, `0x5b` ×6,
`0x40` ×6, and **zero `0x77`** — the wrapper never even reaches its kick, so the failure is in
the prepare steps (`0x3B5A80`/`0x3B5B80`) or in the send-path prologue of `0x3B3128`, not in a
missing DMA ack. That is the piece to instrument next, and it is cheap: the two prepare
routines are small and their exit codes are the only inputs to the retry decision.

## 4. Boot state with the existing diagnostics

- Default: 6 modules, `[BindRpc] no server for sid=0x19720317`, stall at `0x3758B8`,
  `GODHAND_GS_FRAME_PPM` never written.
- `PS2X_BIND_ANY=1`: 7th module `cri_adxi.irx` loads, the **emulated IOP executes the real CRI
  ADX driver** (`CRI ADX Driver Ver.9.67`, thread-priority banners from `[IOP printf]`), the EE
  then issues `sid=0x19720317 rpc=0x2` (traced as unhandled, `send=0x76a880/64`), and the EE
  settles into a spin at `pc=0x33B2B0` with `activeThreads=1`. Still no presented frame: the
  frame capture only fires after `gs().copyLatchedHostPresentationFrame()`, which never
  latches because the boot never reaches a display setup.

## 5. Verified state of the tree

- Temporary probes from this session (`PS2X_SYSCALL_TRACE`, `PS2X_MMIO_TRACE`,
  `PS2X_SIFDUMP`, the `0x781900` read probe and the `0x1000A000` I/O read probe) are **removed**;
  `git status` is clean apart from this document and the bookkeeping files.
- `ps2x_tests`: 447 / 447 pass, rebuilt from the clean tree.
- No frame, no patched guest code, no synthetic GS images.

## 6. Next steps

1. Instrument `0x3B5A80` and `0x3B5B80` (entry/exit values) plus the prologue of `0x3B3128`
   to find which check rejects the call; that is the only thing standing between the boot and
   leak-free module loads.
2. With leaks gone the whole `0x120000-0x1F0000` SIF heap stays free, so `cri_adxi`'s
   `SJX_Init` can allocate and reach `sceSifRegisterRpc` + `CreateThread`; then
   `canBindRpc(0x19720317)` becomes true without `PS2X_BIND_ANY` and the boot proceeds to
   display setup.
3. Only after a real non-uniform guest framebuffer appears may rendering be called working.
