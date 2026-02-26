#!/usr/bin/env bash
#set -euo pipefail

if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  echo "Do not source this script. Run: ./scripts/docker/runTrainerDocker.sh"
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

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"

mkdir -p "$MODELS_DIR"

# Usage:
# - ./scripts/docker/runTrainerDocker.sh            (auto: GPU if available, else CPU)
# - ./scripts/docker/runTrainerDocker.sh --gpu      (prefer GPU based on initEnv variables)
# - ./scripts/docker/runTrainerDocker.sh --cpu      (force CPU)
# - GPU=1 ./scripts/docker/runTrainerDocker.sh      (same as --gpu)
# - GPU=0 ./scripts/docker/runTrainerDocker.sh      (same as --cpu)
MODE="auto" # auto | gpu | cpu
if [ "${GPU:-}" = "1" ]; then
  MODE="gpu"
elif [ "${GPU:-}" = "0" ]; then
  MODE="cpu"
fi
if [ "${1:-}" = "--gpu" ]; then
  MODE="gpu"
elif [ "${1:-}" = "--cpu" ]; then
  MODE="cpu"
fi

TRAINER_SERVICE="$DATASENTINEL_TRAINER_SERVICE"
if [ "$MODE" = "cpu" ]; then
  TRAINER_SERVICE="trainer"
elif [ "$MODE" = "gpu" ]; then
  if [ "${DATASENTINEL_GPU_AVAILABLE:-0}" = "1" ]; then
    TRAINER_SERVICE="trainer-gpu"
  else
    TRAINER_SERVICE="trainer"
    echo "GPU mode fallback: DATASENTINEL_GPU_AVAILABLE=0, using CPU trainer."
  fi
else
  echo "Auto mode: using initEnv selection ($TRAINER_SERVICE)."
fi

echo "Building and running ${TRAINER_SERVICE} via docker compose..."
if [ "$TRAINER_SERVICE" = "trainer-gpu" ]; then
  DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" --profile gpu run --rm --build trainer-gpu
else
  DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" run --rm --build trainer
fi

echo "Trainer run completed."
