/**
 * @file wifi_manager.h
 * @brief WiFi management interface for AP+STA dual-mode operation
 *
 * Provides WiFi functionality for the ESP32-S3:
 * - Access Point (AP) mode for local network access
 * - Station (STA) mode for connecting to external WiFi networks
 * - Automatic reconnection and state management
 *
 * The system operates in AP+STA mode simultaneously, allowing:
 * 1. Local connections directly to the ESP32 (AP mode)
 * 2. Connections to external networks (STA mode)
 *
 * This enables the device to act as both a WiFi hotspot and a WiFi client.
 */

#pragma once

#include "esp_err.h"

/* SoftAP Config */
#define AP_SSID      "ESP32_S3_AP"
#define AP_PASS      "who-zefa"
#define AP_CHANNEL   1
#define AP_MAX_CONN  4

/**
 * @brief Start WiFi in APSTA mode (SoftAP + Station).
 *        Call once during initialization.
 */
void wifi_start_apsta(void);

/**
 * @brief Attempt to connect the STA interface to a given SSID/password.
 *
 * @param ssid      Target network SSID (null-terminated)
 * @param password  Target network password (null-terminated)
 */
void wifi_connect_sta(const char *ssid, const char *password);

/**
 * @brief Disconnect STA, clear credentials, and restart in APSTA mode.
 */
void wifi_reset_to_apsta(void);