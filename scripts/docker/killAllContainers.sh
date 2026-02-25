#!/usr/bin/env bash
#set -euo pipefail

if [ "${BASH_SOURCE[0]}" != "$0" ]; then
  echo "Do not source this script. Run: ./scripts/docker/killAllContainers.sh"
  return 1
fi

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
