# Virtual hardware demonstration

Run Vision Pilot end-to-end **without** a physical camera, RADAR, or MCU.

## What is virtualized

| Real hardware | Virtual stand-in |
|---------------|------------------|
| Front camera | Synthetic MP4 (`generate_demo_assets.py`) |
| Wheel-speed / CAN speed | `virtual_speed.txt` (+ optional TCP `SPEED`) |
| Steering / accel actuators | `virtual_ecu.py` receiving `CMD` on TCP `:59000` |
| ECU / Arduino / ESP32 / Zephyr | Same Python ECU (protocol-compatible) |

## Quick start (automated)

From the repo root (no manual Docker steps):

```bat
run_adas_demo.bat
```

This script starts Docker if needed, builds `visionpilot:cpu`, generates a synthetic
camera + speed feed, runs VisionPilot + virtual ECU, and prints an ADAS component
report (ACC / FCW / AEB / LKAS / LDW / ISA / Autopilot).

Outputs:
- `platforms/virtual_hw/last_demo_output.txt`
- `platforms/virtual_hw/adas_demo_report.txt`


## Generate assets only

```bash
docker run --rm -v "%CD%/../../demo_data:/data/demo" -v "%CD%/../../platforms/virtual_hw:/opt/virtual_hw:ro" \
  --entrypoint python3 visionpilot:cpu \
  /opt/virtual_hw/generate_demo_assets.py --out-dir /data/demo
```

## Protocol

See [`../protocol/vp_vehicle_gateway.md`](../protocol/vp_vehicle_gateway.md).
