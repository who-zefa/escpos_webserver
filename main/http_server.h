/**
 * @file http_server.h
 * @brief HTTP web server interface for WiFi and printer control
 *
 * Defines the HTTP server initialization and handles REST endpoints for:
 * - WiFi management (scan, connect, reset)
 * - Printer status and control (print, feed, cut, beep)
 * - Image printing support (load from file/buffer and print)
 *
 * All endpoints return JSON responses and support CORS-friendly requests.
 *
 * Image Printing via HTTP:
 * For implementing image printing endpoints, use the escpos image API
 * by obtaining the printer handle:
 *
 *   escpos_printer_t *printer = printer_manager_get_printer();
 *   if (printer) {
 *       escpos_image_t image = {0};
 *       escpos_image_params_t params = escpos_image_get_default_params();
 *       params.max_width = escpos_get_printer_width_dots(printer);
 *       escpos_image_load_from_file(file_path, &params, &image);
 *       escpos_print_image(printer, &image, ESCPOS_IMAGE_MODE_NORMAL);
 *       escpos_image_free(&image);
 *   }
 */

#pragma once

#include "esp_http_server.h"

/**
 * @brief Start the HTTP web server and register all URI handlers.
 *
 * @return httpd_handle_t  Handle to the running server, or NULL on failure.
 */
httpd_handle_t http_server_start(void);