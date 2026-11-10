#!/usr/bin/env sh
set -eu
# Usage: ./scripts/build-cross.sh /path/to/zig
zig_binary=${1:?Pass the path to Zig 0.15.2}
project_dir=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
cd "$project_dir"
mkdir -p out
"$zig_binary" c++ -target x86_64-windows-gnu -std=c++17 -O2 -Wall -Wextra -Werror \
  -Wno-unused-command-line-argument -DUNICODE -D_UNICODE -municode -Wl,--subsystem,windows \
  -I include src/launcher.cpp src/game.cpp -lversion -lgdi32 -luser32 -ldwmapi \
  -o out/Titanium-Emporer.exe
"$zig_binary" c++ -target x86_64-windows-gnu -std=c++17 -O2 -Wall -Wextra -Werror \
  -shared src/client.cpp -o out/titanium-client.dll
