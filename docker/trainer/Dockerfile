# Base image with Python 3.11 on a lightweight Debian slim variant.
FROM python:3.11-slim

# Python runtime settings:
# - do not generate .pyc bytecode files
# - flush logs immediately (useful in containers)
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

# Working directory inside the container.
WORKDIR /app

# Install a minimal system library required by packages such as NumPy/PyTorch.
# Clean apt cache at the end to keep the image smaller.
RUN apt-get update \
    && apt-get install -y --no-install-recommends libgomp1 \
    && rm -rf /var/lib/apt/lists/*

# Copy trainer dependencies list and install them with pip.
COPY python/trainer/requirements.txt /tmp/requirements.txt
RUN pip install --no-cache-dir --upgrade pip \
    && pip install --no-cache-dir -r /tmp/requirements.txt

# Copy trainer source code into the container.
COPY python/trainer/train.py /app/python/trainer/train.py
# Create output directory for model artifacts (model.onnx and config.json).
RUN mkdir -p /app/models

# Default command: start training when the container runs.
CMD ["python", "/app/python/trainer/train.py"]
