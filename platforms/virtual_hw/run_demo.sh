#!/usr/bin/env bash
# One-shot virtual-hardware demonstration (no physical camera / MCU).
set -euo pipefail
cd "$(dirname "$0")/../../VisionPilot/docker"

echo "==> Generating synthetic camera + speed assets"
docker compose -f docker-compose.demo.yml run --rm --no-deps demo-assets

echo "==> Starting VisionPilot + virtual ECU"
docker compose -f docker-compose.demo.yml up --abort-on-container-exit --exit-code-from visionpilot-demo visionpilot-demo virtual-ecu
