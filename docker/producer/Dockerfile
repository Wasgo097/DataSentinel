# Base image with Python runtime for the producer client.
FROM python:3.11-slim

# Python runtime settings:
# - do not generate .pyc bytecode files
# - flush logs immediately (useful in containers)
ENV PYTHONDONTWRITEBYTECODE=1 \
    PYTHONUNBUFFERED=1

# Working directory inside the container.
WORKDIR /app

# Copy producer source code.
COPY python/producer/producer.py /app/python/producer/producer.py

# Default command starts producer loop.
CMD ["python", "/app/python/producer/producer.py"]
