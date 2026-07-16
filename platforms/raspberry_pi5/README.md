# Raspberry Pi 5 — Vision Pilot host

Pi 5 is an **ARM64 Linux host** for the full Vision Pilot stack (CPU ONNX Runtime).
It does **not** use the GPU Dockerfile (CUDA is x86_64-only in this repo).

## Build ARM64 image (on x86 with buildx)

```bash
cd VisionPilot/docker
./build.sh --cpu --platform linux/arm64 --tag visionpilot:cpu-arm64
docker save visionpilot:cpu-arm64 | gzip > visionpilot-cpu-arm64.tar.gz
# copy to Pi, then: docker load < visionpilot-cpu-arm64.tar.gz
```

## Native build on Pi OS (Bookworm / Ubuntu 24.04 aarch64)

```bash
# Install deps (OpenCV, GStreamer, Ipopt, CppAD, cmake, …) matching Dockerfile.cpu
# Download onnxruntime-linux-aarch64-1.26.0.tgz from Microsoft releases
cd VisionPilot && mkdir -p build && cd build
cmake -DONNXRUNTIME_ROOT=/opt/onnxruntime -DGPU=OFF ../
make -j$(nproc) VisionPilot
```

## Camera

Use CSI or USB camera at 1–2 MP, ~52–55° HFOV. Set:

```
source.mode = v4l2
source.v4l2_device = /dev/video0
engine.provider = cpu
```

## Vehicle gateway

Flash Arduino / ESP32 / Zephyr gateway firmware, then point the MCU at the Pi IP on port `59000`.
