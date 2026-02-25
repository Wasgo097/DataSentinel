#!/usr/bin/env bash
#set -euo pipefail

# Resolve project root and compose file path.
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="$PROJECT_ROOT/docker/compose.yaml"

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"
# Stop and remove compose-managed containers and network.
echo "Stopping compose services..."

docker compose -f "$COMPOSE_FILE" down
