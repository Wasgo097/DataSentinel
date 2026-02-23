## DataSentinel – Project Roadmap

Below is the evolution of the project from prototype to production-ready versions.

---

### **v1 – Basic Prototype** - done
**Goal:** Test data flow and model training.

- **Python Trainer**
  - Autoencoder (MLP) for anomaly detection
  - Generates random data
  - Exports model to ONNX (`models/model.onnx`) + `config.json`
  - **Does not send data** — only trains the model
- **Python Producer**
  - Generates data (random for now)
  - Sends data to C++ Receiver via TCP
  - Receives results (number of detected anomalies)
- **C++ Receiver**
  - Receives data via TCP (Boost.Asio)
  - Validates data using ONNX model
  - Computes number of anomalies and sends results back
- **Startup Script**
  - `scripts/train.sh` creates venv, installs packages, runs trainer, deactivates venv
- **Output**
  - `models/model.onnx` → trained model
  - `models/config.json` → training configuration

---

### **v2 – Docker**
**Goal:** Isolate services in containers.

- Added folder: `docker/`
- Each service (`trainer`, `runtime`, `client`) has its own Dockerfile
- Docker maps:
  - `models/` folder
  - `config.json` file
  - TCP ports between containers
- File structure of the repo remains the same

---

### **v3 – TensorRT**
**Goal:** Speed up inference in C++.

- Added folder: `cpp/inference/` → TensorRT backend
- Added folder: `cpp/third_party/tensorrt/`
- New file: `models/trt_engine.trt`
- Rest of the repo stays unchanged

---

### **v4 – gRPC**
**Goal:** Replace TCP with gRPC for microservices.

- Added folder: `proto/`
- C++ and Python use the same `.proto` file
- TCP replaced with gRPC
- Rest of the repo and classes remain the same

---

### 🔹 Additional Notes
- Models and config always remain in `models/`
- Main repo structure (`python/`, `cpp/`, `scripts/`, `README.md`) stays consistent across versions
- Scripts in v1 run locally; v2+ support Docker and service isolation