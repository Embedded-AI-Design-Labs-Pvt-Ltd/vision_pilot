#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "lwip/sockets.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

/*
 * Vision Pilot ESP32 vehicle gateway
 * - UART0 @ 115200 for local debug / SBC link
 * - Optional TCP client to host:59000 (set VP_HOST_IP)
 *
 * Protocol: platforms/protocol/vp_vehicle_gateway.md
 */

#define TAG "vp_gateway"
#define UART_NUM UART_NUM_0
#define VP_HOST_IP "192.168.1.10"
#define VP_HOST_PORT 59000

static float g_speed_mps = 0.0f;
static float g_steer = 0.0f;
static float g_accel = 0.0f;

static void handle_cmd(const char *line) {
    float steer = 0.0f, accel = 0.0f;
    if (strncmp(line, "CMD ", 4) == 0) {
        if (sscanf(line + 4, "%f %f", &steer, &accel) == 2) {
            g_steer = steer;
            g_accel = accel;
            ESP_LOGI(TAG, "cmd steer=%.4f accel=%.4f", g_steer, g_accel);
        }
    } else if (strcmp(line, "PING") == 0) {
        uart_write_bytes(UART_NUM, "PONG\n", 5);
    }
}

static void uart_task(void *arg) {
    uint8_t data[128];
    char line[128];
    size_t n = 0;
    while (1) {
        int len = uart_read_bytes(UART_NUM, data, sizeof(data), pdMS_TO_TICKS(20));
        for (int i = 0; i < len; ++i) {
            char c = (char)data[i];
            if (c == '\n') {
                line[n] = 0;
                handle_cmd(line);
                n = 0;
            } else if (c != '\r' && n + 1 < sizeof(line)) {
                line[n++] = c;
            }
        }
        char out[64];
        int m = snprintf(out, sizeof(out), "SPEED %.3f\n", g_speed_mps);
        uart_write_bytes(UART_NUM, out, m);
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

static void tcp_client_task(void *arg) {
    while (1) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in dest = {0};
        dest.sin_family = AF_INET;
        dest.sin_port = htons(VP_HOST_PORT);
        inet_pton(AF_INET, VP_HOST_IP, &dest.sin_addr);
        if (connect(sock, (struct sockaddr *)&dest, sizeof(dest)) == 0) {
            ESP_LOGI(TAG, "connected to %s:%d", VP_HOST_IP, VP_HOST_PORT);
            char buf[128];
            while (1) {
                int m = snprintf(buf, sizeof(buf), "SPEED %.3f\n", g_speed_mps);
                if (send(sock, buf, m, 0) < 0) break;
                int n = recv(sock, buf, sizeof(buf) - 1, MSG_DONTWAIT);
                if (n > 0) {
                    buf[n] = 0;
                    char *save = NULL;
                    char *line = strtok_r(buf, "\n", &save);
                    while (line) {
                        handle_cmd(line);
                        line = strtok_r(NULL, "\n", &save);
                    }
                }
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }
        close(sock);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    ESP_ERROR_CHECK(nvs_flash_init());
    uart_config_t cfg = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_driver_install(UART_NUM, 1024, 0, 0, NULL, 0);
    uart_param_config(UART_NUM, &cfg);
    xTaskCreate(uart_task, "uart_task", 4096, NULL, 5, NULL);
    xTaskCreate(tcp_client_task, "tcp_task", 4096, NULL, 5, NULL);
}
