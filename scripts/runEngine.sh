#!/usr/bin/env bash
set -euo pipefail

# Session guard must run first.
if [ "${DATASENTINEL_ENV_INITIALIZED:-0}" != "1" ]; then
  echo "Environment not initialized."
  echo "Run in current shell first: source ./scripts/initEnv.sh"
  exit 1
fi

# Simple helper to run the built C++ server binary.
# Place this file in the project root and run it to start the server.

BIN="cpp/Engine/build/DataSentinelReceiver"
CMAKE_CACHE="cpp/Engine/build/CMakeCache.txt"

detect_tensorrt_runtime() {
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

binary_built_with_tensorrt() {
  if [[ ! -f "$CMAKE_CACHE" ]]; then
    return 1
  fi

  grep -q "^DS_ENABLE_TENSORRT:BOOL=ON$" "$CMAKE_CACHE"
}

if [ ! -f "$BIN" ]; then
  echo "Executable $BIN not found. Build the project first (buildEngine.sh)."
  exit 1
fi

if [ ! -x "$BIN" ]; then
  chmod +x "$BIN" || true
fi

if [[ -z "${DATASENTINEL_BACKEND:-}" ]]; then
  if binary_built_with_tensorrt && detect_tensorrt_runtime; then
    export DATASENTINEL_BACKEND="tensorrt"
  else
    export DATASENTINEL_BACKEND="onnx"
  fi
fi

if [[ -z "${DATASENTINEL_PROTOCOL:-}" ]]; then
  export DATASENTINEL_PROTOCOL="tcp"
fi

echo "Running engine with DATASENTINEL_BACKEND=$DATASENTINEL_BACKEND"
echo "Running engine with DATASENTINEL_PROTOCOL=$DATASENTINEL_PROTOCOL"
"$BIN" "$@"
