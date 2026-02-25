#!/bin/bash
#set -euo pipefail

# znajdź katalog główny repo (czyli katalog nadrzędny scripts/)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CPP_DIR="$PROJECT_ROOT/cpp/Engine"
BUILD_DIR="$CPP_DIR/build"

echo "Project root: $PROJECT_ROOT"
echo "Building C++ engine..."

cmake -S "$CPP_DIR" -B "$BUILD_DIR"
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "Build completed."