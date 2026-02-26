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

# 1️⃣ Create venv if missing
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating virtual environment..."
    python3 -m venv "$VENV_DIR"
fi

# 2️⃣ Activate venv
echo "Activating virtual environment..."
# shellcheck source=/dev/null
source "$VENV_DIR/bin/activate"

# 3️⃣ Run producer (uses only built-in modules: socket, time, random)
echo "Running producer..."
python "$PRODUCER"

# 4️⃣ Deactivate
deactivate
echo "Producer finished."
