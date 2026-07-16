# Vision Pilot vehicle gateway protocol

Line-oriented ASCII over **TCP** (default port `59000`) or **UART** (115200 8N1).

## MCU → Host (ego speed)

```
SPEED <meters_per_second>\n
```

Example: `SPEED 22.5\n`

## Host → MCU (actuator command)

```
CMD <steering_rad> <acceleration_mps2>\n
```

Example: `CMD -0.05 0.8\n`

## Heartbeat (optional)

```
PING\n
PONG\n
```

## Notes

- Values are decimal floating point, ASCII, space-separated.
- Unknown lines are ignored.
- On the host, `CanInterface` listens on `VP_VEHICLE_GATEWAY_HOST` / `VP_VEHICLE_GATEWAY_PORT`
  (defaults: `0.0.0.0:59000`).
