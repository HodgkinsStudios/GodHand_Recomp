# CP50 — AOT coverage audit, ROM-resident CDVDFSV, and the CRI ADXI blocker

**Result: boot moved past the ADXF status-poll hang of CP49 (nine modules and no graphics) to a new
blocker inside CRI's ADXI initialisation. No GS frame exists yet.** Four defects were found and fixed
along the way; one is a deliberate, documented behavioural choice that contradicts the older SDK
convention and is explained below.

## 1. AOT coverage audit (independent of the generator)

`build-game/audit_aot.py` disassembles the real `.text` of `SLUS_215.03` with Capstone and compares
every direct call target with `/tmp/aot/register_functions.cpp`.

| measurement | value |
| --- | --- |
| `.text` range | `0x00100000`–`0x003BBF20` (2,866,976 bytes) |
| registered AOT entries | 9,360 |
| instructions in a linear sweep | 716,744 |
| distinct `jal` targets | 6,775 |
| `jal` targets inside `.text` | 6,774 |
| **`jal` targets missing from the AOT** | **0** |
| entries reachable only through `jalr`/pointer tables | 2,586 |
| stack-prologue functions with no registered entry | 1,662 |

Direct-call coverage is complete: every function the guest reaches with `jal` has a translation unit.
The 1,662 unregistered prologues are alternate/indirect entry points; the runtime logs zero
unresolved dispatches (`missing_recompiled_function`, `main-dormant`, `invalid-return`) on the real
disc, so nothing is being silently skipped during this boot. Indirect reachability still depends on
the 978 ELF-validated pointer-table seeds from CP49; no further seeding was needed here.

## 2. CRI ADXF never initialised (root cause of the CP49 hang)

The `0x001FFCD0` loop polls `adxf_GetStat` on a NULL handle. Instrumenting the generated code showed
why: `adxf_Init` calls `adxf_CreateAdxFs`, runs the file-open state machine, and destroys the FS again
when the state machine reports state 4 ("can't open"). The failing string is
`E02110501 adxstmf_stat_exec: can't open`, and the open path issues `sceCdSearchFile`, which binds
CDVDFSV SID `0x80000597`.

Two independent faults made that search fail:

- **CDVDFSV was not routable.** It is an IOP kernel ROM module, so libcdvd binds it without loading an
  IRX; this runtime only activated core services after an IRX load, and there is no BIOS. The fix adds
  `IopService::romResident()` and marks `cdvdfsv` as ROM-resident, so its *existing, genuine* ISO9660
  search implementation (which reads real directory records and never invents a file) is reachable
  from reset. Nothing is fabricated: an absent file still returns "not found".
- **The search packet header is 36 bytes, not 32.** The EE libcdvd packet is
  `[36-byte header][NUL-terminated path][...][output pointer at 288]`. The service read the path at
  offset 32 and therefore always searched for an empty name. The name is now located by skipping the
  zero header bytes, so both observed packets resolve (`\MODULES\IRX.IMS;1`, `\0FLISTDV.DIR;1`).

With both fixed, `adxf_Init` returns success for the first time and the guest leaves the poll loop.

## 3. `sceSifDmaStat` never reported completion

The next stall was `0x002BE3D8..0x002BE3E8`, a genuine guest wait loop:

```
0x002BE3D8  jal  0x003B0040      ; syscall 0x76 (sceSifDmaStat)
0x002BE3E8  bltz $v0, 0x002BE3D8 ; wait while the result is negative
```

The stub returned `-1` unconditionally ("transfers are applied immediately"), so the loop could never
terminate. God Hand consistently treats a **non-negative** result as "no pending work" (the same
convention appears at `0x002D3504` and `0x00340178`), while the older SDK convention that
`ps2_sif_dma_tests.cpp` encoded treats `-1` as "completed".

The runtime now answers truthfully for the state it actually models: `sceSifSetDma` applies the
transfer before returning, so an id it issued reports `0` (nothing pending) and an id that was never
issued still reports `-1`. That keeps the two states distinguishable and lets a completion wait
finish. The existing test was updated to the new contract with the reasoning recorded in the test.

## 4. Current blocker: CRI ADXI is bound before it is loaded

After the DMA fix the guest loads six IOP modules (`sio2man`, `padman`, `libsd`, `sdrdrv`, `sdhd`,
`snddrv`) and then spins at `0x003758B8`:

```
0x00375880  jal  0x003B2F48      ; sceSifBindRpc(client, 0x19720317 /* CRI ADXI */, 0)
0x003758C8  bnel ...             ; ~10,000-iteration delay
0x003758D0  beqz $a0, 0x00375878 ; client->server == NULL -> bind again
```

`client->server` stays NULL because SID `0x19720317` is registered only by `cri_adxi.irx`, and this
boot path never loads it — the earlier, pre-fix boot did load it as module 7 from a different list.
Only one EE thread exists (`activeThreads=1`), so this is not a missing thread: on hardware the server
must already be present when this call runs, which points at either an IOPRP/ROM image that a no-BIOS
IOP emulator does not have, or a module-load path that has not run yet in this order.

Next step: find where this boot path loads `cri_adxi` (or the IOPRP image that is expected to contain
it) and make that load happen before `0x00375860` runs.

## Invariants kept

No guest instruction was patched, no return value was invented, no synthetic GS frame was produced,
and every AOT entry came from the repository's own tools and ELF-validated addresses.

## 5. Follow-up: the ADXI bind wait is a cleared flag nobody sets

Reading the CLUT exactly (all four instructions matter, and the earlier summary was wrong about one
of them):

```
0x00375878  addiu $a0, $s0, 0xdc      ; ADXI client struct at 0x76A95C
0x0037587C  ori   $a1, $a1, 0x317     ; sid 0x19720317
0x00375880  jal   0x003B2F48          ; sceSifBindRpc
0x00375888  bgezl $v0, 0x003758A8     ; a negative result jumps to the 0x00375890 spin trap
0x0037588C  lw    $a0, 0x100($s0)     ; delay slot: the flag is *not* client->server
0x003758D0  beqz  $a0, 0x00375878     ; rebind while 0x76A980 == 0
```

So the wait is on the global `0x76A980` (context base `0x76A880` + `0x100`), not on a client field, and
a failed bind would hang at the `0x00375890` trap instead. Watching that address with
`PS2X_GUEST_WATCH_ADDR=0x76A97C` shows exactly one writer in a 40 s boot: the `memset` at
`0x003A533C`/`0x003A5388` called from `0x00377C50`, which *zeroes* the whole ADXI context. Nothing ever
sets `0x76A980`, so the rebind loop is unsatisfiable in this boot.

The loader for `cri_adxi.irx` is the instruction immediately after this wait:

```
0x002C9E18  jal   0x00375318          ; ADXI init (blocks here)
0x002C9E38  jal   0x002BE240          ; sceSifLoadModule("cdrom0:\MODULES\CRI_ADXI.IRX;1", args, 0x13)
```

The same function loads `LIBSD`, `SDRDRV`, `SDHD` and `SNDDRV` immediately before it, which is exactly
modules 3–6 of the current boot. In other words the game binds ADXI *before* loading it and expects the
flag to be raised by something asynchronous — most plausibly the ADXI server's SIF reply.

Disc facts gathered while checking the "missing module" theory:

- The ISO's real directory tree is `SYSTEM.CNF`, `SLUS_215.03`, `MODULES\IOPRP310.IMG` (278,353 bytes),
  `MODULES\IRX.IMS` (314,304 bytes), `_DATA.AFS`, `0FLISTDV.DIR`, `GODHAND.FST`. The earlier note that
  the disc had only five files was wrong: `\MODULES\` is a directory.
- `IRX.IMS` declares nine entries: `sio2man`, `padman`, `mcman`, `mcserv`, `libsd`, `sdhd`, `snddrv`,
  `cri_adxi`, `mcserv` (duplicate). The boot loads six of them.
- `IOPRP310.IMG` is a real IOP boot image with 16 embedded ELF modules, but no ADXI; `sceSifRebootIop`
  is never called in this boot, so the IOPRP is not the missing link here.
- The pre-fix log (`run23`) loaded all nine module paths, in uppercase and in table order, *before* this
  audio init — which is why ADXI init used to succeed. The CDVD fix moved the boot onto a path that
  loads modules lazily, so the earlier "load everything" step no longer runs first.
