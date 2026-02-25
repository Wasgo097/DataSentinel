# DataSentinel

DataSentinel is an anomaly detection prototype built from three parts:
- Python trainer that trains an autoencoder and exports ONNX artifacts
- C++ engine that runs inference and serves a TCP endpoint
- Python producer that sends sample data to the engine

## Current status

- Local prototype flow is implemented and working
- Trainer Dockerization is implemented and working
- Dockerization of engine and producer is not done yet

## Roadmap

### v1 - Local prototype (done)
- Python trainer trains an autoencoder
- Trainer exports `models/model.onnx` and `models/config.json`
- C++ engine loads model/config and serves TCP on port `9000`
- Python producer sends sample data to engine and prints responses

### v2 - Dockerization (in progress)
- Done: trainer Docker image and run script
- Next: dockerize C++ engine
- Next: dockerize Python producer
- Next: add `docker-compose` for end-to-end startup

### v3 - Performance backend (planned)
- Add TensorRT-based inference path for C++ runtime
- Keep ONNX Runtime path as baseline and fallback

### v4 - Service protocol (planned)
- Replace raw TCP communication with gRPC
- Add shared `.proto` contracts for C++ and Python services

## Repository layout

```text
python/
  trainer/
  producer/
cpp/
  Engine/
scripts/
models/
```

## Runtime flow

1. Train model in Python (`python/trainer/train.py`)
2. Export artifacts to `models/`:
   - `model.onnx`
   - `config.json`
3. Start C++ engine (reads `models/`, listens on `TCP :9000`)
4. Start Python producer (connects to engine and sends sample vectors)

## Prerequisites (local run)

- Linux environment
- Python 3
- CMake
- C++ compiler with C++20 support
- Boost (system)
- ONNX Runtime (for C++ engine)

## Run locally (without Docker)

Run trainer:

```bash
./scripts/runTrainer.sh
```

Build C++ engine:

```bash
./scripts/buildEngine.sh
```

Run engine:

```bash
./scripts/runEngine.sh
```

Run producer (in another terminal):

```bash
./scripts/runProducer.sh
```

## Trainer in Docker (implemented)

Build and run trainer via script:

```bash
./scripts/docker/runTrainerDocker.sh
```

This script:
- builds image `datasentinel-trainer:dev`
- runs trainer container
- mounts host `models/` to `/app/models` in the container

## Output artifacts

Trainer produces:
- `models/model.onnx`
- `models/config.json`

These files are consumed by the C++ engine.

## Notes

- Producer currently sends valid vectors most of the time and invalid vectors periodically (for input validation testing)
- ONNX export uses opset 18 in current trainer configuration
