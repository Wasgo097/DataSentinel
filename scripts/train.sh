#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="$ROOT_DIR/venv"
TRAINER="$ROOT_DIR/python/trainer/train.py"

# 1️⃣ Create venv if missing
if [ ! -d "$VENV_DIR" ]; then
    echo "Creating virtual environment..."
    python3 -m venv "$VENV_DIR"
fi

# 2️⃣ Activate venv
echo "Activating virtual environment..."
# shellcheck source=/dev/null
source "$VENV_DIR/bin/activate"

# 3️⃣ Upgrade pip
echo "Upgrading pip..."
python -m pip install --upgrade pip

# 4️⃣ Install required packages (only if missing)
REQUIRED_PKG=("numpy" "torch" "onnx" "onnxscript")
for pkg in "${REQUIRED_PKG[@]}"; do
    if ! python -c "import $pkg" &> /dev/null; then
        echo "Installing $pkg..."
        pip install "$pkg"
    fi
done

# 5️⃣ Run trainer
echo "Running trainer..."
python "$TRAINER"

# 6️⃣ Deactivate
deactivate
echo "Trainer finished successfully."