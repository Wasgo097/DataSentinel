import socket
import time
import random
import os

HOST = os.getenv("ENGINE_HOST", "127.0.0.1")
PORT = int(os.getenv("ENGINE_PORT", "9000"))

# Model expects 8 float values
EXPECTED_INPUT_SIZE = 8


def generate_data():
    """Generate 8 random float values as expected by the model (max 3 decimal places)"""
    return [round(random.uniform(0.0, 100.0), 3) for _ in range(EXPECTED_INPUT_SIZE)]


def generate_invalid_data():
    """Generate invalid data with wrong number of elements (4 instead of 8, max 3 decimal places)"""
    return [round(random.uniform(0.0, 100.0), 3) for _ in range(EXPECTED_INPUT_SIZE // 2)]


def main():
    message_count = 0
    s = None
    
    while True:
        try:
            # Connect only if not connected
            if s is None:
                s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                s.connect((HOST, PORT))
                print("[Producer] Connected to Engine.")

            # Send invalid data every 10 messages
            if message_count % 10 == 9:
                data = generate_invalid_data()
                print(f"[Producer] Sending INVALID data ({len(data)} elements): {data}")
                message_count = 0  # reset count after sending invalid data
            else:
                data = generate_data()
                print(f"[Producer] Sending ({len(data)} elements): {data}")
            
            # Format: space-separated float values with newline
            message = " ".join(f"{value:.3f}" for value in data) + "\n"
            s.sendall(message.encode())

            response = s.recv(4096)
            print("[Producer] Received:", response.decode().strip())
            
            message_count += 1
            time.sleep(1)

        except ConnectionRefusedError:
            print("[Producer] Engine not running. Retrying in 3 seconds...")
            if s is not None:
                s.close()
                s = None
            time.sleep(3)
        
        except ConnectionResetError:
            print("[Producer] Connection lost. Reconnecting...")
            if s is not None:
                s.close()
                s = None
            time.sleep(1)
        
        except Exception as e:
            print("[Producer] Error:", e)
            if s is not None:
                s.close()
                s = None
            time.sleep(1)


if __name__ == "__main__":
    main()
