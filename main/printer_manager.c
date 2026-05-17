/**
 * @file printer_manager.c
 * @brief USB ESC/POS printer manager implementation
 *
 * Implements a thread-safe manager for controlling USB thermal printers.
 * Provides centralized access to printer hardware with mutex-protected operations.
 *
 * Key responsibilities:
 * - Initialize and manage the esp_escpos library
 * - Maintain global printer handle and connection state
 * - Provide thread-safe wrapper functions for all printer operations
 * - Handle printer lifecycle (connect, disconnect, cleanup)
 * - Manage concurrent access using FreeRTOS mutexes
 *
 * The printer manager maintains a global printer instance that is protected by a mutex.
 * All operations wait for the mutex with a timeout to prevent deadlocks.
 */

#include <string.h>

#include "esp_log.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#include "escpos.h"
#include "printer_manager.h"

static const char *TAG = "PRINTER_MGR";

/* Global printer state */
static escpos_printer_t *g_printer = NULL;
static SemaphoreHandle_t g_printer_mutex = NULL;
static bool g_initialized = false;

/* ─────────────────────────────────────────────────────────────────────────────
 * Initialization & Cleanup
 * ───────────────────────────────────────────────────────────────────────────*/

esp_err_t printer_manager_init(void)
{
    if (g_initialized) {
        return ESP_OK;
    }

    /* Create mutex for thread-safe printer access */
    g_printer_mutex = xSemaphoreCreateMutex();
    if (!g_printer_mutex) {
        ESP_LOGE(TAG, "Failed to create mutex");
        return ESP_ERR_NO_MEM;
    }

    /* Initialize ESC/POS library */
    esp_err_t err = escpos_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize escpos library: %s", esp_err_to_name(err));
        vSemaphoreDelete(g_printer_mutex);
        g_printer_mutex = NULL;
        return err;
    }

    /* Try to connect to a USB printer (non-blocking attempt) */
    err = escpos_new_usb(&g_printer);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "No USB printer found initially: %s", esp_err_to_name(err));
        g_printer = NULL;
        /* This is OK - printer may be connected later */
    } else {
        ESP_LOGI(TAG, "USB printer connected on init");
    }

    g_initialized = true;
    return ESP_OK;
}

esp_err_t printer_manager_deinit(void)
{
    if (!g_initialized) {
        return ESP_OK;
    }

    if (g_printer_mutex) {
        xSemaphoreTake(g_printer_mutex, portMAX_DELAY);
    }

    if (g_printer) {
        escpos_destroy(g_printer);
        g_printer = NULL;
    }

    if (g_printer_mutex) {
        xSemaphoreGive(g_printer_mutex);
        vSemaphoreDelete(g_printer_mutex);
        g_printer_mutex = NULL;
    }

    escpos_deinit();
    g_initialized = false;
    return ESP_OK;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Connection Status
 * ───────────────────────────────────────────────────────────────────────────*/

bool printer_manager_is_connected(void)
{
    if (!g_initialized || !g_printer_mutex) {
        return false;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    bool connected = (g_printer != NULL) && escpos_is_connected(g_printer);

    xSemaphoreGive(g_printer_mutex);
    return connected;
}

escpos_printer_t *printer_manager_get_printer(void)
{
    if (!g_initialized || !g_printer) {
        return NULL;
    }
    return g_printer;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Write Operations
 * ───────────────────────────────────────────────────────────────────────────*/

esp_err_t printer_manager_write_raw(const uint8_t *data, size_t len)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!data || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_write_raw(g_printer, data, len);
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}

esp_err_t printer_manager_write_text(const char *text)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (!text) {
        return ESP_ERR_INVALID_ARG;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_write_text(g_printer, text);
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}

/* ─────────────────────────────────────────────────────────────────────────────
 * Printer Control Commands
 * ───────────────────────────────────────────────────────────────────────────*/

esp_err_t printer_manager_init_printer(void)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_init_printer(g_printer);
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}

esp_err_t printer_manager_feed_line(void)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_feed_line(g_printer);
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}

esp_err_t printer_manager_feed_lines(uint8_t n)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_feed_lines(g_printer, n);
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}

esp_err_t printer_manager_cut_paper(void)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_cut_paper(g_printer);
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}

esp_err_t printer_manager_cut_paper_partial(void)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_cut_paper_partial(g_printer);
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}

esp_err_t printer_manager_beep(void)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_beep(g_printer);
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}

esp_err_t printer_manager_wait_disconnect(uint32_t timeout_ms)
{
    if (!g_initialized || !g_printer_mutex) {
        return ESP_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(g_printer_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return ESP_ERR_TIMEOUT;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (g_printer) {
        err = escpos_wait_disconnect(g_printer, timeout_ms);
        if (err == ESP_OK || err == ESP_ERR_TIMEOUT) {
            /* Printer disconnected or timeout occurred */
            escpos_destroy(g_printer);
            g_printer = NULL;
        }
    }

    xSemaphoreGive(g_printer_mutex);
    return err;
}
