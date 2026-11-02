# CP36 — Native AFS auto-extraction and repair

## Requirement
The original ISO9660 extraction does not unpack CRI `_DATA.AFS`: the file in `Extracted_Iso` remains a ~1.54 GB archive. The native launcher must automatically unpack it during first install, check every later launch, and repair incomplete extraction before attempting a game run.

## Implementation
The new standalone native C++17 module `godhandLauncher/src/afs_installer.{hpp,cpp}` parses the genuine `AFS\0` index (offset/size pairs and optional 48-byte names), bounds-checks every original entry and destination file, and uses streaming 1 MiB chunks rather than allocating 1.5 GiB. It creates `~/.local/share/HodgkinsStudios/GodHand_Recomp/Extracted_AFS/000000_name` etc. using safe sanitized, index-prefixed filenames. The full first install copies the ISO, extracts its filesystem, *then* unpacks all AFS entries and only then writes `install.complete`.

Every launcher startup on an existing installation verifies a separate AFS completion marker and every output file's regular-file type/expected byte length. Missing or damaged-size entries are atomically repaired in a background worker, with progress in the GTK window. Play likewise checks readiness and starts repair instead of launching incomplete assets. Existing valid entries are skipped. Stale `.partial` temporary files are discarded. Directory fd and `O_NOFOLLOW` deny symlink traversals in the extraction directory. AFS content never enters GitHub.

## Limitations and next graphics work
This stage unpacks **top-level** CRI AFS entries, including the 188 embedded `.afs` files as files; nested audio archives are not recursively exploded by default. The original game runtime still reads `GodHand.iso` through its CD/DVD layer; extracted files are for analysis and preparation, **not a replacement for the original disc APIs**. Extracting assets cannot solve missing EE AOT function registrations by itself.

The CP35 runtime advanced to a third absent original indirect-call entry `0x003329D0`, after verifying prior restored entries `0x003364F8` and `0x00334230`. Continue the authentic AOT entry restoration pipeline. A non-uniform GS framebuffer is still required before declaring actual visible rendering.

## Regression
`godhand_installer_tests` creates a synthetic ISO9660 image with a synthetic two-entry CRI AFS; verifies full first-run extraction, repeat no-op, recovery after file deletion, and recovery after size truncation. No retail bytes are in the test fixture.
