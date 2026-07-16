/*
 * Vision Pilot Zephyr vehicle gateway
 * Target boards (examples):
 *   - STM32: nucleo_h743zi / stm32f4_disco
 *   - NXP:   frdm_mcxn947 / mimxrt1060_evk
 *   - TI:    cc1352r1_launchxl / msp432p401r
 *
 * Build (from Zephyr workspace):
 *   west build -b <board> platforms/zephyr/apps/vp_gateway
 *
 * Protocol: platforms/protocol/vp_vehicle_gateway.md
 */

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define GATEWAY_UART_NODE DT_CHOSEN(zephyr_console)

static const struct device *uart_dev = DEVICE_DT_GET(GATEWAY_UART_NODE);
static float g_speed_mps;
static float g_steer;
static float g_accel;
static char rx_line[128];
static size_t rx_len;

static void handle_line(char *line) {
	if (strncmp(line, "CMD ", 4) == 0) {
		g_steer = strtof(line + 4, NULL);
		char *second = strchr(line + 4, ' ');
		if (second) {
			g_accel = strtof(second + 1, NULL);
		}
	} else if (strcmp(line, "PING") == 0) {
		const char *pong = "PONG\n";
		for (const char *p = pong; *p; ++p) {
			uart_poll_out(uart_dev, *p);
		}
	}
}

static void uart_cb(const struct device *dev, void *user_data) {
	ARG_UNUSED(user_data);
	if (!uart_irq_update(dev) || !uart_irq_rx_ready(dev)) {
		return;
	}
	uint8_t c;
	while (uart_fifo_read(dev, &c, 1) == 1) {
		if (c == '\n') {
			rx_line[rx_len] = 0;
			handle_line(rx_line);
			rx_len = 0;
		} else if (c != '\r' && rx_len + 1 < sizeof(rx_line)) {
			rx_line[rx_len++] = (char)c;
		}
	}
}

int main(void) {
	if (!device_is_ready(uart_dev)) {
		return -1;
	}
	uart_irq_callback_user_data_set(uart_dev, uart_cb, NULL);
	uart_irq_rx_enable(uart_dev);

	char out[64];
	while (1) {
		int n = snprintk(out, sizeof(out), "SPEED %.3f\n", (double)g_speed_mps);
		for (int i = 0; i < n; ++i) {
			uart_poll_out(uart_dev, out[i]);
		}
		/* Placeholder: read wheel-speed / CAN / IMU and update g_speed_mps */
		k_msleep(50);
	}
	return 0;
}
