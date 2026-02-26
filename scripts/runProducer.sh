#!/usr/bin/env bash
#set -e

# Session guard must run first.
if [ "${DATASENTINEL_ENV_INITIALIZED:-0}" != "1" ]; then
    echo "Environment not initialized."
    echo "Run in current shell first: source ./scripts/initEnv.sh"
    exit 1
fi

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="$ROOT_DIR/venv"
PRODUCER="$ROOT_DIR/python/producer/producer.py"
REQUIREMENTS="$ROOT_DIR/python/producer/requirements.txt"
PROTO_DIR="$ROOT_DIR/proto"
PROTO_FILE="$PROTO_DIR/inference.proto"
GENERATED_DIR="$ROOT_DIR/python/producer/generated"
PROTOCOL="${DATASENTINEL_PROTOCOL:-tcp}"

# 1️⃣ Create venv if missing
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating virtual environment..."
    python3 -m venv "$VENV_DIR"
fi

# 2️⃣ Activate venv
echo "Activating virtual environment..."
# shellcheck source=/dev/null
source "$VENV_DIR/bin/activate"

# 3️⃣ Install deps, generate stubs and run producer
if [ "$PROTOCOL" = "grpc" ]; then
  echo "Installing producer gRPC dependencies..."
  python -m pip install -r "$REQUIREMENTS"

  echo "Generating Python gRPC stubs..."
  mkdir -p "$GENERATED_DIR"
  touch "$GENERATED_DIR/__init__.py"
  python -m grpc_tools.protoc \
    -I "$PROTO_DIR" \
    --python_out="$GENERATED_DIR" \
    --grpc_python_out="$GENERATED_DIR" \
    "$PROTO_FILE"
else
  echo "Producer protocol: $PROTOCOL (no gRPC dependencies required)"
fi

echo "Running producer..."
python "$PRODUCER"

# 4️⃣ Deactivate
deactivate
echo "Producer finished."
