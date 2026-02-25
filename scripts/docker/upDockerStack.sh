#!/usr/bin/env bash
#set -euo pipefail

if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  echo "Do not source this script. Run: ./scripts/docker/upDockerStack.sh"
  return 1
fi

# Resolve project root and compose file path.
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
COMPOSE_FILE="$PROJECT_ROOT/docker/compose.yaml"

echo "Project root: $PROJECT_ROOT"
echo "Compose file: $COMPOSE_FILE"

mkdir -p "$PROJECT_ROOT/models"

# Usage:
# - ./scripts/docker/upDockerStack.sh            (auto: GPU if available, else CPU)
# - ./scripts/docker/upDockerStack.sh --gpu      (prefer GPU, fallback to CPU unless FORCE_GPU=1)
# - ./scripts/docker/upDockerStack.sh --cpu      (force CPU)
# - GPU=1 ./scripts/docker/upDockerStack.sh      (same as --gpu)
# - GPU=0 ./scripts/docker/upDockerStack.sh      (same as --cpu)
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

can_use_nvidia_gpu() {
  # Best-effort check: NVIDIA GPU training requires host NVIDIA drivers + nvidia-container-toolkit.
  # This is intentionally conservative; on failure we fall back to CPU.
  if ! command -v nvidia-smi >/dev/null 2>&1; then
    return 1
  fi
  if ! nvidia-smi -L >/dev/null 2>&1; then
    return 1
  fi
  if ! docker info --format '{{json .Runtimes}}' 2>/dev/null | grep -q '"nvidia"'; then
    return 1
  fi
  return 0
}

# Prepare model artifacts first.
echo "Running trainer one-off job..."
if [ "$MODE" = "cpu" ]; then
  DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" run --rm --build trainer
elif [ "$MODE" = "gpu" ]; then
  echo "GPU mode requested: trainer-gpu (compose profile: gpu)"
  if [ "${FORCE_GPU:-0}" = "1" ] || can_use_nvidia_gpu; then
    DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" --profile gpu run --rm --build trainer-gpu
  else
    echo "GPU preflight failed (no NVIDIA GPU/toolkit detected). Falling back to CPU trainer."
    echo "To force GPU anyway, run: FORCE_GPU=1 ./scripts/docker/upDockerStack.sh --gpu"
    DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" run --rm --build trainer
  fi
else
  # auto
  if can_use_nvidia_gpu; then
    echo "Auto mode: NVIDIA GPU detected. Running trainer-gpu."
    DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" --profile gpu run --rm --build trainer-gpu
  else
    echo "Auto mode: no NVIDIA GPU detected. Running CPU trainer."
    DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f "$COMPOSE_FILE" run --rm --build trainer
  fi
fi

# Start runtime services.
echo "Starting engine and producer..."
docker compose -f "$COMPOSE_FILE" up --build engine producer
