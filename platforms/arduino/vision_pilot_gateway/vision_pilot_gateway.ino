/*
 * Vision Pilot — Arduino vehicle gateway
 * Boards: Uno R4 / Mega / compatible with HardwareSerial
 * Baud: 115200
 *
 * Wiring: USB-serial to host OR external UART to SBC running VisionPilot.
 * Protocol: platforms/protocol/vp_vehicle_gateway.md
 */

#ifndef VP_UART
#define VP_UART Serial
#endif

static float g_speed_mps = 0.0f;
static float g_steer_cmd = 0.0f;
static float g_accel_cmd = 0.0f;

void setup() {
  VP_UART.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
}

static void handle_line(String line) {
  line.trim();
  if (line.startsWith("CMD ")) {
    float steer = 0.0f;
    float accel = 0.0f;
    if (sscanf(line.c_str() + 4, "%f %f", &steer, &accel) == 2) {
      g_steer_cmd = steer;
      g_accel_cmd = accel;
      digitalWrite(LED_BUILTIN, g_accel_cmd > 0.0f ? HIGH : LOW);
      // TODO: map to ESC / steer actuator drivers for your vehicle
    }
  } else if (line == "PING") {
    VP_UART.println("PONG");
  }
}

void loop() {
  static String buf;
  while (VP_UART.available()) {
    char c = (char)VP_UART.read();
    if (c == '\n') {
      handle_line(buf);
      buf = "";
    } else if (c != '\r') {
      buf += c;
      if (buf.length() > 120) buf = "";
    }
  }

  // Demo ego speed: replace with wheel-speed / GPS / CAN decode
  g_speed_mps = 0.0f;
  VP_UART.print("SPEED ");
  VP_UART.println(g_speed_mps, 3);
  delay(50); // ~20 Hz
}
