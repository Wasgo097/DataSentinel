#!/usr/bin/env bash
set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_NAME="datasentinel-producer:dev"
DOCKERFILE_PATH="$PROJECT_ROOT/docker/producer/Dockerfile"
ENGINE_HOST="${ENGINE_HOST:-host.docker.internal}"
ENGINE_PORT="${ENGINE_PORT:-9000}"

echo "Project root: $PROJECT_ROOT"
echo "Building producer image: $IMAGE_NAME"
docker build -f "$DOCKERFILE_PATH" -t "$IMAGE_NAME" "$PROJECT_ROOT"

echo "Image build completed. Starting producer container..."
echo "Producer target: ${ENGINE_HOST}:${ENGINE_PORT}"

docker run --rm \
  --add-host=host.docker.internal:host-gateway \
  -e ENGINE_HOST="$ENGINE_HOST" \
  -e ENGINE_PORT="$ENGINE_PORT" \
  "$IMAGE_NAME"
