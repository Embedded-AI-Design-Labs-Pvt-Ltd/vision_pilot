#!/usr/bin/env python3
"""Generate synthetic front-camera video + ego-speed file for Vision Pilot demos.

Creates an OpenLane-like 1920x1080 clip with lane markings and a matching
one-speed-per-frame text file. No real camera or Google Drive assets required.
"""
from __future__ import annotations

import argparse
import math
from pathlib import Path

import cv2
import numpy as np


def draw_road_frame(w: int, h: int, t: float) -> np.ndarray:
    img = np.zeros((h, w, 3), dtype=np.uint8)
    # sky / road gradient
    img[: h // 2] = (90, 70, 50)
    img[h // 2 :] = (40, 40, 40)
    horizon = h // 2
    # vanishing-point road trapezoid
    road = np.array(
        [
            [w * 0.42 + 8 * math.sin(t * 0.4), horizon],
            [w * 0.58 + 8 * math.sin(t * 0.4), horizon],
            [w * 0.95, h - 1],
            [w * 0.05, h - 1],
        ],
        dtype=np.int32,
    )
    cv2.fillConvexPoly(img, road, (55, 55, 55))

    # dashed center + solid edges (perspective-ish)
    cx = w // 2 + int(40 * math.sin(t * 0.35))
    for y in range(horizon + 10, h, 28):
        offset = int((y - horizon) * 0.02 * math.sin(t))
        thickness = max(2, (y - horizon) // 80)
        cv2.line(img, (cx + offset - 6, y), (cx + offset + 6, y + 14), (230, 230, 230), thickness)
        left = int(w * 0.08 + (y - horizon) * 0.15)
        right = int(w * 0.92 - (y - horizon) * 0.15)
        cv2.line(img, (left, y), (left + 4, y + 10), (200, 200, 200), thickness)
        cv2.line(img, (right, y), (right - 4, y + 10), (200, 200, 200), thickness)

    # fake lead vehicle blob (CIPO stand-in)
    lead_y = horizon + 80 + int(30 * math.sin(t * 0.2))
    scale = max(8, (lead_y - horizon) // 6)
    lead_x = cx + int(20 * math.sin(t * 0.15))
    cv2.rectangle(
        img,
        (lead_x - scale, lead_y - scale // 2),
        (lead_x + scale, lead_y + scale),
        (30, 30, 180),
        -1,
    )
    return img


def main() -> None:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out-dir", type=Path, default=Path("/data/demo"))
    ap.add_argument("--frames", type=int, default=120)
    ap.add_argument("--fps", type=int, default=10)
    ap.add_argument("--width", type=int, default=1920)
    ap.add_argument("--height", type=int, default=1080)
    ap.add_argument("--speed-mps", type=float, default=22.0)
    args = ap.parse_args()

    args.out_dir.mkdir(parents=True, exist_ok=True)
    video_path = args.out_dir / "virtual_camera.mp4"
    speed_path = args.out_dir / "virtual_speed.txt"

    fourcc = cv2.VideoWriter_fourcc(*"mp4v")
    writer = cv2.VideoWriter(str(video_path), fourcc, args.fps, (args.width, args.height))
    if not writer.isOpened():
        raise SystemExit(f"Failed to open VideoWriter for {video_path}")

    speeds: list[str] = []
    for i in range(args.frames):
        t = i / float(args.fps)
        frame = draw_road_frame(args.width, args.height, t)
        writer.write(frame)
        # mild speed variation for planner input
        v = args.speed_mps + 1.5 * math.sin(t * 0.5)
        speeds.append(f"{v:.4f}")

    writer.release()
    speed_path.write_text("\n".join(speeds) + "\n", encoding="utf-8")
    print(f"Wrote {video_path} ({args.frames} frames @ {args.fps} fps)")
    print(f"Wrote {speed_path} ({len(speeds)} samples)")


if __name__ == "__main__":
    main()
