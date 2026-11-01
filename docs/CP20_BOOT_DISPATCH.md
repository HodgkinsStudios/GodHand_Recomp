# CP20 — Continue real native God Hand boot

## Current verified artifacts

- CP19 real native game executable at `built/linux-x86_64/GodHand_Recomp_Game`, **209,109,192 bytes**, Git LFS SHA-256 `6532ea3b9b66eb06b6cece1523c9b9f9fe9a7e75a4be3352bf364f99878f795b`.
- CP19 linked AOT source archive under `built/aot/generated_cpp.tar.gz`, SHA-256 `05dad8f4f4c9680a13899dd8906be18facd5ba60534ec28731b1474f4fa5d866`, 25,000,292 bytes.
- Hosted CP19 linker + Xvfb boot test `41323942723` succeeded at compile/link. Guest boot stalled with a missing indirect JALR target **0x0031DA28** (caller `0x0031DB2C`). This address lies before the generated routine `0x0031DA48`.
- The same runtime trace's `s0[8]=0x00375840` points to nearby executable code immediately before the generated routine `0x00375860`. This second entry is a *pointer-derived candidate*, not yet proven executed; it is separately validated against original ELF and the recompiled registration table.

## Targeted pass, never mocked

CP20 requests the recompiler register genuine original PS2 ELF callable entries:
`0x003BAFB8`, `0x0038F200`, `0x0038B6E0`, `0x0031DA28`, `0x00375840`.

`tools/seed_aot_entry.py` rejects unaligned, non-executable, unmapped and zero/fill instruction words before updating the TOML. Successful generation must include **all five** addresses in the generated `register_functions.cpp` and preserve original instructions. The native linker enforces the same entries before replacing the existing binary. All AOT functions remain compiled, and a successful host link is not treated as game playability.

Inspect `built/reports/link_ci_frame.txt` and only claim an actual rendered frame if real GS writes produced a nonuniform guest framebuffer in `built/frames/first_real_gs_frame.ppm`. Zero guest DMA/GIF/GS/VIF counters mean native window creation alone is not a game image.

## Future work

The original ELF's executable load region can contain embedded function-pointer tables, so coarse ELF section classification may miss additional callable starts. Investigate general discovery from trusted original code pointers rather than continuously seeding addresses. Avoid PS2 game-specific stubs, HLE stand-ins, or fabricated display frames.
