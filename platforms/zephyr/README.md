# Zephyr RTOS vehicle gateway for Vision Pilot

Supported example boards:

| Vendor | Example board string | Notes |
|--------|----------------------|-------|
| ST (STM32) | `nucleo_h743zi` | High-perf Cortex-M7 |
| NXP | `frdm_mcxn947` / `mimxrt1060_evk` | Automotive-friendly MCUs |
| TI | `cc1352r1_launchxl` | Low-power + Zephyr |

```bash
# Install Zephyr SDK + west, then:
west init -m https://github.com/zephyrproject-rtos/zephyr --mr main zephyr-workspace
cd zephyr-workspace && west update
west build -b nucleo_h743zi ../vision_pilot/platforms/zephyr/apps/vp_gateway
west flash
```

Host side: run VisionPilot with `CanInterface` / vehicle gateway TCP enabled and connect UART-USB
adapter or use a TCP bridge (ESP32 companion).
