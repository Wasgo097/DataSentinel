# Build stage: compile C++ engine and prepare ONNX Runtime files.
FROM ubuntu:24.04 AS builder

# ONNX Runtime version used during build.
ARG ONNXRUNTIME_VERSION=1.24.2
# Disable interactive apt prompts in non-interactive container builds.
ENV DEBIAN_FRONTEND=noninteractive

# Build workspace inside the container.
WORKDIR /src

# Install build toolchain and required dev dependencies.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        build-essential \
        cmake \
        libboost-system-dev \
        ca-certificates \
        wget \
        tar \
    && rm -rf /var/lib/apt/lists/*

# Download prebuilt ONNX Runtime package and unpack to /opt/onnxruntime.
RUN wget -q "https://github.com/microsoft/onnxruntime/releases/download/v${ONNXRUNTIME_VERSION}/onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz" \
    && tar -xzf "onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}.tgz" \
    && mv "onnxruntime-linux-x64-${ONNXRUNTIME_VERSION}" /opt/onnxruntime \
    # Some ONNX Runtime packages ship CMake targets pointing to lib64 while binaries are in lib.
    # Create lib64 symlinks to keep both layouts compatible.
    && mkdir -p /opt/onnxruntime/lib64 \
    && for f in /opt/onnxruntime/lib/libonnxruntime.so*; do ln -sf "$f" /opt/onnxruntime/lib64/; done \
    # Some packages also expect include/onnxruntime while headers are in include/.
    # Create a compatibility symlink when needed.
    && if [ ! -d /opt/onnxruntime/include/onnxruntime ]; then ln -s /opt/onnxruntime/include /opt/onnxruntime/include/onnxruntime; fi

# Copy engine source code into the build stage.
COPY cpp/Engine /src/cpp/Engine

# Locate ONNX Runtime CMake config and build the engine binary.
RUN ONNX_CMAKE_DIR="$(find /opt/onnxruntime -type d -path '*/cmake/onnxruntime' | head -n1)" \
    && test -n "$ONNX_CMAKE_DIR" \
    && cmake -S /src/cpp/Engine -B /src/cpp/Engine/build -Donnxruntime_DIR="$ONNX_CMAKE_DIR" \
    && cmake --build /src/cpp/Engine/build -j"$(nproc)"

# Runtime stage: lightweight image used only to run the compiled binary.
FROM ubuntu:24.04 AS runtime

# Disable interactive apt prompts in non-interactive container builds.
ENV DEBIAN_FRONTEND=noninteractive
# Runtime working directory.
WORKDIR /app

# Install runtime libraries required by the executable.
RUN apt-get update \
    && apt-get install -y --no-install-recommends \
        libboost-system-dev \
        libstdc++6 \
        ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# Copy ONNX Runtime shared libraries and the compiled engine binary.
COPY --from=builder /opt/onnxruntime /opt/onnxruntime
COPY --from=builder /src/cpp/Engine/build/DataSentinelReceiver /app/DataSentinelReceiver

# Make ONNX Runtime shared libraries visible to the dynamic linker.
ENV LD_LIBRARY_PATH="/opt/onnxruntime/lib:/opt/onnxruntime/lib64"

# Engine TCP port.
EXPOSE 9000

# Default command to start the engine server.
CMD ["/app/DataSentinelReceiver"]
