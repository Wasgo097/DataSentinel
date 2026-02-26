#!/usr/bin/env bash
set -euo pipefail

if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  echo "Do not source this script. Run: ./scripts/docker/runEngineDocker.sh"
  return 1
fi

# Session guard must run first.
if [ "${DATASENTINEL_ENV_INITIALIZED:-0}" != "1" ]; then
  echo "Environment not initialized."
  echo "Run in current shell first: source ./scripts/initEnv.sh"
  exit 1
fi

# Resolve project root and compose file path.
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="$PROJECT_ROOT/docker/compose.yaml"
MODELS_DIR="$PROJECT_ROOT/models"
ENGINE_SERVICE="auto" # auto | engine | engine-trt

if [ "${1:-}" = "--trt" ]; then
  ENGINE_SERVICE="engine-trt"
elif [ "${1:-}" = "--onnx" ]; then
  ENGINE_SERVICE="engine"
fi

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"

mkdir -p "$MODELS_DIR"

# Start engine in attached mode (Ctrl+C to stop).
if [ "$ENGINE_SERVICE" = "auto" ]; then
  ENGINE_SERVICE="${DATASENTINEL_ENGINE_SERVICE:-engine}"
  echo "Auto mode: using initEnv selection ($ENGINE_SERVICE)."
fi

if [ "$ENGINE_SERVICE" = "engine-trt" ] && [ "${DATASENTINEL_TRT_AVAILABLE:-0}" != "1" ]; then
  echo "TensorRT runtime not available according to initEnv."
  echo "Run: source ./scripts/initEnv.sh"
  exit 1
fi

echo "Building and starting $ENGINE_SERVICE via docker compose on port 9000..."
docker compose -f "$COMPOSE_FILE" up --build "$ENGINE_SERVICE"
