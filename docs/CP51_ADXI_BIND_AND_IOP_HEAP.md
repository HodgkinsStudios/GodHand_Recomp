# CP51 - ADXI bind semantics, and the IOP heap blocker behind it

Status: **no GS frame yet.** The CRI ADXI stall from CP50 is now fully explained, a
diagnostic switch proves what comes next, and the next genuine blocker is identified with
numbers. Default runtime behaviour is unchanged and the suite is green (447/447).

## 1. Why `0x003758B8` spins

`sceSifBindRpc` is an EE kernel syscall in this game (the guest stub at `0x003B2F48` is
`li v1, ...; syscall`), so the recompiled call lands in `ps2_syscalls::SifBindRpc`.

The CRI ADXI init at `0x00375860` does:

```
0x00375870  s0 = 0x76A880                       ; ADXI context
0x00375878  a0 = s0 + 0x0DC                     ; client data  = 0x76A95C
0x0037587C  a1 = 0x19720317                     ; ADXI SID
0x00375880  jal sceSifBindRpc
0x00375888  bgezl v0, 0x003758A8                ; negative return => trap at 0x00375890
0x0037588C  lw a0, 0x100(s0)                    ; delay slot: a0 = [0x76A980]
0x003758A8  ... 0x270F-iteration delay ...
0x003758D0  beqz a0, 0x00375878                 ; still null => rebind
```

`0x76A980` is not a separate flag: it is `client->server` at **client + 0x24**, which is
exactly `server` in the SDK layout (`t_SifRpcHeader` 0x00-0x0F, `command` 0x10, `buf` 0x14,
`cbuf` 0x18, `end_function` 0x1C, `end_param` 0x20, `server` 0x24). The runtime's
`t_SifRpcClientData` in `ps2xRuntime/src/lib/Kernel/Syscalls/Helpers/State.h` agrees, which
is why the host address is `0x76A95C + 0x24 = 0x76A980`.

Protocol, checked against ps2sdk instead of guessed:

- `ee/kernel/src/sifrpc.c` - `sceSifBindRpc` clears `cd->server`, sends one
  `SIF_CMD_RPC_BIND`, and returns **0** even when no server exists.
- `iop/system/sifcmd/src/sifrpc.c` - `_request_bind` **always** replies; on a miss it echoes
  `sd = NULL`, `buf = NULL`. There is no "server not found" error path and no EE-side timeout.
- Callers are expected to test `cd->server` and retry - which is what this loop does, forever.

So the loop can only end if the IOP reports a registered server for `0x19720317`, and the
module that registers it (`cri_adxi.irx`) is loaded *after* the loop at `0x002C9E38`, in the
same straight-line function (`0x002C9D58`: LIBSD, SDRDRV, SDHD, SNDDRV, then
`jal 0x00375318` = ADXI init, then `sceSifLoadModule("cdrom0:\\MODULES\\CRI_ADXI.IRX;1",
"sdinit=0")`).

Measured state at the stall (default build):

```
modules loaded      : 6  (sio2man, padman, libsd, sdrdrv, sdhd, snddrv)
stall pc            : 0x003758B8   ra = 0x00375888   activeThreads = 1
[BindRpc] no server for sid=0x19720317 client=0x76a95c ra=0x375888
```

The `[BindRpc] no server` line is new and permanent: a silently refused bind used to cost a
whole boot with no trace in the log.

## 2. Diagnostic switch: `PS2X_BIND_ANY`

`RPC.cpp` keeps the strict, tested behaviour by default (`cd->server` stays null until the
emulated IRX registers the SID - see the `emulated RPC bind waits for a registered IOP
server` test). `PS2X_BIND_ANY=1` publishes a client-side descriptor for every SID instead,
purely to measure what the boot does next. It is a diagnostic, not a fix, and it is not a
default path.

Effect, same disc, same build:

| mode | modules | stall |
| --- | --- | --- |
| default | 6 | `0x003758B8` (ADXI bind loop) |
| `PS2X_BIND_ANY=1` | 7 (adds `cri_adxi.irx`) | `0x0033B2B0` (new fatal) |

With the switch the ADXI init returns, `sceSdRemoteInit` runs, `cri_adxi.irx` loads, and the
IOP prints its banner (`CRI ADX Driver Ver.9.67`, thread priorities, SPU core count).

## 3. The next blocker: IOP heap exhaustion

`0x0033B2B0` is a fatal-error spin, not a wait loop:

```
0x0033B290  jal sceSifAllocIopHeap(0x8D0)
0x0033B298  bnez v0, 0x0033B2D0          ; success path
0x0033B2A0  jal 0x003260C8               ; print
0x0033B2A4  a0 = 0x455DC8                ; "E0100301: SJX_Init can't allocate IOP Heap\n"
0x0033B2C4  b 0x0033B2B0                 ; halt
```

The allocation really is refused: the IOP heap (`IopMemory::HeapBase = 0x00120000`,
`HeapLimit = 0x001F0000`, 851 KB) is already full before audio init.

Where it goes: the guest has its own archive loader (the module-loading wrapper around
`0x002BE240`..`0x002BE450`) which, per attempt, does

```
sceSifAllocIopHeap(entrySize)        ; 0x003B5968
sceSifSetDma(staging, iopAddr, size) ; 0x003B2148   staging buffer = 0x11C0180 (IRX.IMS copy)
sceSifDmaStat(trid) until >= 0       ; syscall 0x76, now returns 0 for issued ids
0x003B5E50(...)                      ; cache writeback helper
```

and never frees the staging block on the retry path. A 25-35 s boot produces **163
allocations of `0x1480`** (5248 B = `sio2man.irx`, always copied from the same staging
address), filling the heap: `[IOP mem] alloc fail size=0x8d0 candidate=0x1ef900 limit=0x1f0000
blocks=162`.

Note this leak is present in the *default* build too (the 8 `sceSifAllocIopHeap` failures at
boot happen before ADXI), so it is an independent defect that will have to be fixed either
way. `sceSifDmaStat` is not the cause: every poll in the log reported `issued=1`, i.e. the
DMA wait terminates as CP50 intended.

The remaining question for that loop is why the guest retries the same `sio2man` load 163
times - i.e. what its post-DMA load step (`0x003B5E50` -> `0x003B5C48`) reports back.

## 4. IOP-side gap found while chasing this

The IOP emulator executes real IRX code (it prints driver banners), yet in a full boot:

- **zero** `sceSifRegisterRpc` calls (`iop_rpc.cpp` case 17 never taken),
- **zero** `CreateThread` calls (`iop_kernel.cpp` case 4 never taken),

so `IopRpcBridge::m_servers` stays empty, `hasRpcServer()` is always false, and
`IopSubsystem::canBindRpc()` can only be satisfied by an HLE route. That is why the ADXI SID
never becomes bindable, and why the EE's `sid=0x19720317 rpc=0x2` request is traced as
unhandled even after `cri_adxi.irx` is loaded: the driver's init stops right after printing
its banner, before it registers its server.

This is the real target: IRX module init never reaches its RPC registration and never spawns
its worker threads.

## 5. Environment note (runner was reset twice)

The build tree survived, but `/tmp` did not, so the toolchain had to be rebuilt without root:

```
python3 -m pip install --target /tmp/tooling cmake ninja capstone
apt-get -o Dir::State::lists=/tmp/apt/lists -o Dir::Cache=/tmp/apt/cache \
        -o Dir::State::status=/tmp/apt/status -o APT::Sandbox::User=$USER update
apt-get ... download x11proto-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev \
        libxi-dev libxext-dev libxrender-dev libxfixes-dev libxxf86vm-dev libgl-dev \
        libglvnd-dev libxkbcommon-dev xvfb xserver-common libxfont2 libfontenc1 \
        libpixman-1-0 libunwind8 libfreetype6 libpng16-16t64 libbrotli1 libxkbfile1 \
        x11-xkb-utils xkb-data
for f in *.deb; do dpkg -x "$f" /tmp/sysroot; done
ln -sf /usr/lib/x86_64-linux-gnu/libGL.so.1.7.0 /tmp/sysroot/usr/lib/x86_64-linux-gnu/libGL.so
```

`Xvfb` wants `/usr/bin/xkbcomp`, so it runs inside a user namespace with an overlay over
`/usr/bin` (`unshare -rm`, overlayfs upperdir in `/tmp/xkb`). The AOT C++ archive
(`built/aot/generated_cpp.tar.gz`) was restored into `/tmp/aot` with mtimes older than the
existing objects, so all 9362 translation units stayed cached and only a 20 s relink was
needed. `tools/godhand_iso_pipeline.py --extract-elf` rebuilds `/tmp/game/boot.elf` from the
ISO parts in `iso/`.

Run command used throughout:

```
DISPLAY=:99 PS2X_CD_IMAGE=/tmp/GodHand.iso GODHAND_GS_FRAME_PPM=/tmp/frame.ppm \
  timeout --kill-after=3s 35s build-game/ps2xRuntime/ps2EntryRunner /tmp/game/boot.elf
```

## 6. Next steps

1. Fix the 163x module-load retry / IOP heap leak: find what the guest checks after
   `0x003B5E50` and make that step succeed (or free the staging block).
2. Make emulated IRX init reach `sceSifRegisterRpc` + `CreateThread`: trace `cri_adxi.irx`
   init instruction-by-instruction after its banner print.
3. With (2) done, `canBindRpc(0x19720317)` becomes true on its own and `PS2X_BIND_ANY` is no
   longer needed to reach the audio stack.
4. Only then chase the GS: no `GODHAND_GS_FRAME_PPM` frame has been produced yet.
