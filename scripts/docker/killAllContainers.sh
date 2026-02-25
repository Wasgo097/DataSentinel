#!/usr/bin/env bash
#set -euo pipefail

# Collect IDs of all currently running containers.
CONTAINERS="$(docker ps -q)"

if [ -z "$CONTAINERS" ]; then
  echo "No running containers."
  exit 0
fi

# Force-stop all running containers.
echo "Killing all running containers..."
docker kill $CONTAINERS
echo "All running containers were killed."
