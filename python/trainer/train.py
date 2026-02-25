import os
import json
import numpy as np
import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import DataLoader, TensorDataset

from dataset import load_labeled_csv


# =========================
# CONFIG
# =========================

INPUT_DIM = 8
LATENT_DIM = 3
NORMAL_LABEL = 0.0
BATCH_SIZE = 64
EPOCHS = 40
LEARNING_RATE = 1e-3
ROOT_DIR = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
MODEL_DIR = os.path.join(ROOT_DIR, "models")
MODEL_PATH = os.path.join(MODEL_DIR, "model.onnx")
CONFIG_PATH = os.path.join(MODEL_DIR, "config.json")
DATA_DIR = os.path.join(os.path.dirname(__file__), "data")
TRAIN_CSV_PATH = os.path.join(DATA_DIR, "train.csv")

os.makedirs(MODEL_DIR, exist_ok=True)


# =========================
# MODEL
# =========================

class Autoencoder(nn.Module):
    def __init__(self, input_dim=8, latent_dim=3):
        super(Autoencoder, self).__init__()

        self.encoder = nn.Sequential(
            nn.Linear(input_dim, 6),
            nn.ReLU(),
            nn.Linear(6, latent_dim),
            nn.ReLU()
        )

        self.decoder = nn.Sequential(
            nn.Linear(latent_dim, 6),
            nn.ReLU(),
            nn.Linear(6, input_dim)
        )

    def forward(self, x):
        z = self.encoder(x)
        x_hat = self.decoder(z)
        return x_hat


# =========================
# TRAINING
# =========================

def train(data: np.ndarray):
    print("Training model...")
    dataset = TensorDataset(torch.from_numpy(data))
    dataloader = DataLoader(dataset, batch_size=BATCH_SIZE, shuffle=True)

    model = Autoencoder(INPUT_DIM, LATENT_DIM)
    optimizer = optim.Adam(model.parameters(), lr=LEARNING_RATE)
    criterion = nn.MSELoss()

    print("Starting training...")

    for epoch in range(EPOCHS):
        epoch_loss = 0.0

        for batch in dataloader:
            x = batch[0]

            optimizer.zero_grad()
            x_hat = model(x)
            loss = criterion(x_hat, x)
            loss.backward()
            optimizer.step()

            epoch_loss += loss.item()

        print(f"Epoch [{epoch+1}/{EPOCHS}] Loss: {epoch_loss/len(dataloader):.6f}")

    print("Training complete.")
    return model


# =========================
# THRESHOLD CALCULATION
# =========================

def calculate_threshold(model, data):
    model.eval()
    with torch.no_grad():
        x = torch.from_numpy(data)
        x_hat = model(x)
        reconstruction_error = torch.mean((x_hat - x) ** 2, dim=1)

        mean = reconstruction_error.mean().item()
        std = reconstruction_error.std().item()

        threshold = mean + 3 * std

    print(f"Reconstruction error mean: {mean:.6f}")
    print(f"Reconstruction error std:  {std:.6f}")
    print(f"Calculated threshold:      {threshold:.6f}")

    return threshold


# =========================
# EXPORT ONNX
# =========================

def export_onnx(model):
    model.eval()

    dummy_input = torch.randn(1, INPUT_DIM)

    torch.onnx.export(
        model,
        dummy_input,
        MODEL_PATH,
        input_names=["input"],
        output_names=["output"],
        dynamic_axes={
            "input": {0: "batch_size"},
            "output": {0: "batch_size"}
        },
        opset_version=18
    )

    print(f"Model exported to {MODEL_PATH}")


# =========================
# SAVE CONFIG
# =========================

def save_config(threshold):
    config = {
        "input_dim": INPUT_DIM,
        "output_dim": INPUT_DIM,
        "latent_dim": LATENT_DIM,
        "threshold": threshold,
    }

    with open(CONFIG_PATH, "w") as f:
        json.dump(config, f, indent=4)

    print(f"Config saved to {CONFIG_PATH}")


# =========================
# MAIN
# =========================

def main():
    os.makedirs(MODEL_DIR, exist_ok=True)
    os.makedirs(DATA_DIR, exist_ok=True)

    print(f"Loading labeled dataset from: {TRAIN_CSV_PATH}")
    result = load_labeled_csv(TRAIN_CSV_PATH, input_dim=INPUT_DIM, normal_label=NORMAL_LABEL)
    data = result.data
    print(f"Loaded normal rows: {data.shape[0]} (dropped: {result.dropped_rows})")

    model = train(data)
    threshold = calculate_threshold(model, data)
    export_onnx(model)
    save_config(threshold)

    print("Trainer finished successfully.")


if __name__ == "__main__":
    main()
