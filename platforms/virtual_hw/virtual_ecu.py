#!/usr/bin/env python3
"""Virtual MCU / ECU for Vision Pilot demonstrations.

Connects to the host vehicle gateway (TCP) and:
  - sends SPEED <mps> at ~20 Hz (virtual wheel-speed sensor)
  - receives CMD <steer> <accel> and logs them (virtual actuators)

Protocol: platforms/protocol/vp_vehicle_gateway.md
"""
from __future__ import annotations

import argparse
import math
import socket
import sys
import time


def main() -> int:
    ap = argparse.ArgumentParser(description="Virtual Vision Pilot MCU/ECU")
    ap.add_argument("--host", default="visionpilot-demo")
    ap.add_argument("--port", type=int, default=59000)
    ap.add_argument("--speed", type=float, default=22.0, help="nominal ego speed m/s")
    ap.add_argument("--duration", type=float, default=60.0, help="seconds (0 = forever)")
    ap.add_argument("--retry", type=float, default=2.0)
    args = ap.parse_args()

    buf = ""
    t0 = time.time()
    print(f"[virtual-ecu] targeting {args.host}:{args.port}", flush=True)

    while True:
        if args.duration > 0 and (time.time() - t0) > args.duration:
            print("[virtual-ecu] duration elapsed — exit", flush=True)
            return 0
        try:
            with socket.create_connection((args.host, args.port), timeout=5) as sock:
                sock.settimeout(0.05)
                print("[virtual-ecu] connected (virtual UART/CAN over TCP)", flush=True)
                n = 0
                while True:
                    if args.duration > 0 and (time.time() - t0) > args.duration:
                        return 0
                    elapsed = time.time() - t0
                    speed = args.speed + 1.2 * math.sin(elapsed * 0.4)
                    msg = f"SPEED {speed:.3f}\n"
                    sock.sendall(msg.encode("ascii"))
                    try:
                        chunk = sock.recv(256)
                        if not chunk:
                            print("[virtual-ecu] host closed connection", flush=True)
                            break
                        buf += chunk.decode("ascii", errors="ignore")
                        while "\n" in buf:
                            line, buf = buf.split("\n", 1)
                            line = line.strip()
                            if line.startswith("CMD "):
                                print(f"[virtual-ecu] actuator {line}", flush=True)
                            elif line == "PING":
                                sock.sendall(b"PONG\n")
                    except socket.timeout:
                        pass
                    n += 1
                    if n % 40 == 0:
                        print(f"[virtual-ecu] heartbeat speed={speed:.2f} m/s", flush=True)
                    time.sleep(0.05)
        except OSError as exc:
            print(f"[virtual-ecu] waiting for host gateway: {exc}", flush=True)
            time.sleep(args.retry)


if __name__ == "__main__":
    sys.exit(main())
