#!/usr/bin/env bash
# Installs the GitHub Actions-built Linux launcher without compiling it.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BINARY="$ROOT/built/linux-x86_64/GodHand_Recomp_Launcher"
CHECKSUM="$ROOT/built/linux-x86_64/SHA256SUMS.txt"
if [[ ! -f "$BINARY" || ! -f "$CHECKSUM" ]]; then
  echo 'The verified compiled launcher is missing from built/linux-x86_64/.' >&2
  echo 'Pull the latest GitHub main branch or run the cloud launcher workflow.' >&2
  exit 1
fi
(cd "$ROOT/built/linux-x86_64" && sha256sum -c SHA256SUMS.txt)
mkdir -p "$HOME/.local/bin" "$HOME/.local/share/applications"
install -m755 "$BINARY" "$HOME/.local/bin/GodHand_Recomp_Launcher"
GAME="$ROOT/built/linux-x86_64/GodHand_Recomp_Game"
if [[ -f "$GAME" && -x "$GAME" ]]; then
  mkdir -p "$HOME/.local/share/HodgkinsStudios/GodHand_Recomp/bin"
  install -m755 "$GAME" "$HOME/.local/share/HodgkinsStudios/GodHand_Recomp/bin/GodHand_Recomp_Game"
else
  echo 'Playable AOT game binary not present yet (launcher will display a clear message).'
fi
cat > "$HOME/.local/share/applications/godhand-recomp.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=GodHand:Recomp
Comment=GodHand recompilation project by HodgkinsStudios
Exec=$HOME/.local/bin/GodHand_Recomp_Launcher
Icon=applications-games
Terminal=false
Categories=Game;
StartupNotify=true
EOF
chmod 644 "$HOME/.local/share/applications/godhand-recomp.desktop"
echo "Launcher installed: $HOME/.local/bin/GodHand_Recomp_Launcher"
