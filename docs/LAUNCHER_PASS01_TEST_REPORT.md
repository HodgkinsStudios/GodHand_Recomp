# GodHand:Recomp Launcher — CP01 Verification

Date: 2026-10-31

## Verified here

- C++17/libarchive backend compilation with system CMake and GCC succeeded.
- Standalone `ctest --output-on-failure`: **1/1 passed**.
- Test creates a synthetic ISO9660 image at runtime, then validates disc parsing, local copy, extraction, persistent install marker, idempotent re-entry, unsafe path filtering, and clean failure for invalid images.
- Tested the optional launcher component from top-level CMake with upstream components disabled. No retail game data was used.

## Unverified / blocked

- This environment does not have GTK3 development headers; full GTK3 UI binary could not be compiled or visually launched here. The new GitHub Actions Linux launcher build is designed to verify this.
- The user's real ISO is on their Ubuntu computer, inaccessible from this container. Its files and hashes have **not** been inspected.
- No accessible method in this session to dispatch the private local-disk workflow on the user's runner. It must be manually started once the source changes have been pushed.
- God Hand AOT executable is not yet generated. Play intentionally displays a missing-executable error.
- No God Hand boot, graphics, audio or gameplay verification.

## Local runner requirements

- Isolated Ubuntu runner with GTK3/libarchive development packages, full PS2Recomp build dependencies, and read access to the path stored in the `GODHAND_ISO_PATH` Actions variable.
- Trigger `GodHand Local Build` only against trusted `main` commits. Never run arbitrary public PR code on a runner that can read local files.
- Do not add retail data to workflow artifacts or logs.
