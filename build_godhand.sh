#!/usr/bin/env bash
# Incremental native God Hand AOT build. Run with: bash ./build_godhand.sh
set -Eeuo pipefail
ROOT="$(cd "$(dirname "\${BASH_SOURCE[0]}")" && pwd)"
cd "$ROOT"
ISO="\${GODHAND_ISO:-/home/hodgkinsstudios/Desktop/Roms/PS2/God Hand (USA).iso}"
JOBS="\${GODHAND_JOBS:-2}"
WORK="$ROOT/.godhand-local"
AOT="$WORK/aot"
GAME_WORK="$WORK/game"
ARCHIVE="$ROOT/built/aot/generated_cpp.tar.gz"
LOGS="$ROOT/built/reports"
GAME="$ROOT/built/linux-x86_64/GodHand_Recomp_Game"
MODE=cached
RUN=0
PUSH=0
usage() {
  cat <<'HELP'
Usage: bash ./build_godhand.sh [--regenerate] [--run] [--push] [--jobs N] [--iso PATH]
  Default: incremental build using existing AOT C++ archive (no ISO required).
  --regenerate: analyze the supplied local ISO and regenerate game AOT C++.
  --run: run completed native game with boot ELF extracted from local ISO.
  --push: commit successful game binary and checksum to GitHub main.
  --jobs N: compiler parallelism, default 2 (use 4+ on powerful machines).
  --iso PATH: overrides the default desktop God Hand ISO path.
  Env alternatives: GODHAND_ISO, GODHAND_JOBS.
NOTE: A successful link does not guarantee game boot or real rendered frames.
HELP
}
while (($#)); do
  case "$1" in
    --regenerate) MODE=regenerate; shift ;;
    --run) RUN=1; shift ;;
    --push) PUSH=1; shift ;;
    --jobs) [[ $# -ge 2 ]] || { echo 'Missing --jobs N' >&2; exit 2; }; JOBS="$2"; shift 2 ;;
    --iso) [[ $# -ge 2 ]] || { echo 'Missing --iso PATH' >&2; exit 2; }; ISO="$2"; shift 2 ;;
    -h|--help) usage; exit 0 ;;
    *) echo "Unknown option: $1" >&2; usage >&2; exit 2 ;;
  esac
done
[[ "$JOBS" =~ ^[1-9][0-9]*$ ]] && ((JOBS<=128)) || { echo "Invalid job count: $JOBS" >&2; exit 2; }
[[ -f CMakeLists.txt && -d ps2xRuntime && -f tools/godhand_iso_pipeline.py ]] || {
  echo 'Run from your existing GodHand_Recomp source root.' >&2; exit 1; }
for cmd in cmake ninja g++ python3 tar sha256sum gzip; do
  command -v "$cmd" >/dev/null || { echo "Missing dependency: $cmd" >&2; exit 1; }
done
mkdir -p "$WORK" "$LOGS" "$(dirname "$GAME")"
if [[ "$MODE" == regenerate ]]; then
  [[ -r "$ISO" ]] || { echo "ISO unavailable: $ISO" >&2; exit 1; }
  echo 'Extracting original boot ELF from ISO...'
  mkdir -p "$GAME_WORK"
  python3 tools/godhand_iso_pipeline.py --iso "$ISO" --out "$GAME_WORK" --extract-elf
  echo 'Compiling analyzer and recompiler...'
  cmake -S . -B build-aot-local -G Ninja -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_C_FLAGS=-msse4.1 -DCMAKE_CXX_FLAGS=-msse4.1 \
    -DPS2X_BUILD_RECOMP=ON -DPS2X_BUILD_ANALYZER=ON \
    -DPS2X_BUILD_RUNTIME=OFF -DPS2X_BUILD_STUDIO=OFF -DPS2X_BUILD_TEST=OFF
  cmake --build build-aot-local --parallel "$JOBS"
  analyzer="$(find build-aot-local -type f -name ps2_analyzer -executable -print -quit)"
  recomp="$(find build-aot-local -type f -name ps2_recomp -executable -print -quit)"
  [[ -n "$analyzer" && -n "$recomp" ]] || { echo 'Missing analyzer/recompiler binaries' >&2; exit 1; }
  "$analyzer" "$GAME_WORK/boot.elf" "$GAME_WORK/godhand.toml" > "$LOGS/local_analyzer.log" 2>&1 || {
    tail -n 40 "$LOGS/local_analyzer.log"; exit 1; }
  "$recomp" "$GAME_WORK/godhand.toml" > "$LOGS/local_recompiler.log" 2>&1 || {
    tail -n 40 "$LOGS/local_recompiler.log"; exit 1; }
  [[ -s "$GAME_WORK/output/register_functions.cpp" ]] || {
    echo 'AOT did not generate its function table' >&2; exit 1; }
  rm -rf "$AOT"; mkdir -p "$AOT"
  cp -a "$GAME_WORK/output/." "$AOT/"
  mkdir -p "$(dirname "$ARCHIVE")"
  tar -czf "$ARCHIVE" -C "$AOT" .
else
  if [[ ! -s "$AOT/register_functions.cpp" ]]; then
    if [[ ! -s "$ARCHIVE" ]] || ! gzip -t "$ARCHIVE" 2>/dev/null; then
      if [[ -d .git ]] && command -v git-lfs >/dev/null; then
        echo 'Retrieving only the generated AOT C++ archive using Git LFS...'
        git lfs pull --include='built/aot/generated_cpp.tar.gz' --exclude=''
      fi
    fi
    if [[ ! -s "$ARCHIVE" ]] || ! gzip -t "$ARCHIVE" 2>/dev/null; then
      echo 'AOT archive missing (or LFS pointer). Use a git clone with git-lfs, or --regenerate.' >&2
      exit 1
    fi
    mkdir -p "$AOT"
    tar -xzf "$ARCHIVE" -C "$AOT"
  fi
fi
for file in register_functions.cpp ps2_recompiled_functions.h ps2_recompiled_stubs.h; do
  [[ -s "$AOT/$file" ]] || { echo "Missing generated $file" >&2; exit 1; }
done
echo "Preparing native runtime and $(find "$AOT" -name '*.cpp' | wc -l) guest C++ files..."
cmake -S . -B build-game-local -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_FLAGS=-msse4.1 -DCMAKE_CXX_FLAGS=-msse4.1 \
  -DPS2X_BUILD_RECOMP=OFF -DPS2X_BUILD_ANALYZER=OFF \
  -DPS2X_BUILD_RUNTIME=ON -DPS2X_BUILD_STUDIO=OFF -DPS2X_BUILD_TEST=OFF \
  -DPS2X_ENABLE_DEBUG_UI=OFF -DPS2X_ENABLE_FFMPEG=OFF \
  -DPS2X_ENABLE_RUNNER_UNITY_BUILD=OFF -DPS2X_ENABLE_RUNNER_PCH=OFF \
  -DPS2X_AOT_FAST_COMPILE=ON -DPS2X_AOT_SOURCE_DIR="$AOT" \
  2>&1 | tee "$LOGS/local_configure.log"
echo "Building native executable ($JOBS compile jobs). Progress: $LOGS/local_build.log"
code=0
cmake --build build-game-local --target ps2EntryRunner --parallel "$JOBS" \
  2>&1 | tee "$LOGS/local_build.log" || code=$?
printf 'native_link_exit_code=%s\n' "$code" > "$LOGS/local_link_status.txt"
if ((code != 0)); then
  echo "Game link failed (exit $code). First errors:" >&2
  grep -n -m 25 -E 'FAILED:|error:|fatal error:|undefined reference|collect2:|ninja: build stopped' \
    "$LOGS/local_build.log" >&2 || true
  echo "Full log: $LOGS/local_build.log" >&2
  exit "$code"
fi
native=build-game-local/ps2xRuntime/ps2EntryRunner
[[ -x "$native" ]] || { echo 'Build succeeded without producing ps2EntryRunner' >&2; exit 1; }
install -m755 "$native" "$GAME"
(cd "$(dirname "$GAME")" && sha256sum "$(basename "$GAME")" > GodHand_Recomp_Game.sha256)
echo "Built native game at: $GAME"
echo 'Link success is not proof of game boot or real rendered frames.'
if ((PUSH)); then
  [[ -d .git && "$(git branch --show-current)" == main ]] || {
    echo 'Publishing requires a git checkout on branch main.' >&2; exit 1; }
  git add -f built/linux-x86_64/GodHand_Recomp_Game built/linux-x86_64/GodHand_Recomp_Game.sha256
  if ! git diff --cached --quiet; then
    git commit -m 'build: native God Hand AOT executable (boot unverified) [skip ci]'
    git push origin main
  fi
fi
if ((RUN)); then
  [[ -r "$ISO" ]] || { echo "--run requires ISO: $ISO" >&2; exit 1; }
  if [[ ! -s "$GAME_WORK/boot.elf" ]]; then
    mkdir -p "$GAME_WORK"
    python3 tools/godhand_iso_pipeline.py --iso "$ISO" --out "$GAME_WORK" --extract-elf
  fi
  export GODHAND_DATA_DIR="\${GODHAND_DATA_DIR:-$HOME/.local/share/HodgkinsStudios/GodHand_Recomp}"
  export GODHAND_GS_FRAME_PPM="\${GODHAND_GS_FRAME_PPM:-$GODHAND_DATA_DIR/first_real_gs_frame.ppm}"
  mkdir -p "$(dirname "$GODHAND_GS_FRAME_PPM")"
  exec "$GAME" "$GAME_WORK/boot.elf"
fi
