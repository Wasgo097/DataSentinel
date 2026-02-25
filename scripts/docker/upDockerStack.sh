#!/usr/bin/env bash
#set -euo pipefail

# Resolve project root and compose file path.
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="$PROJECT_ROOT/docker/compose.yaml"

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"

mkdir -p "$PROJECT_ROOT/models"

# Prepare model artifacts first.
echo "Running trainer one-off job..."
DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" run --rm --build trainer

# Start runtime services.
echo "Starting engine and producer..."
docker compose -f "$COMPOSE_FILE" up --build engine producer
