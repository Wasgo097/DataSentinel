#!/usr/bin/env bash
set -euo pipefail

if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  echo "Do not source this script. Run: ./scripts/docker/runEngineDocker.sh"
  return 1
fi

# Resolve project root and compose file path.
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="$PROJECT_ROOT/docker/compose.yaml"
MODELS_DIR="$PROJECT_ROOT/models"
TENSORRT_REPO_DEB="${TENSORRT_REPO_DEB:-$PROJECT_ROOT/docker/engine/deps/nv-tensorrt-local-repo-ubuntu2404-10.15.1-cuda-12.9_1.0-1_amd64.deb}"
ENGINE_SERVICE="auto" # auto | engine | engine-trt

if [ "${1:-}" = "--trt" ]; then
  ENGINE_SERVICE="engine-trt"
elif [ "${1:-}" = "--onnx" ]; then
  ENGINE_SERVICE="engine"
fi

can_use_nvidia_gpu() {
  if ! command -v nvidia-smi >/dev/null 2>&1; then
    return 1
  fi
  if ! nvidia-smi -L >/dev/null 2>&1; then
    return 1
  fi
  if ! docker info --format '{{json .Runtimes}}' 2>/dev/null | grep -q '"nvidia"'; then
    return 1
  fi
  return 0
}

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"

mkdir -p "$MODELS_DIR"

# Start engine in attached mode (Ctrl+C to stop).
if [ "$ENGINE_SERVICE" = "auto" ]; then
  if can_use_nvidia_gpu && [ -f "$TENSORRT_REPO_DEB" ]; then
    ENGINE_SERVICE="engine-trt"
    echo "Auto mode: selecting TensorRT engine (GPU + TensorRT repo package detected)."
  else
    ENGINE_SERVICE="engine"
    if can_use_nvidia_gpu && [ ! -f "$TENSORRT_REPO_DEB" ]; then
      echo "Auto mode fallback: GPU detected but TensorRT repo package not found:"
      echo "  $TENSORRT_REPO_DEB"
      echo "Selecting ONNX engine."
    else
      echo "Auto mode: selecting ONNX engine."
    fi
  fi
fi

if [ "$ENGINE_SERVICE" = "engine-trt" ] && [ ! -f "$TENSORRT_REPO_DEB" ]; then
  echo "TensorRT Docker repo package not found:"
  echo "  $TENSORRT_REPO_DEB"
  echo "Set TENSORRT_REPO_DEB to the .deb path or copy the file into docker/engine/deps/."
  exit 1
fi

echo "Building and starting $ENGINE_SERVICE via docker compose on port 9000..."
docker compose -f "$COMPOSE_FILE" up --build "$ENGINE_SERVICE"
