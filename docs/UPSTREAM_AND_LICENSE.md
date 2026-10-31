# Upstream attribution, provenance and licensing

## Origin

**GodHand_Recomp** began from a user-supplied archive named `PS2Recomp-main.zip`. The original project is **PS2Recomp**, created and maintained by **ran-j and the upstream contributors**:

- **Project:** https://github.com/ran-j/PS2Recomp
- **Documentation / wiki:** https://github.com/ran-j/PS2Recomp/wiki
- **Upstream license:** https://github.com/ran-j/PS2Recomp/blob/main/LICENSE
- **Contributors:** https://github.com/ran-j/PS2Recomp/graphs/contributors

The exact upstream commit hash for the supplied ZIP was **not recorded in the archive**, so this checkpoint does not assert a specific upstream revision. A future change should record an exact comparison base when identified.

## Credit and modifications

The included `ps2xAnalyzer`, `ps2xRecomp`, `ps2xRuntime`, `ps2xIOP`, `ps2xStudio`, `ps2xTest`, `android` and `vita` source and infrastructure are PS2Recomp-derived; credits for that engineering belong to its original authors and other contributors. The present checkpoint only makes the following project-specific changes:

- Replace the **root** README to describe GodHand_Recomp and clearly distinguish goals from achieved features.
- Add `docs/` files describing project status, upstream credit and intended local-only ISO workflow.
- Add `CHANGELOG.md` recording the documentation-only import.
- Expand `.gitignore` patterns to avoid accidentally staging game discs and local generated content.

**No upstream C++ implementation files, build logic, license text or existing GitHub Actions workflow are intentionally modified in this checkpoint.** Existing module-level readmes remain inherited technical reference documentation and may mention PS2Recomp names by design.

Creating a separate README and name does **not** make the rest of the code original or remove upstream copyright. Future changes should preserve existing source notices, describe actual modifications and continue giving credit to the authors of borrowed components.

## License

The archive's original `LICENSE` is kept intact. PS2Recomp is licensed under **GNU GPL version 3**; modified and redistributed covered code remains subject to that license and its obligations. The repository may have independently licensed dependencies and third-party components, whose notices should also be preserved and checked before release.

This project does not contain a God Hand ISO, original PS2 executable, audio/video, textures or other retail game assets. *God Hand* and related game content belong to their respective rights holders. This project is an unofficial effort with no stated endorsement from Capcom, Clover Studio or PS2Recomp's maintainers.

## Other upstream acknowledgments

The original PS2Recomp README also acknowledges N64Recomp, ELFIO, toml11, fmt, and PCSX2 as inspiration, libraries or references. Preserve original copyright notices and dependency licenses. Refer to upstream's documentation for the source and provenance of individual third-party components.
