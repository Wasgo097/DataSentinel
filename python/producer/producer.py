import socket
import time
import random

HOST = "127.0.0.1"
PORT = 9000

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
    while True:
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.connect((HOST, PORT))

                # Send invalid data every 10 messages
                if message_count % 10 == 9:
                    data = generate_invalid_data()
                    print(f"[Producer] Sending INVALID data (4 elements): {data}")
                    message_count = 0  # reset count after sending invalid data
                else:
                    data = generate_data()
                    print(f"[Producer] Sending: {data}")
                
                # Format: space-separated float values with newline
                message = " ".join(f"{value:.3f}" for value in data) + "\n"
                s.sendall(message.encode())

                response = s.recv(4096)
                print("[Producer] Received:", response.decode().strip())
                
                message_count += 1

        except ConnectionRefusedError:
            print("[Producer] Engine not running...")
        
        except Exception as e:
            print("[Producer] Error:", e)

        time.sleep(1)


if __name__ == "__main__":
    main()