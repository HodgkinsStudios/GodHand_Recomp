# CP30 — AFS game-data investigation and bounded extraction tool

God Hand's original NTSC-U disc contains _DATA.AFS (1,541,885,952 bytes), GODHAND.FST, 0FLISTDV.DIR, and MODULES/IRX.IMS. IRX.IMS was verified and the CP29 loader has unit tests passing.

## New tool
tools/godhand_afs.py is a standard-library CRI AFS parser with a bounds-checked index and optional filename TOC. It performs random reads of the original ISO without loading the whole 1.5-GiB archive in memory. It classifies entry header signatures (MIPS ELF, textures, video, audio, archive types), reports the available metadata and small samples of FST/0FLISTDV strings, and can extract selected entries or the entire archive into a local directory using bounded streaming. Extraction refuses unsafe pathnames and unexpected entry offsets.

Examples, from repository root:

python3 tools/godhand_afs.py --iso "/home/hodgkinsstudios/Desktop/Roms/PS2/God Hand (USA).iso" --report afs_report.json
python3 tools/godhand_afs.py --iso "/home/hodgkinsstudios/Desktop/Roms/PS2/God Hand (USA).iso" --extract-dir ./Extracted_AFS --ids 0,1,2 --max-total-mib 128
python3 tools/godhand_afs.py --iso "/home/hodgkinsstudios/Desktop/Roms/PS2/God Hand (USA).iso" --extract-dir ./Extracted_AFS --all --max-total-mib 2048 --max-entry-mib 2048

For an already-extracted _DATA.AFS file, replace --iso with --afs.

## Testing and privacy
An automated GitHub Actions job first runs synthetic fixture tests, then indexes the supplied original ISO in the temporary runner, extracts a bounded sample of small AFS entries for actual inspection, and removes those binaries. Only metadata reports are committed: built/reports/afs_inventory.json and built/reports/afs_analysis_summary.txt. No retail AFS entry bytes are committed or uploaded as build artifacts.

The AFS analysis does not claim a visible game frame; genuine graphics require a successful AOT execution and a nonuniform guest GS framebuffer.
