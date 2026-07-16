# Deployment & platform matrix

## Host runtimes (full ADAS stack)

| Target | How |
|--------|-----|
| Linux x86_64 + NVIDIA | `VisionPilot/docker/./build.sh --gpu` then `./run.sh --gpu` |
| Linux x86_64 CPU / Windows Docker | `./build.sh --cpu` then `./run.sh --cpu --no-display` |
| Raspberry Pi 5 | `./build.sh --cpu --platform linux/arm64 --tag visionpilot:cpu-arm64` — see `platforms/raspberry_pi5/` |
| QNX SDP | Cross-compile with `platforms/qnx_sdp/cmake/qnx.toolchain.cmake` (SDP license required) |

Compose:

```bash
cd VisionPilot/docker
docker compose up -d --build visionpilot
docker compose --profile jenkins up -d jenkins   # http://localhost:8081
```

## MCU / RTOS (vehicle gateway only)

| Target | Path |
|--------|------|
| Arduino | `platforms/arduino/vision_pilot_gateway/` |
| ESP32 (ESP-IDF) | `platforms/esp32/vision_pilot_gateway/` |
| Zephyr STM32 / NXP / TI | `platforms/zephyr/apps/vp_gateway/` |

Protocol: `platforms/protocol/vp_vehicle_gateway.md` (TCP `:59000` or UART 115200).

## Cloud (Terraform)

| Provider | Module |
|----------|--------|
| Amazon Web Services | `infra/terraform/aws` |
| Google Cloud | `infra/terraform/gcp` |
| Microsoft Azure | `infra/terraform/azure` |

Jenkins pipeline: `infra/jenkins/Jenkinsfile`.

## Windows note

Native MSVC builds are not supported (`.so` ORT, V4L2, DEB packaging). Use Docker Desktop (WSL2 backend) as on this workstation.
