#!/usr/bin/env bash
#set -euo pipefail

if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  echo "Do not source this script. Run: ./scripts/docker/runProducerDocker.sh"
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
# Producer target can be overridden from environment.
ENGINE_HOST="${ENGINE_HOST:-${DATASENTINEL_ENGINE_HOST:-engine}}"
ENGINE_PORT="${ENGINE_PORT:-9000}"
DATASENTINEL_PROTOCOL="${DATASENTINEL_PROTOCOL:-tcp}"

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"
echo "Producer target: ${ENGINE_HOST}:${ENGINE_PORT}"
echo "Producer protocol: ${DATASENTINEL_PROTOCOL}"

if [ "$ENGINE_HOST" = "engine" ]; then
  # Default compose-network mode: allow producer dependency on engine service.
  echo "Running producer via docker compose (engine dependency enabled)..."
  ENGINE_HOST="$ENGINE_HOST" ENGINE_PORT="$ENGINE_PORT" DATASENTINEL_PROTOCOL="$DATASENTINEL_PROTOCOL" \
    docker compose -f "$COMPOSE_FILE" run --rm --build producer
else
  # External target mode: do not auto-start engine service.
  echo "Running producer via docker compose without engine dependency..."
  ENGINE_HOST="$ENGINE_HOST" ENGINE_PORT="$ENGINE_PORT" DATASENTINEL_PROTOCOL="$DATASENTINEL_PROTOCOL" \
    docker compose -f "$COMPOSE_FILE" run --rm --build --no-deps producer
fi
