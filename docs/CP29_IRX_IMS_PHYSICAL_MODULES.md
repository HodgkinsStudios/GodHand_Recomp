# CP29 — Original IRX.IMS archive loading

## Grounded discovery
A GitHub-hosted inspection of the original 1,567,686,656-byte God Hand disc confirmed that the ISO9660 MODULES directory contains IRX.IMS, not independent SDRDRV.IRX, PADMAN.IRX, or SIO2MAN.IRX. IRX.IMS is 314,304 bytes, has an 'IMS' header and 9 directory entries, and contains nine embedded genuine MIPS ELF modules. One directory record is SDRDRV.IRX. Previous real-game traces looped after failure to load that module.

## Fix
The existing IOP module loader now attempts the regular host/extracted path, original ISO9660 file extent, and finally, only for CDROM paths, the authentic MODULES/IRX.IMS directory. The IMS directory is 16-byte header with 32-byte records (filename[16], offset32, size32, reserved[8]). Every record's extent is bounds-checked. Only original embedded ELF bytes are loaded; the existing MIPS IRX image/relocation loader runs unmodified. No made-up functions, game-specific CPU patching or module stubs.

## Test
Added a synthetic ISO9660 fixture containing an IMS archive with two distinct embedded MIPS ELF images; tests verify exact byte match and correctly reject missing modules and corrupt archive extents. Subsequent full native game AOT and GS framebuffer tests are required to certify further boot progress.

## Related
The ISO inventory is built/reports/disc_file_inventory.txt. The physical original archive header and nine MIPS ELF offsets are listed in built/reports/irx_ims_layout.txt.
