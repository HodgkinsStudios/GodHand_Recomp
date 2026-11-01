# CP17 — Original ELF-backed indirect-call recovery

Observed first real missing guest indirect call at **source 0x0031DB2C → target 0x003BAFB8**. The target is inside the original executable MIPS segment, but was omitted from the generated AOT registration table.

The project already supports non-patching TOML `entry_points` hints. `tools/seed_aot_entry.py` now validates 32-bit little-endian MIPS ELF, a 4-byte-aligned target inside executable PT_LOAD, and genuine file-backed instruction words before adding that address to the analyzer-generated TOML. This is function-boundary metadata, **not a replacement for guest code**.

The AOT reanalysis job only publishes the new source archive if PS2Recomp actually emits a callable function and registers the address. The native-link job is triggered only by a successful publication of a changed generated source archive; it uses existing ccache objects. Failure leaves the previous successful executable in GitHub main unchanged.

Expected post-link validation: repeat the real boot test; confirm the first missing target changes or disappears, and inspect GS DMA/GIF writes and `built/frames/first_real_gs_frame.ppm`. No success claim until a genuine nonuniform GS frame is captured. All earlier unhandled instructions are still unresolved and may block later boot.
