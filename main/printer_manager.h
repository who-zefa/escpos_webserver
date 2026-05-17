/**
 * @file printer_manager.h
 * @brief USB ESC/POS printer manager interface
 *
 * Provides a thread-safe API for controlling USB thermal printers using the ESC/POS protocol.
 * Manages printer lifecycle, connection state, and command buffering.
 *
 * Features:
 * - Thread-safe access to USB printer device
 * - Automatic USB device detection and connection
 * - Support for text printing, paper feeding, cutting, and beeping
 * - Mutex-protected operations to prevent concurrent access issues
 *
 * All operations are protected by a FreeRTOS mutex to ensure thread-safe access.
 */

#pragma once

#include "esp_err.h"
#include "escpos.h"

/**
 * @brief Initialize the printer manager.
 * Should be called during system startup.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_init(void);

/**
 * @brief Deinitialize the printer manager.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_deinit(void);

/**
 * @brief Check if a USB printer is currently connected.
 *
 * @return true if connected, false otherwise.
 */
bool printer_manager_is_connected(void);

/**
 * @brief Get the current printer handle (if connected).
 *
 * @return Pointer to escpos_printer_t on success, NULL if not connected.
 */
escpos_printer_t *printer_manager_get_printer(void);

/**
 * @brief Write raw data to the printer.
 *
 * @param data Pointer to the data buffer.
 * @param len  Length of the data.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_write_raw(const uint8_t *data, size_t len);

/**
 * @brief Write text to the printer.
 *
 * @param text Null-terminated text string.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_write_text(const char *text);

/**
 * @brief Feed one line on the printer.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_feed_line(void);

/**
 * @brief Feed multiple lines on the printer.
 *
 * @param n Number of lines to feed.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_feed_lines(uint8_t n);

/**
 * @brief Initialize the printer (reset state).
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_init_printer(void);

/**
 * @brief Cut the paper (full cut).
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_cut_paper(void);

/**
 * @brief Cut the paper (partial cut).
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_cut_paper_partial(void);

/**
 * @brief Trigger printer beep.
 *
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_beep(void);

/**
 * @brief Wait for printer to disconnect (with timeout).
 *
 * @param timeout_ms Timeout in milliseconds.
 * @return ESP_OK on success, or an error code.
 */
esp_err_t printer_manager_wait_disconnect(uint32_t timeout_ms);
