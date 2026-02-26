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

# Run trainer as one-off compose job and rebuild image if needed.
TRAINER_SERVICE="${DATASENTINEL_TRAINER_SERVICE:-trainer}"
echo "Building and running ${TRAINER_SERVICE} via docker compose..."
if [ "$TRAINER_SERVICE" = "trainer-gpu" ]; then
  DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" --profile gpu run --rm --build trainer-gpu
else
  DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" run --rm --build trainer
fi

echo "Trainer run completed."
