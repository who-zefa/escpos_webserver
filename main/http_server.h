/**
 * @file http_server.h
 * @brief HTTP web server interface for WiFi and printer control
 *
 * Defines the HTTP server initialization and handles REST endpoints for:
 * - WiFi management (scan, connect, reset)
 * - Printer status and control (print, feed, cut, beep)
 *
 * All endpoints return JSON responses and support CORS-friendly requests.
 */

#pragma once

#include "esp_http_server.h"

/**
 * @brief Start the HTTP web server and register all URI handlers.
 *
 * @return httpd_handle_t  Handle to the running server, or NULL on failure.
 */
httpd_handle_t http_server_start(void);