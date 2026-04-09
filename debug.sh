#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
make clean
make CXXFLAGS="-std=c++23 -fPIC -Wall -Wextra -g -O0 -DDEBUG -Isrc $(pkg-config --cflags hyprland)"
[ ! -f "hyprview.so" ] && echo "❌ Debug build failed" && exit 1
export HYPRLAND_LOG_LEVEL=3
Hyprland
