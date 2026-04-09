#!/bin/bash
set -e
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
make clean
make
[ -f "hyprview.so" ] && echo "✅ Build successful: hyprview.so" || (
  echo "❌ Build failed"
  exit 1
)
