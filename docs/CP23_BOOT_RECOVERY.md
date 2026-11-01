# CP23 — ELF-backed recovery from real missing-call logs

The CP22 generic ELF pointer discovery pass found 131 additional generated translation units (relative to CP19), including `sub_0031CE98_0x31ce98.cpp`, the most recently reported missing `JALR` target. CP22 native link / genuine frame test is run `41326634630`; the updated AOT source archive was published to main first.

New diagnostic helper `tools/observed_entry_recovery.py` eliminates manually transcribing one missing call at a time. It reads **actual guest** `[guest-branch:missing-target]` diagnostics, extracts only 4-byte-aligned targets explicitly marked `codeRegion=yes` and with an observed caller, then calls `tools/seed_aot_entry.py`'s original ELF validator for every target *before* updating analyzer TOML. No guessed target, synthetic guest operation, HLE or framebuffer is inserted. Existing entries are preserved and duplicates are ignored.

Usage with a boot ELF and analyzer-generated TOML:

```bash
python3 tools/observed_entry_recovery.py \
  --elf /path/to/extracted/boot.elf \
  --log /path/to/real/native-execution.log \
  --config /path/to/generated/godhand.toml \
  --report built/reports/observed_entry_recovery.json
```

The full real host execution log is retained by CI artifacts; `built/reports/link_ci_frame.txt` also records the initial missing target and can be supplied as the log input. Re-running the helper is idempotent. It is intentionally **not auto-applied** by CI: review each new generated registry and original ELF confirmation before committing a new AOT archive.

Six synthetic-ELF regression checks cover the observed path, duplicate deduplication, idempotence, bad alignment, non-executable pointers, out-of-ELF candidates, and irrelevant tick lines. No game-specific runtime behavior is added.

The definitive milestone remains a real nonuniform guest GS frame in `built/frames/first_real_gs_frame.ppm`; host OpenGL startup alone is insufficient.
