# Base image with Python runtime for the producer client.
FROM python:3.11-slim

# Python runtime settings:
# - do not generate .pyc bytecode files
# - flush logs immediately (useful in containers)
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

# Working directory inside the container.
WORKDIR /app

# Copy proto and producer dependencies first to leverage Docker layer cache.
COPY proto/inference.proto /app/proto/inference.proto
COPY python/producer/requirements.txt /app/python/producer/requirements.txt

# Install runtime dependencies and generate Python gRPC stubs.
RUN pip install --no-cache-dir -r /app/python/producer/requirements.txt \
    && mkdir -p /app/python/producer/generated \
    && python -m grpc_tools.protoc \
        -I /app/proto \
        --python_out=/app/python/producer/generated \
        --grpc_python_out=/app/python/producer/generated \
        /app/proto/inference.proto \
    && touch /app/python/producer/generated/__init__.py

# Copy producer source code.
COPY python/producer/producer.py /app/python/producer/producer.py

# Default command starts producer loop.
CMD ["python", "/app/python/producer/producer.py"]
