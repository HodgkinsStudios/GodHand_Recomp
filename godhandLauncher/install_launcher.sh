#!/usr/bin/env bash
set -euo pipefail

script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
project_root="$(cd "$script_dir/.." && pwd)"
build_dir="$project_root/build-godhand-launcher"

if ! pkg-config --exists gtk+-3.0 libarchive sdl2; then
  echo 'Missing GTK3/libarchive development packages.' >&2
  echo 'Run: sudo apt install cmake ninja-build g++ pkg-config libgtk-3-dev libarchive-dev libsdl2-dev' >&2
  exit 1
fi

cmake -S "$script_dir" -B "$build_dir" -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --parallel 2
ctest --test-dir "$build_dir" --output-on-failure

install -Dm755 "$build_dir/GodHand_Recomp_Launcher" \
  "$HOME/.local/bin/GodHand_Recomp_Launcher"
install -d "$HOME/.local/share/applications"
cat > "$HOME/.local/share/applications/godhand-recomp.desktop" <<DESKTOP
[Desktop Entry]
Type=Application
Name=GodHand:Recomp
Comment=Experimental God Hand native recompilation launcher
Exec=$HOME/.local/bin/GodHand_Recomp_Launcher
Icon=applications-games
Terminal=false
Categories=Game;
StartupNotify=true
DESKTOP
chmod 644 "$HOME/.local/share/applications/godhand-recomp.desktop"
echo "Installed launcher: $HOME/.local/bin/GodHand_Recomp_Launcher"
echo 'Find GodHand:Recomp in your desktop application menu.'
