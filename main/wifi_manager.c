/**
 * @file wifi_manager.c
 * @brief WiFi management implementation for AP+STA dual-mode operation
 *
 * Implements WiFi management for the ESP32-S3 including:
 * - Initialization of AP+STA dual-mode WiFi
 * - WiFi network scanning and connection
 * - Connection event handling
 * - Automatic reconnection on disconnection
 * - Mode reset to default AP+STA configuration
 *
 * The device broadcasts an Access Point (AP) for direct local connections
 * while also maintaining the ability to connect to external WiFi networks.
 */

#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
#include "esp_log.h"

#include "wifi_manager.h"

static const char *TAG = "WIFI_MANAGER";

/* Cached credentials (used for reconnect / reset reference) */
static char sta_ssid[33] = {0};
static char sta_pass[65] = {0};

/* ------------------------------------------------------------------ */

void wifi_start_apsta(void)
{
    wifi_config_t ap_config = {
        .ap = {
            .ssid        = AP_SSID,
            .ssid_len    = strlen(AP_SSID),
            .channel     = AP_CHANNEL,
            .password    = AP_PASS,
            .max_connection = AP_MAX_CONN,
            .authmode    = WIFI_AUTH_WPA2_PSK,
        }
    };

    if (strlen(AP_PASS) == 0) {
        ap_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SoftAP started  SSID: %s", AP_SSID);
}

/* ------------------------------------------------------------------ */

void wifi_connect_sta(const char *ssid, const char *password)
{
    wifi_config_t sta_config = {0};

    strncpy((char *)sta_config.sta.ssid,
            ssid,
            sizeof(sta_config.sta.ssid) - 1);

    strncpy((char *)sta_config.sta.password,
            password,
            sizeof(sta_config.sta.password) - 1);

    sta_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    sta_config.sta.pmf_cfg.capable    = true;
    sta_config.sta.pmf_cfg.required   = false;

    ESP_LOGI(TAG, "Connecting to SSID: %s", ssid);

    esp_wifi_scan_stop();
    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(500));

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta_config));

    esp_err_t err = esp_wifi_connect();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Connect failed: %s", esp_err_to_name(err));
        return;
    }

    /* Cache credentials */
    strncpy(sta_ssid, ssid,     sizeof(sta_ssid) - 1);
    strncpy(sta_pass, password, sizeof(sta_pass) - 1);
}

/* ------------------------------------------------------------------ */

void wifi_reset_to_apsta(void)
{
    ESP_LOGI(TAG, "Resetting WiFi to APSTA mode");

    esp_wifi_disconnect();

    memset(sta_ssid, 0, sizeof(sta_ssid));
    memset(sta_pass, 0, sizeof(sta_pass));

    wifi_start_apsta();
}