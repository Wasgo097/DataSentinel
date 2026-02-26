#!/usr/bin/env bash

if [ "${BASH_SOURCE[0]}" = "$0" ]; then
  echo "Run this script in current shell:"
  echo "  source ./scripts/initEnv.sh"
  exit 1
fi

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TRT_BASE_DOCKERFILE="$PROJECT_ROOT/docker/engine/Dockerfile.trt-base"
TENSORRT_REPO_DEB_REL="docker/engine/deps/nv-tensorrt-local-repo-ubuntu2404-10.15.1-cuda-12.9_1.0-1_amd64.deb"
TENSORRT_REPO_DEB="$PROJECT_ROOT/$TENSORRT_REPO_DEB_REL"
TRT_DEVEL_IMAGE="${TRT_DEVEL_IMAGE:-datasentinel-trt-devel:dev}"
TRT_RUNTIME_IMAGE="${TRT_RUNTIME_IMAGE:-datasentinel-trt-runtime:dev}"

unset DATASENTINEL_ENV_INITIALIZED DATASENTINEL_GPU_AVAILABLE DATASENTINEL_TRT_DEB_AVAILABLE \
  DATASENTINEL_TRT_BASE_IMAGES_READY DATASENTINEL_TRT_AVAILABLE DATASENTINEL_TRAINER_SERVICE \
  DATASENTINEL_ENGINE_SERVICE DATASENTINEL_ENGINE_HOST

can_use_nvidia_gpu() {
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

build_missing_trt_base_images() {
  local need_devel=0
  local need_runtime=0
  if ! docker image inspect "$TRT_DEVEL_IMAGE" >/dev/null 2>&1; then
    need_devel=1
  fi
  if ! docker image inspect "$TRT_RUNTIME_IMAGE" >/dev/null 2>&1; then
    need_runtime=1
  fi

  echo "TensorRT base image check:"
  if [ "$need_devel" -eq 1 ]; then
    echo "  missing: $TRT_DEVEL_IMAGE"
  else
    echo "  present: $TRT_DEVEL_IMAGE"
  fi
  if [ "$need_runtime" -eq 1 ]; then
    echo "  missing: $TRT_RUNTIME_IMAGE"
  else
    echo "  present: $TRT_RUNTIME_IMAGE"
  fi

  if [ "$need_devel" -eq 1 ]; then
    echo "Building TensorRT base image: $TRT_DEVEL_IMAGE"
    docker build -f "$TRT_BASE_DOCKERFILE" \
      --target trt-devel \
      --build-arg TENSORRT_REPO_DEB="$TENSORRT_REPO_DEB_REL" \
      -t "$TRT_DEVEL_IMAGE" \
      "$PROJECT_ROOT" || return 1
  fi
  if [ "$need_runtime" -eq 1 ]; then
    echo "Building TensorRT base image: $TRT_RUNTIME_IMAGE"
    docker build -f "$TRT_BASE_DOCKERFILE" \
      --target trt-runtime \
      --build-arg TENSORRT_REPO_DEB="$TENSORRT_REPO_DEB_REL" \
      -t "$TRT_RUNTIME_IMAGE" \
      "$PROJECT_ROOT" || return 1
  fi
}

echo "Project root: $PROJECT_ROOT"
echo "TensorRT .deb required at: $TENSORRT_REPO_DEB"
echo "TRT devel image: $TRT_DEVEL_IMAGE"
echo "TRT runtime image: $TRT_RUNTIME_IMAGE"

DATASENTINEL_GPU_AVAILABLE=0
if can_use_nvidia_gpu; then
  DATASENTINEL_GPU_AVAILABLE=1
fi

DATASENTINEL_TRT_DEB_AVAILABLE=0
if [ -f "$TENSORRT_REPO_DEB" ]; then
  DATASENTINEL_TRT_DEB_AVAILABLE=1
fi

if [ ! -f "$TRT_BASE_DOCKERFILE" ]; then
  echo "Missing Dockerfile: $TRT_BASE_DOCKERFILE"
  return 1
fi

DATASENTINEL_TRT_BASE_IMAGES_READY=0
if [ "$DATASENTINEL_TRT_DEB_AVAILABLE" = "1" ]; then
  if build_missing_trt_base_images; then
    if docker image inspect "$TRT_DEVEL_IMAGE" >/dev/null 2>&1 \
      && docker image inspect "$TRT_RUNTIME_IMAGE" >/dev/null 2>&1; then
      DATASENTINEL_TRT_BASE_IMAGES_READY=1
    fi
  else
    echo "TensorRT base image build failed."
    return 1
  fi
else
  echo "TensorRT .deb not found; TensorRT will be disabled."
fi

DATASENTINEL_TRT_AVAILABLE=0
if [ "$DATASENTINEL_GPU_AVAILABLE" = "1" ] && [ "$DATASENTINEL_TRT_BASE_IMAGES_READY" = "1" ]; then
  DATASENTINEL_TRT_AVAILABLE=1
fi

DATASENTINEL_TRAINER_SERVICE="trainer"
if [ "$DATASENTINEL_GPU_AVAILABLE" = "1" ]; then
  DATASENTINEL_TRAINER_SERVICE="trainer-gpu"
fi

DATASENTINEL_ENGINE_SERVICE="engine"
DATASENTINEL_ENGINE_HOST="engine"
if [ "$DATASENTINEL_TRT_AVAILABLE" = "1" ]; then
  DATASENTINEL_ENGINE_SERVICE="engine-trt"
  DATASENTINEL_ENGINE_HOST="engine-trt"
fi

export TRT_DEVEL_IMAGE
export TRT_RUNTIME_IMAGE
export DATASENTINEL_ENV_INITIALIZED=1
export DATASENTINEL_PROJECT_ROOT="$PROJECT_ROOT"
export DATASENTINEL_GPU_AVAILABLE
export DATASENTINEL_TRT_DEB_AVAILABLE
export DATASENTINEL_TRT_BASE_IMAGES_READY
export DATASENTINEL_TRT_AVAILABLE
export DATASENTINEL_TRAINER_SERVICE
export DATASENTINEL_ENGINE_SERVICE
export DATASENTINEL_ENGINE_HOST

echo "Environment initialized:"
echo "  DATASENTINEL_GPU_AVAILABLE=$DATASENTINEL_GPU_AVAILABLE"
echo "  DATASENTINEL_TRT_DEB_AVAILABLE=$DATASENTINEL_TRT_DEB_AVAILABLE"
echo "  DATASENTINEL_TRT_BASE_IMAGES_READY=$DATASENTINEL_TRT_BASE_IMAGES_READY"
echo "  DATASENTINEL_TRT_AVAILABLE=$DATASENTINEL_TRT_AVAILABLE"
echo "  DATASENTINEL_TRAINER_SERVICE=$DATASENTINEL_TRAINER_SERVICE"
echo "  DATASENTINEL_ENGINE_SERVICE=$DATASENTINEL_ENGINE_SERVICE"
echo "  DATASENTINEL_ENGINE_HOST=$DATASENTINEL_ENGINE_HOST"
echo "Detected configuration:"
echo "  project_root=$DATASENTINEL_PROJECT_ROOT"
echo "  tensor_deb_path=$TENSORRT_REPO_DEB"
echo "  trt_devel_image=$TRT_DEVEL_IMAGE"
echo "  trt_runtime_image=$TRT_RUNTIME_IMAGE"
echo "  gpu_available=$DATASENTINEL_GPU_AVAILABLE"
echo "  trt_deb_available=$DATASENTINEL_TRT_DEB_AVAILABLE"
echo "  trt_base_images_ready=$DATASENTINEL_TRT_BASE_IMAGES_READY"
echo "  trt_available=$DATASENTINEL_TRT_AVAILABLE"
echo "  trainer_service=$DATASENTINEL_TRAINER_SERVICE"
echo "  engine_service=$DATASENTINEL_ENGINE_SERVICE"
echo "  engine_host=$DATASENTINEL_ENGINE_HOST"
