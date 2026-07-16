# Vision Pilot — multi-platform support

Vision Pilot’s neural-net ADAS stack (AutoSpeed / AutoSteer / AutoDrive + MPC) targets
**Linux x86_64**, **Linux aarch64 (Raspberry Pi 5)**, and optionally **QNX SDP** for
automotive hosts.

Microcontrollers (**Arduino**, **ESP32**, **STM32**, **NXP**, **TI**) do **not** run the
ONNX models. They run a **vehicle gateway** that exchanges speed / steering / acceleration
with the host over UART or TCP using the shared protocol in
[`protocol/vp_vehicle_gateway.md`](protocol/vp_vehicle_gateway.md).

| Platform | Role | Path |
|----------|------|------|
| Docker (CPU/GPU) | Primary host runtime | `VisionPilot/docker/` |
| Raspberry Pi 5 | ARM64 host (CPU ORT) | `platforms/raspberry_pi5/` |
| QNX + SDP | Safety host (toolchain stubs) | `platforms/qnx_sdp/` |
| Zephyr (STM32 / NXP / TI) | RTOS vehicle gateway | `platforms/zephyr/` |
| ESP32 | Wi-Fi/UART gateway | `platforms/esp32/` |
| Arduino | UART gateway | `platforms/arduino/` |
| Virtual HW (demo) | Synthetic camera + virtual ECU | `platforms/virtual_hw/` |

Cloud / CI:

| Stack | Path |
|-------|------|
| Jenkins | `infra/jenkins/` |
| Terraform AWS / GCP / Azure | `infra/terraform/` |
