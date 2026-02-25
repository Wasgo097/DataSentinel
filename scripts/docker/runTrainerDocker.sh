#!/usr/bin/env bash
#set -euo pipefail

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
IMAGE_NAME="datasentinel-trainer:dev"
DOCKERFILE_PATH="$PROJECT_ROOT/docker/trainer/Dockerfile"
MODELS_DIR="$PROJECT_ROOT/models"

echo "Project root: $PROJECT_ROOT"
echo "Building trainer image: $IMAGE_NAME"
docker build -f "$DOCKERFILE_PATH" -t "$IMAGE_NAME" "$PROJECT_ROOT"

echo "Image build completed. Starting trainer container..."

mkdir -p "$MODELS_DIR"

echo "Running trainer container..."
docker run --rm \
  -v "$MODELS_DIR:/app/models" \
  "$IMAGE_NAME"

echo "Trainer run completed."
