#!/usr/bin/env bash
# usage: ./build.sh [project-dir]   (defaults to the current directory)
set -euo pipefail

project=$(cd "${1:-.}" && pwd)
build="$project/build"

[ -f "$project/CMakeLists.txt" ] || { echo "no CMakeLists.txt in $project" >&2; exit 1; }

cmake -S "$project" -B "$build"
cmake --build "$build"

uf2=$(find "$build" -maxdepth 1 -name '*.uf2' -print -quit)
[ -n "$uf2" ] || { echo "no .uf2 produced in $build" >&2; exit 1; }

bootsel_drive() {
  local letter
  for letter in {a..z}; do
    if [ -f "/$letter/INFO_UF2.TXT" ]; then
      echo "${letter^^}"
      return 0
    fi
  done
  return 1
}

wait_for_bootsel() {
  local letter
  for _ in $(seq 1 20); do
    letter=$(bootsel_drive)
    if [ -n "$letter" ]; then
      echo "$letter"
      return 0
    fi
    sleep 0.5
  done
  return 1
}

if picotool load -f -x "$uf2" >/dev/null 2>&1; then
  echo "flashed $uf2"
else
  echo "picotool could not flash, using the BOOTSEL drive"
  letter=$(wait_for_bootsel) || {
    echo "board is not in BOOTSEL mode: hold the BOOTSEL button and replug it" >&2
    exit 1
  }
  cp "$uf2" "${letter}:/"
  echo "flashed $uf2 via ${letter}:"
fi