#!/usr/bin/env bash
set -euo pipefail

# Session guard must run first.
if [ "${DATASENTINEL_ENV_INITIALIZED:-0}" != "1" ]; then
  echo "Environment not initialized."
  echo "Run in current shell first: source ./scripts/initEnv.sh"
  exit 1
fi

# znajdź katalog główny repo (czyli katalog nadrzędny scripts/)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

CPP_DIR="$PROJECT_ROOT/cpp/Engine"
BUILD_DIR="$CPP_DIR/build"

has_explicit_tensorrt_flag() {
  local arg
  for arg in "$@"; do
    if [[ "$arg" == -DDS_ENABLE_TENSORRT=* ]]; then
      return 0
    fi
  done
  return 1
}

detect_tensorrt() {
  local include_candidates=(
    "/usr/include/x86_64-linux-gnu/NvInfer.h"
    "/usr/include/NvInfer.h"
    "/usr/local/include/NvInfer.h"
    "/usr/local/TensorRT/include/NvInfer.h"
  )

  local include_found=0
  local p
  for p in "${include_candidates[@]}"; do
    if [[ -f "$p" ]]; then
      include_found=1
      break
    fi
  done

  if [[ $include_found -eq 0 ]]; then
    return 1
  fi

  local libnvinfer_found=0
  local libnvonnxparser_found=0

  if command -v ldconfig >/dev/null 2>&1; then
    if ldconfig -p 2>/dev/null | grep -q "libnvinfer.so"; then
      libnvinfer_found=1
    fi
    if ldconfig -p 2>/dev/null | grep -q "libnvonnxparser.so"; then
      libnvonnxparser_found=1
    fi
  fi

  if [[ $libnvinfer_found -eq 0 ]]; then
    if compgen -G "/usr/lib/x86_64-linux-gnu/libnvinfer.so*" >/dev/null; then
      libnvinfer_found=1
    elif compgen -G "/lib/x86_64-linux-gnu/libnvinfer.so*" >/dev/null; then
      libnvinfer_found=1
    fi
  fi

  if [[ $libnvonnxparser_found -eq 0 ]]; then
    if compgen -G "/usr/lib/x86_64-linux-gnu/libnvonnxparser.so*" >/dev/null; then
      libnvonnxparser_found=1
    elif compgen -G "/lib/x86_64-linux-gnu/libnvonnxparser.so*" >/dev/null; then
      libnvonnxparser_found=1
    fi
  fi

  if [[ $libnvinfer_found -eq 0 || $libnvonnxparser_found -eq 0 ]]; then
    return 1
  fi

  return 0
}

echo "Project root: $PROJECT_ROOT"
echo "Building C++ engine..."

EXTRA_CMAKE_ARGS=("$@")
if ! has_explicit_tensorrt_flag "$@"; then
  if detect_tensorrt; then
    EXTRA_CMAKE_ARGS+=("-DDS_ENABLE_TENSORRT=ON")
    echo "TensorRT detected (headers + nvinfer + nvonnxparser): enabling -DDS_ENABLE_TENSORRT=ON"
  else
    EXTRA_CMAKE_ARGS+=("-DDS_ENABLE_TENSORRT=OFF")
    echo "TensorRT not detected (missing header or shared libs): building ONNX-only binary"
  fi
fi

cmake -S "$CPP_DIR" -B "$BUILD_DIR" "${EXTRA_CMAKE_ARGS[@]}"
cmake --build "$BUILD_DIR" -j"$(nproc)"

echo "Build completed."
