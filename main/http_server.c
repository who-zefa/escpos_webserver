/**
 * @file http_server.c
 * @brief HTTP web server implementation with REST API endpoints
 *
 * Implements a lightweight HTTP server with the following features:
 * - Serves embedded HTML UI pages (index.html and printer.html)
 * - WiFi management endpoints (/scan, /connect, /reset)
 * - Printer control endpoints (/printer/status, /printer/print, /printer/feed, etc.)
 * - URL decoding for form data processing
 * - JSON response formatting
 *
 * Endpoints:
 *   GET  /              - Serves WiFi manager page
 *   GET  /printer       - Serves printer manager page
 *   GET  /scan          - Scans available WiFi networks
 *   POST /connect       - Connects to a WiFi network
 *   POST /reset         - Resets WiFi to AP+STA mode
 *   GET  /printer/status - Gets printer connection status
 *   POST /printer/print - Sends text to printer
 *   POST /printer/feed  - Feeds paper
 *   POST /printer/cut   - Cuts paper
 *   POST /printer/init  - Initializes printer
 *   POST /printer/beep  - Triggers printer beep
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_http_server.h"

#include "http_server.h"
#include "wifi_manager.h"
#include "printer_manager.h"

static const char *TAG = "HTTP_SERVER";

#define MAX_SCAN_RESULTS 15

/* ========================= EMBEDDED HTML ========================= */
/* These symbols are generated automatically by ESP-IDF when
 * target_add_binary_data() is used in CMakeLists.txt.
 * The linker maps the raw bytes of index.html between _start and _end.
 * _end points to one byte PAST the last character (like a C array sentinel).
 * The file is embedded as TEXT so a null terminator is appended,
 * meaning (index_html_end - index_html_start) equals strlen + 1.          */
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

extern const uint8_t printer_html_start[] asm("_binary_printer_html_start");
extern const uint8_t printer_html_end[]   asm("_binary_printer_html_end");

/* ========================= URL DECODE ========================= */
static void url_decode(char *dst, const char *src)
{
    char a, b;
    while (*src) {
        if ((*src == '%') &&
            (a = src[1]) && (b = src[2]) &&
            isxdigit((unsigned char)a) && isxdigit((unsigned char)b))
        {
            if (a >= 'a') a -= 32;
            a = (a >= 'A') ? (a - 'A' + 10) : (a - '0');

            if (b >= 'a') b -= 32;
            b = (b >= 'A') ? (b - 'A' + 10) : (b - '0');

            *dst++ = 16 * a + b;
            src += 3;
        } else if (*src == '+') {
            *dst++ = ' ';
            src++;
        } else {
            *dst++ = *src++;
        }
    }
    *dst = '\0';
}

/* ========================= HANDLERS ========================= */

static esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t html_len = index_html_end - index_html_start;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, html_len);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t printer_get_handler(httpd_req_t *req)
{
    const size_t html_len = printer_html_end - printer_html_start;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)printer_html_start, html_len);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t scan_get_handler(httpd_req_t *req)
{
    wifi_scan_config_t scan_config = {
        .ssid        = NULL,
        .bssid       = NULL,
        .channel     = 0,
        .show_hidden = true,
    };

    wifi_ap_record_t ap_records[MAX_SCAN_RESULTS];
    uint16_t ap_count = MAX_SCAN_RESULTS;

    esp_err_t err = esp_wifi_scan_start(&scan_config, true);

    if (err == ESP_ERR_WIFI_STATE) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"networks\":[]}");
        return ESP_OK;
    }

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Scan start failed: %s", esp_err_to_name(err));
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));

    /* Build JSON response */
    char *json = malloc(12000);
    if (!json) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    strcpy(json, "{\"networks\":[");

    for (int i = 0; i < ap_count; i++) {
        char entry[256];
        snprintf(entry, sizeof(entry),
                 "{\"ssid\":\"%s\",\"rssi\":%d,\"channel\":%d,\"auth\":%d}%s",
                 (char *)ap_records[i].ssid,
                 ap_records[i].rssi,
                 ap_records[i].primary,
                 ap_records[i].authmode,
                 (i < ap_count - 1) ? "," : "");
        strcat(json, entry);
    }

    strcat(json, "]}");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, json, HTTPD_RESP_USE_STRLEN);

    free(json);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t connect_post_handler(httpd_req_t *req)
{
    char buf[256] = {0};

    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[len] = '\0';

    char encoded_ssid[128] = {0};
    char encoded_pass[128] = {0};
    char ssid[64]          = {0};
    char password[64]      = {0};

    sscanf(buf, "ssid=%127[^&]&password=%127s", encoded_ssid, encoded_pass);

    url_decode(ssid,     encoded_ssid);
    url_decode(password, encoded_pass);

    ESP_LOGI(TAG, "Connect request  SSID: %s", ssid);

    wifi_connect_sta(ssid, password);

    httpd_resp_sendstr(req, "Connection attempt started");
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t reset_post_handler(httpd_req_t *req)
{
    wifi_reset_to_apsta();
    httpd_resp_sendstr(req, "Reset to APSTA successful");
    return ESP_OK;
}

/* ========================= PRINTER HANDLERS ========================= */

static esp_err_t printer_status_get_handler(httpd_req_t *req)
{
    bool connected = printer_manager_is_connected();
    
    char json[128];
    snprintf(json, sizeof(json), 
             "{\"connected\":%s}",
             connected ? "true" : "false");
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t printer_print_post_handler(httpd_req_t *req)
{
    if (!printer_manager_is_connected()) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_sendstr(req, "{\"error\":\"Printer not connected\"}");
        return ESP_OK;
    }

    char buf[512] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }
    buf[len] = '\0';

    /* Parse JSON-like request or form data */
    char decoded_text[256] = {0};
    if (sscanf(buf, "text=%255[^&]", decoded_text) == 1) {
        url_decode(decoded_text, decoded_text);
        
        esp_err_t err = printer_manager_write_text(decoded_text);
        if (err != ESP_OK) {
            httpd_resp_set_status(req, "500 Internal Server Error");
            httpd_resp_sendstr(req, "{\"error\":\"Failed to write to printer\"}");
            return ESP_OK;
        }
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t printer_feed_post_handler(httpd_req_t *req)
{
    if (!printer_manager_is_connected()) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_sendstr(req, "{\"error\":\"Printer not connected\"}");
        return ESP_OK;
    }

    char buf[128] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        /* Default to 1 line if no parameter */
        printer_manager_feed_line();
    } else {
        buf[len] = '\0';
        
        int lines = 1;
        if (sscanf(buf, "lines=%d", &lines) == 1) {
            if (lines > 0 && lines < 256) {
                printer_manager_feed_lines(lines);
            } else {
                printer_manager_feed_line();
            }
        }
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t printer_cut_post_handler(httpd_req_t *req)
{
    if (!printer_manager_is_connected()) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_sendstr(req, "{\"error\":\"Printer not connected\"}");
        return ESP_OK;
    }

    char buf[128] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    
    bool partial = false;
    if (len > 0) {
        buf[len] = '\0';
        if (strstr(buf, "partial") != NULL) {
            partial = true;
        }
    }

    esp_err_t err = partial ? 
        printer_manager_cut_paper_partial() : 
        printer_manager_cut_paper();
        
    if (err != ESP_OK) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\":\"Failed to cut paper\"}");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t printer_init_post_handler(httpd_req_t *req)
{
    if (!printer_manager_is_connected()) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_sendstr(req, "{\"error\":\"Printer not connected\"}");
        return ESP_OK;
    }

    esp_err_t err = printer_manager_init_printer();
    if (err != ESP_OK) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\":\"Failed to initialize printer\"}");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

/* ------------------------------------------------------------------ */

static esp_err_t printer_beep_post_handler(httpd_req_t *req)
{
    if (!printer_manager_is_connected()) {
        httpd_resp_set_status(req, "503 Service Unavailable");
        httpd_resp_sendstr(req, "{\"error\":\"Printer not connected\"}");
        return ESP_OK;
    }

    esp_err_t err = printer_manager_beep();
    if (err != ESP_OK) {
        httpd_resp_set_status(req, "500 Internal Server Error");
        httpd_resp_sendstr(req, "{\"error\":\"Failed to beep\"}");
        return ESP_OK;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

/* ========================= SERVER START ========================= */

httpd_handle_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 17;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }

    /* URI table */
    static const httpd_uri_t uris[] = {
        { .uri = "/",               .method = HTTP_GET,  .handler = root_get_handler        },
        { .uri = "/printer",        .method = HTTP_GET,  .handler = printer_get_handler     },
        { .uri = "/scan",           .method = HTTP_GET,  .handler = scan_get_handler        },
        { .uri = "/connect",        .method = HTTP_POST, .handler = connect_post_handler   },
        { .uri = "/reset",          .method = HTTP_POST, .handler = reset_post_handler     },
        { .uri = "/printer/status", .method = HTTP_GET,  .handler = printer_status_get_handler },
        { .uri = "/printer/print",  .method = HTTP_POST, .handler = printer_print_post_handler },
        { .uri = "/printer/feed",   .method = HTTP_POST, .handler = printer_feed_post_handler },
        { .uri = "/printer/cut",    .method = HTTP_POST, .handler = printer_cut_post_handler },
        { .uri = "/printer/init",   .method = HTTP_POST, .handler = printer_init_post_handler },
        { .uri = "/printer/beep",   .method = HTTP_POST, .handler = printer_beep_post_handler },
    };

    for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        httpd_register_uri_handler(server, &uris[i]);
    }

    ESP_LOGI(TAG, "HTTP server started");
    return server;
}