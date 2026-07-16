# QNX SDP — Vision Pilot host port (scaffold)

QNX is a common automotive OS. Full Vision Pilot on QNX requires a licensed
**QNX Software Development Platform (SDP)** toolchain and QNX-compatible packages
for OpenCV, ONNX Runtime, and Ipopt/CppAD (or alternatives).

This directory provides:

1. CMake toolchain file for cross-compiling from Linux with QNX SDP
2. Porting checklist for SDP 8.0 / 7.1

## Prerequisites

- QNX SDP installed (e.g. `/opt/qnx800` or `/opt/qnx710`)
- Environment: `source qnxsdp-env.sh`
- ONNX Runtime built for QNX aarch64/x86_64 (vendor or source build)
- OpenCV + Eigen + Ipopt available for the QNX target

## Configure

```bash
export QNX_HOST=/opt/qnx800/host/linux/x86_64
export QNX_TARGET=/opt/qnx800/target/qnx
cd VisionPilot && mkdir -p build-qnx && cd build-qnx
cmake \
  -DCMAKE_TOOLCHAIN_FILE=../../platforms/qnx_sdp/cmake/qnx.toolchain.cmake \
  -DONNXRUNTIME_ROOT=/opt/onnxruntime-qnx \
  -DGPU=OFF \
  -DENABLE_ROS2_INTERFACE=OFF \
  ../
make -j$(nproc) VisionPilot
```

## Porting checklist

- [ ] Replace Linux V4L2 camera backend with QNX screen/camera or shared memory frames
- [ ] Rebuild ONNX Runtime with QNX toolchain (CPU EP first)
- [ ] Map GStreamer / WebRTC viz to QNX Screen or disable visualization (`visualization_on = false`)
- [ ] Vehicle I/O via QNX io-can / resource managers instead of Linux sockets if required
- [ ] Safety: isolate ASIL partitions per your ISO 26262 concept

## SDP note

SDP binaries and headers are proprietary to BlackBerry QNX. This repository only
ships **open scaffolding**; you must obtain SDP separately under your OEM license.
