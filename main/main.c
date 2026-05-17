/**
 * @file main.c
 * @brief Main application entry point for ESP32-S3 WiFi & USB Printer Manager
 *
 * This file implements the main application logic:
 * - Initializes NVS (Non-Volatile Storage) for configuration
 * - Sets up network stack and WiFi in AP+STA mode
 * - Initializes the printer manager for USB ESC/POS printer control
 * - Starts the HTTP web server for remote control
 *
 * The system runs two WiFi modes simultaneously:
 * 1. AP (Access Point) mode - allows local connections
 * 2. STA (Station) mode - allows connection to external WiFi networks
 *
 * @author who-zefa
 * @date 2026
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_err.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "wifi_manager.h"
#include "http_server.h"
#include "printer_manager.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    /* ---- NVS ---- */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* ---- Network / Event loop ---- */
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();

    /* ---- WiFi driver init ---- */
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    /* ---- Printer manager init ---- */
    ESP_ERROR_CHECK(printer_manager_init());

    /* ---- Start AP+STA ---- */
    wifi_start_apsta();

    /* Allow AP to settle before serving requests */
    vTaskDelay(pdMS_TO_TICKS(2000));

    /* ---- Start web server ---- */
    http_server_start();

    ESP_LOGI(TAG, "System ready");

    /* Keep app_main alive; real work is done in callbacks / handlers */
    while (1) {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}