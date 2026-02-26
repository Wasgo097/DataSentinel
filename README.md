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

### v3 - Data quality + GPU training (completed)
- Done: Train model on labeled dataset instead of synthetic random-only input
- Done: Add GPU-accelerated training path for the trainer

### v4 - Performance backend (completed)
- Done: Add TensorRT-based inference path for C++ runtime
- Done: Keep ONNX Runtime path as baseline and fallback

### v5 - Service protocol (planned)
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
- NVIDIA Container Toolkit (only for GPU training)
- TensorRT local repo `.deb` file for TensorRT Docker image build (GPU engine only):
  place it at `docker/engine/deps/` or set `TENSORRT_REPO_DEB=/abs/path/to/file.deb`

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

## Dataset format (V3)

Trainer expects labeled training data at `python/trainer/data/train.csv`.

- Columns: first 8 columns are numeric features, last column is `label`
- Convention: `label=0` means normal; training uses only normal rows

CSV header:
- `f1,f2,f3,f4,f5,f6,f7,f8,label`

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

`buildEngine.sh` auto-detects local TensorRT installation and sets:
- `-DDS_ENABLE_TENSORRT=ON` when TensorRT is available
- `-DDS_ENABLE_TENSORRT=OFF` otherwise

Manual override is still possible, for example:
`./scripts/buildEngine.sh -DDS_ENABLE_TENSORRT=ON`

Run engine:

```bash
./scripts/runEngine.sh
```

`runEngine.sh` auto-selects backend:
- `tensorrt` if binary was built with TensorRT and runtime libraries are present
- `onnx` otherwise

You can still force backend manually:
`DATASENTINEL_BACKEND=onnx ./scripts/runEngine.sh`

Run producer (in another terminal):

```bash
./scripts/runProducer.sh
```

### Docker mode (Compose-based)

Prefer project scripts for day-to-day usage.

Prepare TensorRT package for Docker (required only for `engine-trt`):

```bash
mkdir -p docker/engine/deps
cp /path/to/nv-tensorrt-local-repo-ubuntu2404-*.deb docker/engine/deps/
```

Run full stack (preferred):
- `./scripts/docker/upDockerStack.sh`
- `./scripts/docker/downDockerStack.sh`

What `upDockerStack.sh` does:
1. Runs `trainer` as a one-off job to generate `models/model.onnx` and `models/config.json`
2. Starts engine + producer:
   - `engine-trt` when NVIDIA GPU is available and TensorRT `.deb` is present
   - otherwise `engine` (ONNX)

Run single Docker services:
- `./scripts/docker/runTrainerDocker.sh` - build/run one-off trainer
- `./scripts/docker/runEngineDocker.sh` - auto-select engine:
  - `engine-trt` when GPU + TensorRT `.deb` are detected
  - otherwise `engine` (ONNX)
- `./scripts/docker/runEngineDocker.sh --trt` - build/start TensorRT engine (requires GPU + TensorRT `.deb`)
- `./scripts/docker/runEngineDocker.sh --onnx` - force ONNX engine
- `./scripts/docker/runProducerDocker.sh` - build/run producer client
  - for TensorRT runtime target: `ENGINE_HOST=engine-trt ./scripts/docker/runProducerDocker.sh`

Minimal direct compose commands:

```bash
docker compose -f docker/compose.yaml run --rm --build trainer
docker compose -f docker/compose.yaml up --build engine producer
TENSORRT_REPO_DEB=/abs/path/to/nv-tensorrt-local-repo-ubuntu2404-*.deb docker compose -f docker/compose.yaml up --build engine-trt producer
docker compose -f docker/compose.yaml down
```

GPU trainer (optional):

```bash
DS_UID="$(id -u)" DS_GID="$(id -g)" docker compose -f docker/compose.yaml --profile gpu run --rm --build trainer-gpu
```

## Environment variables

- `DATASENTINEL_BACKEND`
  Engine backend selector. Supported values: `onnx`, `tensorrt` (aliases: `trt`, `tensor`).
  Default: `onnx`.
- `TENSORRT_REPO_DEB`
  Path to TensorRT local repo `.deb` used to build `engine-trt` Docker image.
  Default: `docker/engine/deps/nv-tensorrt-local-repo-ubuntu2404-10.15.1-cuda-12.9_1.0-1_amd64.deb`
- `ENGINE_HOST`
  Producer target host. Default in Docker Compose: `engine`.
- `ENGINE_PORT`
  Producer target port. Default: `9000`.

## Engine backend build options (local, non-Docker)

Default local build (auto-detection):

```bash
./scripts/buildEngine.sh
```

`buildEngine.sh` checks local TensorRT installation and chooses:
- `-DDS_ENABLE_TENSORRT=ON` when TensorRT is available
- `-DDS_ENABLE_TENSORRT=OFF` otherwise

Force ONNX-only build:

```bash
./scripts/buildEngine.sh -DDS_ENABLE_TENSORRT=OFF
```

Force TensorRT-enabled build:

```bash
./scripts/buildEngine.sh -DDS_ENABLE_TENSORRT=ON
```

Run with selected backend:

```bash
DATASENTINEL_BACKEND=onnx ./cpp/Engine/build/DataSentinelReceiver
DATASENTINEL_BACKEND=tensorrt ./cpp/Engine/build/DataSentinelReceiver
```

Run using helper script with auto backend selection:

```bash
./scripts/runEngine.sh
```

`runEngine.sh` chooses backend automatically:
- `tensorrt` if binary was built with `DS_ENABLE_TENSORRT=ON` and TensorRT runtime libs are present
- `onnx` otherwise (including ONNX-only build)

When TensorRT backend starts, engine cache file is kept next to ONNX model in `models/`:
- input ONNX: `models/model.onnx`
- TensorRT engine cache: `models/model.engine`

If `models/model.engine` does not exist, engine tries to build it automatically at startup.

If TensorRT backend is selected but binary was built without TensorRT support,
engine exits with a clear error and asks to rebuild with `-DDS_ENABLE_TENSORRT=ON`.

## Output artifacts

Trainer produces:
- `models/model.onnx`
- `models/config.json`

These files are consumed by the C++ engine.

## Implementation TODOs (outside roadmap)

Cross-cutting:
- Add a small labeled `test.csv` and a simple evaluation script (TP/FP/FN) for threshold sanity checks
- Add feature normalization (mean/std) computed in trainer and persisted in `models/config.json`, then apply it in engine/producer
- Add a quick smoke test (1 epoch on tiny CSV) verifying that `model.onnx` and `config.json` are produced
- Unify GPU Docker base layers for `docker/trainer/Dockerfile.gpu` and `docker/engine/Dockerfile.trt` (shared CUDA/Ubuntu base image) to improve layer cache reuse and reduce repeated image downloads
- Optimize `.dockerignore` to reduce Docker build context size (exclude `venv/`, temporary artifacts, and other non-build assets)

Trainer:
- Add `val.csv` split and use it to compute threshold (e.g., percentile of reconstruction MSE) instead of `mean + 3*std`
- Save basic training metadata to `models/` (seed, epochs, loss curve summary)

Engine:
- Validate model IO: ensure output length equals `input_dim` and fail fast with clear error
- Improve protocol responses (include numeric MSE in debug mode)

Producer:
- Support a deterministic seed for reproducible runs (env var)
- Optional mode to replay samples from a CSV instead of generating random data


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
