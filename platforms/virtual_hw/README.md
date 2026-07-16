# Virtual hardware demonstration

Run Vision Pilot end-to-end **without** a physical camera, RADAR, or MCU.

## What is virtualized

| Real hardware | Virtual stand-in |
|---------------|------------------|
| Front camera | Synthetic MP4 (`generate_demo_assets.py`) |
| Wheel-speed / CAN speed | `virtual_speed.txt` (+ optional TCP `SPEED`) |
| Steering / accel actuators | `virtual_ecu.py` receiving `CMD` on TCP `:59000` |
| ECU / Arduino / ESP32 / Zephyr | Same Python ECU (protocol-compatible) |

## Quick start

```powershell
cd platforms/virtual_hw
.\run_demo.ps1
```

Or manually:

```bash
cd VisionPilot/docker
docker compose -f docker-compose.demo.yml run --rm --no-deps demo-assets
docker compose -f docker-compose.demo.yml up --abort-on-container-exit --exit-code-from visionpilot-demo visionpilot-demo virtual-ecu
```

Watch logs for:

- `[INFO] plan: tyre=... accel=...` from VisionPilot
- `[virtual-ecu] actuator CMD ...` from the virtual MCU
- `[Viz] Headless mode` (no real display required)

## Generate assets only

```bash
docker run --rm -v "%CD%/../../demo_data:/data/demo" -v "%CD%/../../platforms/virtual_hw:/opt/virtual_hw:ro" \
  --entrypoint python3 visionpilot:cpu \
  /opt/virtual_hw/generate_demo_assets.py --out-dir /data/demo
```

## Protocol

See [`../protocol/vp_vehicle_gateway.md`](../protocol/vp_vehicle_gateway.md).
