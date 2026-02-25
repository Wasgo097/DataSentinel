#!/usr/bin/env bash
#set -euo pipefail

CONTAINERS="$(docker ps -q)"

if [ -z "$CONTAINERS" ]; then
  echo "No running containers."
  exit 0
fi

echo "Killing all running containers..."
docker kill $CONTAINERS
echo "All running containers were killed."
