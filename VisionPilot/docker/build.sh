#!/usr/bin/env bash
#
# Build the VisionPilot Docker image, choosing GPU or CPU variant and
# optionally enabling ROS2 support.
#
# Usage:
#   ./build.sh [--gpu|--cpu] [--ros2] [--no-cache] [--tag <name>] [--platform <arch>]
#
# Examples:
#   ./build.sh                    # GPU build, ROS2 off (default)
#   ./build.sh --cpu              # CPU build, ROS2 off
#   ./build.sh --gpu --ros2       # GPU build, ROS2 on
#   ./build.sh --cpu --no-cache   # CPU build, force a clean rebuild
#   ./build.sh --cpu --tag myimg:latest
#   ./build.sh --cpu --platform linux/arm64   # Raspberry Pi 5 / aarch64

set -euo pipefail

VARIANT="gpu"
ENABLE_ROS2="OFF"
NO_CACHE=""
CUSTOM_TAG=""
PLATFORM=""

usage() {
    awk '/^#!/{next} /^#/{sub(/^# ?/,""); print; next} {exit}' "$0"
    exit 1
}

while [ $# -gt 0 ]; do
    case "$1" in
        --gpu)
            VARIANT="gpu"
            shift
            ;;
        --cpu)
            VARIANT="cpu"
            shift
            ;;
        --ros2)
            ENABLE_ROS2="ON"
            shift
            ;;
        --no-cache)
            NO_CACHE="--no-cache"
            shift
            ;;
        --tag)
            [ $# -ge 2 ] || { echo "Error: --tag requires a value" >&2; exit 1; }
            CUSTOM_TAG="$2"
            shift 2
            ;;
        --platform)
            [ $# -ge 2 ] || { echo "Error: --platform requires a value" >&2; exit 1; }
            PLATFORM="$2"
            shift 2
            ;;
        -h|--help)
            usage
            ;;
        *)
            echo "Error: unknown argument '$1'" >&2
            usage
            ;;
    esac
done

# Sanity checks before doing anything expensive
if ! command -v docker >/dev/null 2>&1; then
    echo "Error: docker is not installed or not on PATH." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONTEXT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

if [ ! -f "$CONTEXT_DIR/CMakeLists.txt" ]; then
    echo "Error: CMakeLists.txt not found in $CONTEXT_DIR." >&2
    exit 1
fi

if [ "$VARIANT" = "gpu" ]; then
    DOCKERFILE="$SCRIPT_DIR/Dockerfile"
else
    DOCKERFILE="$SCRIPT_DIR/Dockerfile.cpu"
fi

if [ ! -f "$DOCKERFILE" ]; then
    echo "Error: $DOCKERFILE not found." >&2
    exit 1
fi

# Default tag reflects the chosen options, e.g. visionpilot:gpu-ros2
TAG="visionpilot:${VARIANT}"
if [ "$ENABLE_ROS2" = "ON" ]; then
    TAG="${TAG}-ros2"
fi
if [ -n "$CUSTOM_TAG" ]; then
    TAG="$CUSTOM_TAG"
fi

echo "=================================================="
echo " VisionPilot Docker build"
echo "=================================================="
echo " Variant:      $VARIANT"
echo " ROS2 support: $ENABLE_ROS2"
echo " Dockerfile:   $DOCKERFILE"
echo " Context:      $CONTEXT_DIR"
echo " Image tag:    $TAG"
echo " Platform:     ${PLATFORM:-host default}"
echo " No cache:     $([ -n "$NO_CACHE" ] && echo yes || echo no)"
echo "=================================================="

BUILD_ARGS=(build $NO_CACHE -t "$TAG" -f "$DOCKERFILE" --build-arg ENABLE_ROS2="$ENABLE_ROS2")
if [ -n "$PLATFORM" ]; then
    BUILD_ARGS+=(--platform "$PLATFORM")
    # Map docker platform to Dockerfile TARGETARCH for CPU ORT download
    case "$PLATFORM" in
        *arm64*|*aarch64*) BUILD_ARGS+=(--build-arg TARGETARCH=arm64) ;;
        *amd64*|*x86_64*) BUILD_ARGS+=(--build-arg TARGETARCH=amd64) ;;
    esac
fi
BUILD_ARGS+=("$CONTEXT_DIR")

docker "${BUILD_ARGS[@]}"

echo
echo "Build complete: $TAG"
if [ "$VARIANT" = "gpu" ]; then
    echo "Run with:  ./run.sh --gpu"
else
    echo "Run with:  ./run.sh --cpu --no-display"
fi