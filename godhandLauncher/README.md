# GodHand:Recomp — GTK3 launcher (Ubuntu)

This launcher is a **new GodHand_Recomp-specific component** by HodgkinsStudios, alongside the upstream-derived [PS2Recomp](https://github.com/ran-j/PS2Recomp) source. It does **not** include a game recompiler output or playable binary.

## Current functionality

- Native GTK3 ISO chooser (Linux desktop portal integration where available).
- On first installation, copies the chosen disc image into `~/.local/share/HodgkinsStudios/GodHand_Recomp/GodHand.iso`.
- Extracts the ISO9660 file system into `~/.local/share/HodgkinsStudios/GodHand_Recomp/Extracted_Iso` using libarchive, with directory traversal protection and no symlink materialization.
- Runs copy/extraction in a worker thread with responsive progress UI; only marks installation complete after both steps finish.
- On later launches, displays **Play**. Attempts to run `~/.local/share/HodgkinsStudios/GodHand_Recomp/bin/GodHand_Recomp_Game`. The executable does not currently exist; a clear dialog explains this until an AOT native game build is produced.
- Preserves all game data **locally**, outside the source repo.

This is an **ISO selection and installation** mechanism, not a legal validation of ownership. No data is uploaded to GitHub by the launcher.

## Install the Ubuntu desktop launcher

From the repository root, run `bash godhandLauncher/install_launcher.sh`. This compiles and tests the launcher, installs it into `~/.local/bin/`, and adds a desktop application entry. A command-line build is available below.

## Build only the launcher

```bash
sudo apt install -y cmake g++ pkg-config libgtk-3-dev libarchive-dev
cmake -S godhandLauncher -B build-godhand-launcher -DCMAKE_BUILD_TYPE=Release
cmake --build build-godhand-launcher --parallel 2
ctest --test-dir build-godhand-launcher --output-on-failure
./build-godhand-launcher/GodHand_Recomp_Launcher
```

The launcher can also be enabled in the root build with `-DGODHAND_BUILD_LAUNCHER=ON`. On machines without GTK3 development files, its native GUI is skipped and the non-GUI installer tests still build if libarchive is present.

**Free disk space:** allow approximately twice the ISO size for its copied image and extracted assets. The file chooser accepts a user-supplied ISO; the backend currently expects ISO9660. A game using other formats may need extraction support added.

**Do not commit the local `GodHand.iso`, extracted files, generated game code, or any retail assets.**

## Local self-hosted GitHub Actions

The standalone `godhand_iso_inspect --inspect-iso /path/to/file.iso` utility reports whether an ISO9660 disc can be enumerated, without copying/extracting the disc or printing private file names. The repository also has `.github/workflows/godhand-local.yml`, which must be explicitly dispatched on the trusted main branch. The runner should be isolated from personal files and should **not** process untrusted pull requests. It validates the user's private ISO and runs a launcher build/test pass; there is no retail ISO or extracted assets upload step.

Because a self-hosted runner running in a public repository executes repository-defined code, configure a restricted runner account/VM with access only to a *read-only* ISO mount and needed build workspace. A manual trigger by itself is not a security boundary.

Before running the local Actions workflow, install `libgtk-3-dev` and `libarchive-dev` (plus the upstream PS2Recomp build dependencies) on the isolated runner. The workflow checks for GTK3 and fails clearly if the native launcher was not built. It does not install sudo packages on your behalf.

## CP02 Settings (development)

The GTK3 launcher includes a Settings dialog with remappable keyboard and SDL2 controller actions, hotplug detection and persistent screen mode (Windowed, Fullscreen and Fullscreen stretched). Settings are stored at `~/.config/HodgkinsStudios/GodHand_Recomp/settings.ini`. Install GTK3, libarchive **and SDL2** development packages. Input helper functions are separate from the UI and intended for eventual AOT game integration. Esc cycling within actual gameplay requires wiring the native game loop, which does not yet exist.

## Install from prebuilt development binary (no compilation)

After pulling the latest `main`, run `bash godhandLauncher/install_prebuilt.sh`. This verifies `built/linux-x86_64/SHA256SUMS.txt`, copies the GitHub-hosted Actions-built launcher to `~/.local/bin/` and creates a desktop launcher. It also installs a native game binary automatically if a verified executable is later added to `built/linux-x86_64/GodHand_Recomp_Game` (not available yet). GTK3, SDL2 and libarchive runtime libraries must be present on Ubuntu.
