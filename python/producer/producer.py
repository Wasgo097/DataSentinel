import os
import random
import socket
import sys
import time
from pathlib import Path

HOST = os.getenv("ENGINE_HOST", "127.0.0.1")
PORT = int(os.getenv("ENGINE_PORT", "9000"))
PROTOCOL = os.getenv("DATASENTINEL_PROTOCOL", "tcp").strip().lower()
TARGET = f"{HOST}:{PORT}"

# Model expects 8 float values
EXPECTED_INPUT_SIZE = 8

# Data generation settings (tuned to match trainer scale roughly).
# Normal samples default to [-1, 1]. Anomalies default to [-2, 2].
NORMAL_MIN = float(os.getenv("NORMAL_MIN", "-1.0"))
NORMAL_MAX = float(os.getenv("NORMAL_MAX", "1.0"))
ANOMALY_MIN = float(os.getenv("ANOMALY_MIN", "-2.0"))
ANOMALY_MAX = float(os.getenv("ANOMALY_MAX", "2.0"))

# Probability that the next message is an anomaly (0.0 - 1.0).
ANOMALY_RATE = float(os.getenv("ANOMALY_RATE", "0.1"))


def generate_data():
    """Generate a "normal" sample (max 3 decimal places)."""
    return [round(random.uniform(NORMAL_MIN, NORMAL_MAX), 3) for _ in range(EXPECTED_INPUT_SIZE)]


def generate_anomaly_data():
    """
    Generate an "anomaly" sample (max 3 decimal places).
    Ensures at least one value is outside the normal range.
    """
    data = [random.uniform(ANOMALY_MIN, ANOMALY_MAX) for _ in range(EXPECTED_INPUT_SIZE)]

    # Guarantee at least one out-of-normal-range feature so it's meaningfully different.
    if all(NORMAL_MIN <= v <= NORMAL_MAX for v in data):
        idx = random.randrange(EXPECTED_INPUT_SIZE)
        # Push one feature to the edge of anomaly range (positive or negative).
        data[idx] = ANOMALY_MAX if random.random() < 0.5 else ANOMALY_MIN

    return [round(v, 3) for v in data]


def generate_invalid_data():
    """Generate invalid data with wrong number of elements (4 instead of 8, max 3 decimal places)."""
    return [round(random.uniform(0.0, 100.0), 3) for _ in range(EXPECTED_INPUT_SIZE // 2)]


def next_payload(message_count):
    if message_count % 10 == 9:
        data = generate_invalid_data()
        print(f"[Producer] Sending INVALID data ({len(data)} elements): {data}")
        return data, 0

    if random.random() < ANOMALY_RATE:
        data = generate_anomaly_data()
        print(f"[Producer] Sending ANOMALY ({len(data)} elements): {data}")
    else:
        data = generate_data()
        print(f"[Producer] Sending ({len(data)} elements): {data}")

    return data, message_count + 1


def run_tcp():
    message_count = 0
    sock = None
    print(f"[Producer] Protocol: tcp, target: {TARGET}")

    while True:
        try:
            if sock is None:
                sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                sock.connect((HOST, PORT))
                print("[Producer] Connected to Engine.")

            data, message_count = next_payload(message_count)
            message = " ".join(f"{value:.3f}" for value in data) + "\n"
            sock.sendall(message.encode())

            response = sock.recv(4096)
            print("[Producer] Received:", response.decode().strip())
            time.sleep(1)

        except ConnectionRefusedError:
            print("[Producer] Engine not running. Retrying in 3 seconds...")
            if sock is not None:
                sock.close()
                sock = None
            time.sleep(3)

        except ConnectionResetError:
            print("[Producer] Connection lost. Reconnecting...")
            if sock is not None:
                sock.close()
                sock = None
            time.sleep(1)

        except Exception as e:
            print("[Producer] Error:", e)
            if sock is not None:
                sock.close()
                sock = None
            time.sleep(1)


def run_grpc():
    import grpc

    generated_dir = Path(__file__).resolve().parent / "generated"
    if str(generated_dir) not in sys.path:
        sys.path.insert(0, str(generated_dir))

    import inference_pb2
    import inference_pb2_grpc

    message_count = 0
    channel = None
    stub = None
    print(f"[Producer] Protocol: grpc, target: {TARGET}")

    while True:
        try:
            if stub is None:
                channel = grpc.insecure_channel(TARGET)
                grpc.channel_ready_future(channel).result(timeout=3.0)
                stub = inference_pb2_grpc.InferenceServiceStub(channel)
                print(f"[Producer] Connected to Engine gRPC at {TARGET}.")

            data, message_count = next_payload(message_count)
            request = inference_pb2.EvaluateRequest(values=data)
            response = stub.Evaluate(request, timeout=5.0)
            status_name = inference_pb2.EvaluateResponse.Status.Name(response.status)

            if response.status == inference_pb2.EvaluateResponse.ERROR:
                print(f"[Producer] Received ERROR: {response.message}")
            else:
                print(
                    f"[Producer] Received: status={status_name}, mse={response.mse:.6f}, message={response.message}"
                )

            time.sleep(1)

        except grpc.FutureTimeoutError:
            print(f"[Producer] Engine gRPC {TARGET} not ready. Retrying in 3 seconds...")
            stub = None
            if channel is not None:
                channel.close()
                channel = None
            time.sleep(3)

        except grpc.RpcError as e:
            print(f"[Producer] gRPC error: {e.code().name} - {e.details()}")
            stub = None
            if channel is not None:
                channel.close()
                channel = None
            time.sleep(1)

        except Exception as e:
            print("[Producer] Error:", e)
            stub = None
            if channel is not None:
                channel.close()
                channel = None
            time.sleep(1)


def main():
    if PROTOCOL == "tcp":
        run_tcp()
        return

    if PROTOCOL == "grpc":
        run_grpc()
        return

    raise ValueError(f"Unsupported DATASENTINEL_PROTOCOL={PROTOCOL}. Supported values: tcp, grpc")


if __name__ == "__main__":
    main()
