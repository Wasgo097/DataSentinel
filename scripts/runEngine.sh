#!/usr/bin/env bash
set -euo pipefail

# Simple helper to run the built C++ server binary.
# Place this file in the project root and run it to start the server.

BIN="cpp/build/receiver"

if [ ! -f "$BIN" ]; then
  echo "Executable $BIN not found. Build the project first (e.g. cd cpp && mkdir -p build && cd build && cmake .. && make)."
  exit 1
fi

if [ ! -x "$BIN" ]; then
  chmod +x "$BIN" || true
fi

 exec "$BIN" "$@"
#exec "$BIN"
