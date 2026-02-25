# DataSentinel

DataSentinel is an anomaly detection prototype built from three parts:
- Python trainer that trains an autoencoder and exports ONNX artifacts
- C++ engine that runs inference and serves a TCP endpoint
- Python producer that sends sample data to the engine

## Roadmap

### v1 - Local prototype (completed)
- Done: Python trainer trains an autoencoder
- Done: Trainer exports `models/model.onnx` and `models/config.json`
- Done: C++ engine loads model/config and serves TCP on port `9000`
- Done: Python producer sends sample data to engine and prints responses

### v2 - Dockerization (completed)
- Done: trainer Docker image and run script
- Done: engine Docker image and run script
- Done: producer Docker image and run script
- Done: `docker compose` stack for end-to-end startup

### v3 - Performance backend (planned)
- ToDo: Add TensorRT-based inference path for C++ runtime
- ToDo: Keep ONNX Runtime path as baseline and fallback

### v4 - Service protocol (planned)
- ToDo: Add shared `.proto` contracts for C++ and Python services
- ToDo: Replace raw TCP communication with gRPC

## Local prerequisites

- Linux environment
- Python 3
- CMake
- C++ compiler with C++20 support
- Boost (system)
- ONNX Runtime (for C++ engine)

## Docker prerequisites

- Docker Engine
- Docker Compose plugin (`docker compose`)

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

1. Train model in Python and export artifacts to `models/`:
   - `model.onnx`
   - `config.json`
2. Start C++ engine (reads `models/`, listens on `TCP :9000`)
3. Start Python producer (connects to engine and sends sample vectors)

## Run modes

### Local mode (without Docker)

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

### Docker mode (Compose-based)

Prefer project scripts for day-to-day usage.

Run full stack (preferred):
- `./scripts/docker/upDockerStack.sh`
- `./scripts/docker/downDockerStack.sh`

What `upDockerStack.sh` does:
1. Runs `trainer` as a one-off job to generate `models/model.onnx` and `models/config.json`
2. Starts `engine` and `producer` services

Run single Docker services:
- `./scripts/docker/runTrainerDocker.sh` - build/run one-off trainer
- `./scripts/docker/runEngineDocker.sh` - build/start engine on port `9000`
- `./scripts/docker/runProducerDocker.sh` - build/run producer client

Minimal direct compose commands:

```bash
docker compose -f docker/compose.yaml run --rm --build trainer
docker compose -f docker/compose.yaml up --build engine producer
docker compose -f docker/compose.yaml down
```

## Environment variables

- `ENGINE_HOST`
  Producer target host. Default in Docker Compose: `engine`.
- `ENGINE_PORT`
  Producer target port. Default: `9000`.

## Output artifacts

Trainer produces:
- `models/model.onnx`
- `models/config.json`

These files are consumed by the C++ engine.


## Troubleshooting

- `Bind for :::9000 failed: port is already allocated`
  Another process/container uses port `9000`. Stop conflicting container (`docker ps`, then `docker stop <id>`) or use a different host port mapping.

- Engine exits because model files are missing
  Run trainer first (`./scripts/docker/runTrainerDocker.sh` or `./scripts/runTrainer.sh`) and verify `models/model.onnx` + `models/config.json` exist.

- Need custom producer target host/port
  Use `ENGINE_HOST` and `ENGINE_PORT`, for example:
  `ENGINE_HOST=127.0.0.1 ENGINE_PORT=9000 ./scripts/docker/runProducerDocker.sh`.

- Need local non-Docker flow
  Use `./scripts/runTrainer.sh`, `./scripts/buildEngine.sh`, `./scripts/runEngine.sh`, `./scripts/runProducer.sh`.

- `Ctrl+C` does not clean up everything from Compose runs
  Run `./scripts/docker/downDockerStack.sh`.

- Need to force-stop all running containers
  Run `./scripts/docker/killAllContainers.sh`.
