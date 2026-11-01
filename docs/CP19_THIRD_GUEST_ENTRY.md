# CP19 — Third observed genuine PS2 MIPS callable entry

## Authoritative current outputs (verified on GitHub main)

- CP18 generated code: `built/aot/generated_cpp.tar.gz` (Git LFS SHA-256 `9c140fcb3992aa2eae2f3bba7d3e36d354be71112abb4c9c695de20fa3ae7d74`; 25,005,158 bytes).
- CP18 host executable: `built/linux-x86_64/GodHand_Recomp_Game` (Git LFS SHA-256 `40f776e1f95b76723d74ce745d7c668f3865d7472a986b1129cfef26e3e11058`; 209,109,192 bytes).
- CP18 native link and runtime test run `41322708904`: all four compiler batches succeeded, 9,230 compiler-cache hits, linking succeeded. Guest original ELF executed past 0x003BAFB8 and 0x0038F200. No GS writes or real frame captured.

## New observed stop

The next MIPS indirect JALR call from `0x0031DB2C` targets `0x0038B6E0`. The guest's actual instruction at that address, as shown in the runtime's original ELF mapping, is `0x03E00008` (JR RA); next word is `0x00000000` (NOP). The nearest subsequent generated translation unit begins at `0x0038B6E8`. This is evidence of a missed short callable thunk/boundary, *not* a reason to skip the call or return a fabricated value.

The CP19 AOT analysis must check the original executable segment and instruction backing for all three observed function entries:
`0x003BAFB8`, `0x0038F200`, `0x0038B6E0`.
The verified addresses are used exclusively as PS2Recomp function discovery metadata. No guest ELF code changes, HLE replacement or graphics simulation.

The workflow will reject generated AOT code unless all three appear in the original guest function registry. Only a fully successful native link may replace the previous known-good host executable. New graphics must be produced by real guest GS writes; a blank host OpenGL window is not a game frame.

## Next evidence

After the next build and real-ELF smoke test, inspect `built/reports/link_ci_frame.txt`. If execution advances, prioritize guest DMA/GIF/GS/VIF counters and first authentic GS frame. Preserve checkpoints and checksum records.
