#!/usr/bin/env bash
#set -euo pipefail

if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  echo "Do not source this script. Run: ./scripts/docker/runTrainerDocker.sh"
  return 1
fi

# Resolve project root and compose file path.
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="$PROJECT_ROOT/docker/compose.yaml"
MODELS_DIR="$PROJECT_ROOT/models"

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"

mkdir -p "$MODELS_DIR"

# Run trainer as one-off compose job and rebuild image if needed.
echo "Building and running trainer via docker compose..."
DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" run --rm --build trainer

echo "Trainer run completed."
