#!/usr/bin/env bash
#set -euo pipefail

if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  echo "Do not source this script. Run: ./scripts/docker/runEngineDocker.sh"
  return 1
fi

# Resolve project root and compose file path.
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="$PROJECT_ROOT/docker/compose.yaml"
MODELS_DIR="$PROJECT_ROOT/models"

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"

mkdir -p "$MODELS_DIR"

# Start engine in attached mode (Ctrl+C to stop).
echo "Building and starting engine via docker compose on port 9000..."
docker compose -f "$COMPOSE_FILE" up --build engine
