#!/usr/bin/env bash
#set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_NAME="datasentinel-engine:dev"
DOCKERFILE_PATH="$PROJECT_ROOT/docker/engine/Dockerfile"
MODELS_DIR="$PROJECT_ROOT/models"

echo "Project root: $PROJECT_ROOT"
echo "Building engine image: $IMAGE_NAME"
docker build -f "$DOCKERFILE_PATH" -t "$IMAGE_NAME" "$PROJECT_ROOT"

echo "Image build completed. Starting engine container..."

mkdir -p "$MODELS_DIR"

echo "Running engine container on port 9000..."
docker run --rm \
  -p 9000:9000 \
  -v "$MODELS_DIR:/app/models:ro" \
  "$IMAGE_NAME"
