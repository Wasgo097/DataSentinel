#!/usr/bin/env bash
set -e

# ============================
# Paths
# ============================
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="$ROOT_DIR/venv"
TRAINER="$ROOT_DIR/python/trainer/train.py"

# ============================
# Check venv
# ============================
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating virtual environment..."
    python3 -m venv "$VENV_DIR"
fi

# Activate venv
echo "Activating virtual environment..."
source "$VENV_DIR/bin/activate"

# ============================
# Install minimum packages
# ============================
echo "Installing minimum packages (numpy, torch, onnx, onnxscript)..."
pip install --upgrade pip
pip install numpy torch onnx onnxscript

# ============================
# Run trainer
# ============================
echo "Running trainer..."
python "$TRAINER"
deactivate
echo "Trainer finished successfully."
echo "Virtual environment deactivated."