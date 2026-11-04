# CP52 — The 163× IOP-heap leak: why every module load retries until the heap is exhausted

Status: no GS frame yet (`GODHAND_GS_FRAME_PPM` still never written). This checkpoint does
not change runtime behaviour — it pins down, with syscall+MMIO traces that were added and
then fully removed, the mechanism behind the IOP heap exhaustion that killed `SJX_Init`
(`E0100301`) in the `PS2X_BIND_ANY` run, and what has to be implemented next.

## 1. The leak mechanism, traced end to end

Temporary env-gated traces used for this analysis (all removed afterwards, none of them in
the tree): `PS2X_SYSCALL_TRACE` (every EE syscall with id/pc/args), `PS2X_IOPHEAP_TRACE`
(every IOP-heap alloc/free/fail), `PS2X_MMIO_TRACE` (DMAC/SIF window writes). Baseline boot
in this run: 6 modules loaded, stall `pc=0x3758B8` on the ADXI bind, 162× `sceSifSetDma`
and 163× `sceSifAllocIopHeap` from the guest, all leaked.

The boot loader `0x002C9D58` calls the game's linked `libk` SifLoadModule wrapper
`0x002BE240` per module, each followed by a boot-side retry loop:

```
2c9db8  jal 0x2be240            ; load "cdrom0:\MODULES\sio2man.irx"
2c9dc0  bltz $2, 0x2c9db0       ; retry while result < 0
```

The same `bltz` retry pattern exists for padman, libsd, sdrdrv, snddrv (and the pair
mcman/mcserv in `0x002AE3F0`). One wrapper call does:

1. lowercase the path, scan the in-memory IRX.IMS archive (`0x2BE2D4` entry table,
   9 entries), name-match the entry (`0x2BE38C` memcmp);
2. `sceSifAllocIopHeap(entry.size)` at `0x2BE3A4` — this is the 0x1480-sized allocation;
3. `sceSifSetDma` at `0x2BE3CC` — one descriptor, `src=0x11C0180 dst=<alloc> size=0x1480`
   (162 such descriptors logged, all identical except the growing IOP destination);
4. poll `sceSifDmaStat` at `0x2BE3D8` — our emulation completes immediately, fine;
5. run the in-lib SIF send wrapper `0x2BE3F4 → 0x3B5E50 → 0x3B5C48` — this is where every
   attempt fails (see §2);
6. return the wrapper's value; the boot retry loop repeats until the wrapper returns ≥ 0.

The wrapper's own alloc-failure path `0x2BE3B0 beqz $17 → 0x2BE2B8` (Kprintf) jumps to
`0x2BE420 jal 0x3B60A0` — i.e. **only the alloc-failure fallback calls the
`sceSifLoadModule` syscall**, whose emulation prints `[SIF module] load-emulated id=N`
and loads the module. That is why the log shows exactly this sequence with the heap
exhausted:

```
[IopHeap] alloc FAIL size=0x75c0 ...  →  [SIF module] load-emulated id=2 padman
[IopHeap] alloc FAIL size=0x2400 ...  →  [SIF module] load-emulated id=3 libsd
...
```

So the guest's own bulk-transfer path never succeeds even once in a whole boot; every
successful "load-emulated" happens only because the leaked allocations finally exhaust the
IOP heap (163 × 0x1480 = 0xD0000 = the whole 0x120000–0x1F0000 heap) and the fallback
fires. The leaked blocks are never freed — no `sceSifFreeIopHeap` call occurs at all.

## 2. What actually fails: the SIF cmd send/ack is pure guest code with no IOP

The syscall trace shows per attempt **only** alloc + setDma + dmaStat — no SendCmd, no
BindRpc, no CheckStatRpc. The in-lib SendCmd (`0x3B3128`) and everything around it is pure
guest code operating on the game's own static SIF state (0x78xxxx area) plus raw MMIO:

- `0x3B5A80` first checks a state word and, when a bind is needed, binds **SID
  0x80000006** (`0x3B5AA8 lui $5,0x8000; ori $5,6; jal 0x3B2F48 = sceSifBindRpc`) and then
  delays ~0x100000 iterations before rebinding — the classic "wait for `cd->server`
  += 0x24" pattern, identical in shape to the ADXI loop. No bind syscall was ever observed
  in this boot, so the early-exit path is what runs today.
- `0x3B3128` allocates a slot from the guest's cmd queue (`0x3B2AC8`, 64-byte entry ring),
  formats the packet, and kicks the transfer by a raw MMIO DMA-channel write.
- The MMIO trace captured that kick precisely: **16385 writes of `CHCR=0x101` to
  `0x1000A000`, with `MADR=0x7867C0`, `QWC=0x46`** (one 0x460-byte SIF cmd batch per retry
  attempt), plus the paired `MADR`/`QWC` writes. Our runtime's CHCR write path only models
  scratchpad (`0x1000D000/0x1000D400`) and VIF0/VIF1/GIF (`0x10008000/9000/A000`); SIF
  command traffic through that channel has **no consumer**: the packet is never delivered
  to the IOP, no IOP-side handler parses it, and no ack is DMA'd back into EE RAM.
- The wrapper's success gate reads a word out of the guest's cmd-queue state (`lw $2,
  6528($20)` / `lw $3, 4($16)` around `0x3B5E00-0x3B5E24`) — a word only the IOP side can
  update. It never changes, so the wrapper returns negative forever and the boot retries.

Conclusion: the IOP SIF cmd command-dispatch/ack path (EE libk SendCmd ⇒ IOP cmd handler ⇒
IOP→EE ack DMA) is unimplemented. This is the same gap class CP51 recorded for ADXI
(zero `sceSifRegisterRpc`/`CreateThread` in a full boot): the IOP never processes anything
the EE sends it that is not one of the four HLE services (cdvdfsv, mcserv, libsd, dbcman).

## 3. Next steps (in order)

1. Implement EE→IOP SIF cmd delivery: on the CHCR write for the SIF channel the guest
   uses (`0x1000A000` in this build's trace: `MADR`/`QWC` formatted like a SIF cmd
   batch), deliver the EE payload to IOP RAM and notify the IOP subsystem (extend
   `SifTransferKind` with a SendCmd/queue kind, or route through a dedicated bridge).
2. Implement the IOP-side SIF cmd dispatcher as a rom-resident IOP service (same pattern
   as cdvdfsv/mcserv): parse the SIF cmd packet, execute the genuine system semantics —
   for the module-load command, invoke the existing `IopModuleManager` load path (the same
   machinery behind `[SIF module] load-emulated`) and DMA the result word back into the
   EE cmd-queue flag the wrapper polls. Registering SID 0x80000006 on the IOP side also
   unblocks the in-lib bind gate of `0x3B5A80` for a real bind when one is needed.
3. Expected effect: every module load succeeds on its first attempt, the IOP heap keeps
   all 163 blocks free, `cri_adxi.irx` init (CRI ADX banner is already reached under
   `PS2X_BIND_ANY`) can `SJX_Init` allocate and then reach `sceSifRegisterRpc`/`CreateThread`,
   `canBindRpc(0x19720317)` becomes true without `PS2X_BIND_ANY`, and the boot advances
   past the ADXI bind toward game/GS init.
4. Then continue run → stall → fix; verify with a real non-uniform guest GS framebuffer
   via `GODHAND_GS_FRAME_PPM` before claiming any rendering success.

## 4. Reproduction

```
DISPLAY=:99 PS2X_CD_IMAGE=/tmp/GodHand.iso GODHAND_GS_FRAME_PPM=/tmp/frame.ppm \
  timeout --kill-after=3s 40s build-game/ps2xRuntime/ps2EntryRunner /tmp/game/boot.elf
```

Default: 6 modules, `[BindRpc] no server for sid=0x19720317`, stall `0x3758B8`, rc=124,
no frame. `PS2X_BIND_ANY=1`: 7th module `cri_adxi.irx` prints its banner and hits the
IOP-heap exhaustion at `0x0033B2B0`.

## 5. Toolchain restore (rootless, post-reset) — refreshed this session

- `python3 -m pip install --target /tmp/tooling cmake ninja capstone` (needs
  `PYTHONPATH=/tmp/tooling` for the cmake wrapper); capstone 5.0.7.
- Rootless apt into `/tmp/apt`: `x11proto-dev libx11-dev libxau-dev libxcb1-dev
  xtrans-dev libxrandr-dev … libgl-dev libglvnd-dev libxkbcommon-dev` etc. `dpkg -x` into
  `/tmp/sysroot`. The linker needs `CPATH=/tmp/sysroot/usr/include
  LIBRARY_PATH=/tmp/sysroot/usr/lib/x86_64-linux-gnu`.
- Xvfb runtime libs: `libpixman-1-0 libfreetype6 libfontconfig1 libpng16-16t64
  libbrotli1 libdrm2` + `x11-xkb-utils libxkbfile1` (provides `xkbcomp`). Run Xvfb inside
  `unshare -rm` with an overlayfs `/usr/bin` that includes the extracted `xkbcomp`, and
  bind-mount `/tmp/sysroot/usr/share/X11` over `/usr/share/X11`; `LD_LIBRARY_PATH` must
  point at the sysroot libs. Remember `export DISPLAY=:99` for the runner.
- Disassembly: GNU objdump lacks MIPS here, but `binutils-multiarch` extracted to
  `/tmp/sysroot` works:
  `LD_LIBRARY_PATH=/tmp/sysroot/usr/lib/x86_64-linux-gnu /tmp/sysroot/usr/bin/x86_64-linux-gnu-objdump -D -b binary -m mips:6000 -M gpr-names=numeric --adjust-vma=0x100000 …` — wrapped by `/tmp/mdis.py <addr> [count]` against `/tmp/game/boot.elf` (guest 0x100000 → file offset 0x1000).
- `/tmp/game/boot.elf` and `/tmp/GodHand.iso`: rebuilt with `cat iso/GodHand.iso.part-* >
  /tmp/GodHand.iso` then `python3 tools/godhand_iso_pipeline.py --iso /tmp/GodHand.iso
  --out /tmp/game --extract-elf`. AOT restore: untar `built/aot/generated_cpp.tar.gz`
  into `/tmp/aot` and set mtimes older than `build-game/CMakeCache.txt` (cache stays hot:
  ~25 s rebuild). `build-game/` binary relinks with
  `ninja -C build-game ps2EntryRunner`.
