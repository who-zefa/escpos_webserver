/**
 * @file http_server.c
 * @brief HTTP web server implementation with comprehensive REST API endpoints
 *
 * Implements a complete HTTP server exposing all ESC/POS printer functionality:
 * - Text formatting (alignment, bold, underline, font, size, styles)
 * - Image printing (load from file/buffer with dithering)
 * - Barcodes (multiple types with configuration)
 * - QR codes with adjustable size and error correction
 * - Paper control (feed, cut)
 * - Printer configuration (width, reset)
 * - WiFi management (scan, connect, reset)
 *
 * All endpoints return JSON responses and support CORS-friendly requests.
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
#define MAX_BUFFER_SIZE 4096

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

/* ========================= UTILITY HELPERS ========================= */
static void send_json_error(httpd_req_t *req, int status, const char *error)
{
    char json[256];
    snprintf(json, sizeof(json), "{\"error\":\"%s\"}", error);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_status(req, status == 503 ? "503 Service Unavailable" : "400 Bad Request");
    httpd_resp_sendstr(req, json);
}

static void send_json_success(httpd_req_t *req, const char *message)
{
    char json[256];
    snprintf(json, sizeof(json), "{\"status\":\"ok\",\"message\":\"%s\"}", message);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json);
}

static bool ensure_printer_connected(httpd_req_t *req)
{
    if (!printer_manager_is_connected()) {
        send_json_error(req, 503, "Printer not connected");
        return false;
    }
    return true;
}

/* ========================= WEB PAGES ========================= */
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const size_t html_len = index_html_end - index_html_start;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, html_len);
    return ESP_OK;
}

static esp_err_t printer_get_handler(httpd_req_t *req)
{
    const size_t html_len = printer_html_end - printer_html_start;

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)printer_html_start, html_len);
    return ESP_OK;
}

/* ========================= WiFi HANDLERS ========================= */
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

    ESP_LOGI(TAG, "Connect request SSID: %s", ssid);

    wifi_connect_sta(ssid, password);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

static esp_err_t reset_post_handler(httpd_req_t *req)
{
    wifi_reset_to_apsta();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

/* ========================= PRINTER STATUS ========================= */
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

/* ========================= PRINTER INIT ========================= */
static esp_err_t printer_init_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_init_printer(printer);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to initialize printer");
        return ESP_OK;
    }

    send_json_success(req, "Printer initialized");
    return ESP_OK;
}

static esp_err_t printer_set_width_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[128] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    int width_mm = 58;
    sscanf(buf, "width=%d", &width_mm);

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_init_printer_with_width_mm(printer, width_mm);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to set width");
        return ESP_OK;
    }

    send_json_success(req, "Width set");
    return ESP_OK;
}

/* ========================= TEXT OPERATIONS ========================= */
static esp_err_t printer_text_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[512] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    char decoded_text[256] = {0};
    if (sscanf(buf, "text=%255[^&]", decoded_text) == 1) {
        url_decode(decoded_text, decoded_text);
        
        escpos_printer_t *printer = printer_manager_get_printer();
        esp_err_t err = escpos_write_text(printer, decoded_text);
        
        if (err != ESP_OK) {
            send_json_error(req, 400, "Failed to write text");
            return ESP_OK;
        }
    }

    send_json_success(req, "Text written");
    return ESP_OK;
}

static esp_err_t printer_text_aligned_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[512] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    char decoded_text[256] = {0};
    int align = 0;
    
    sscanf(buf, "text=%255[^&]&align=%d", decoded_text, &align);
    url_decode(decoded_text, decoded_text);

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_write_text_aligned(printer, decoded_text, align);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to write aligned text");
        return ESP_OK;
    }

    send_json_success(req, "Aligned text written");
    return ESP_OK;
}

static esp_err_t printer_set_bold_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[64] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    int enabled = 0;
    sscanf(buf, "enabled=%d", &enabled);

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_set_bold(printer, enabled ? true : false);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to set bold");
        return ESP_OK;
    }

    send_json_success(req, "Bold setting updated");
    return ESP_OK;
}

static esp_err_t printer_set_underline_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[64] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    int thickness = 0;
    sscanf(buf, "thickness=%d", &thickness);

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_set_underline(printer, thickness);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to set underline");
        return ESP_OK;
    }

    send_json_success(req, "Underline setting updated");
    return ESP_OK;
}

static esp_err_t printer_set_font_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[64] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    int font = 0;
    sscanf(buf, "font=%d", &font);

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_set_font(printer, font);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to set font");
        return ESP_OK;
    }

    send_json_success(req, "Font updated");
    return ESP_OK;
}

static esp_err_t printer_set_text_size_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[64] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    int width = 1, height = 1;
    sscanf(buf, "width=%d&height=%d", &width, &height);

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_set_text_size(printer, width, height);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to set text size");
        return ESP_OK;
    }

    send_json_success(req, "Text size updated");
    return ESP_OK;
}

static esp_err_t printer_set_align_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[64] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    int align = 0;
    sscanf(buf, "align=%d", &align);

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_set_align(printer, align);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to set alignment");
        return ESP_OK;
    }

    send_json_success(req, "Alignment updated");
    return ESP_OK;
}

static esp_err_t printer_reset_text_format_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_reset_text_format(printer);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to reset text format");
        return ESP_OK;
    }

    send_json_success(req, "Text format reset");
    return ESP_OK;
}

/* ========================= IMAGE OPERATIONS ========================= */
static esp_err_t printer_image_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[512] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    char file_path[256] = {0};
    int dither = 1;
    int mode = 0;
    
    sscanf(buf, "path=%255[^&]&dither=%d&mode=%d", file_path, &dither, &mode);
    url_decode(file_path, file_path);

    escpos_printer_t *printer = printer_manager_get_printer();
    escpos_image_params_t params = escpos_image_get_default_params();
    params.dither_mode = dither;
    params.max_width = escpos_get_printer_width_dots(printer);

    escpos_image_t image = {0};
    esp_err_t err = escpos_image_load_from_file(file_path, &params, &image);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to load image");
        return ESP_OK;
    }

    err = escpos_print_image(printer, &image, mode);
    escpos_image_free(&image);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to print image");
        return ESP_OK;
    }

    send_json_success(req, "Image printed");
    return ESP_OK;
}

/* ========================= BARCODE OPERATIONS ========================= */
static esp_err_t printer_barcode_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[512] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    char barcode_data[128] = {0};
    int barcode_type = 73;  // CODE128
    int width = 2;
    int height = 80;
    int hri = 2;
    
    sscanf(buf, "data=%127[^&]&type=%d&width=%d&height=%d&hri=%d", 
           barcode_data, &barcode_type, &width, &height, &hri);
    url_decode(barcode_data, barcode_data);

    escpos_printer_t *printer = printer_manager_get_printer();
    escpos_barcode_config_t config = escpos_barcode_get_default_config(barcode_type);
    config.width = width;
    config.height = height;
    config.hri = hri;
    config.align = ESCPOS_ALIGN_CENTER;

    esp_err_t err = escpos_print_barcode(printer, barcode_data, &config);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to print barcode");
        return ESP_OK;
    }

    send_json_success(req, "Barcode printed");
    return ESP_OK;
}

/* ========================= QR CODE OPERATIONS ========================= */
static esp_err_t printer_qr_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[512] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        send_json_error(req, 400, "Invalid request");
        return ESP_OK;
    }
    buf[len] = '\0';

    char qr_data[256] = {0};
    int size = 6;
    int ec = 49;  // ESCPOS_QR_EC_M
    
    sscanf(buf, "data=%255[^&]&size=%d&ec=%d", qr_data, &size, &ec);
    url_decode(qr_data, qr_data);

    escpos_printer_t *printer = printer_manager_get_printer();
    escpos_qr_config_t config = escpos_qr_get_default_config();
    config.size = size;
    config.ec_level = ec;
    config.align = ESCPOS_ALIGN_CENTER;

    esp_err_t err = escpos_print_qr(printer, qr_data, &config);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to print QR code");
        return ESP_OK;
    }

    send_json_success(req, "QR code printed");
    return ESP_OK;
}

/* ========================= PAPER CONTROL ========================= */
static esp_err_t printer_feed_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[128] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    
    int lines = 1;
    if (len > 0) {
        buf[len] = '\0';
        sscanf(buf, "lines=%d", &lines);
        if (lines < 1 || lines > 255) lines = 1;
    }

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_feed_lines(printer, lines);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to feed paper");
        return ESP_OK;
    }

    send_json_success(req, "Paper fed");
    return ESP_OK;
}

static esp_err_t printer_cut_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    char buf[128] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    
    bool partial = false;
    if (len > 0) {
        buf[len] = '\0';
        if (strstr(buf, "partial") != NULL) {
            partial = true;
        }
    }

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = partial ? 
        escpos_cut_paper_partial(printer) : 
        escpos_cut_paper(printer);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to cut paper");
        return ESP_OK;
    }

    send_json_success(req, "Paper cut");
    return ESP_OK;
}

static esp_err_t printer_beep_post_handler(httpd_req_t *req)
{
    if (!ensure_printer_connected(req)) return ESP_OK;

    escpos_printer_t *printer = printer_manager_get_printer();
    esp_err_t err = escpos_beep(printer);
    
    if (err != ESP_OK) {
        send_json_error(req, 400, "Failed to beep");
        return ESP_OK;
    }

    send_json_success(req, "Beep sent");
    return ESP_OK;
}

/* ========================= SERVER INITIALIZATION ========================= */
httpd_handle_t http_server_start(void)
{
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.max_uri_handlers = 35;

    httpd_handle_t server = NULL;

    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return NULL;
    }

    static const httpd_uri_t uris[] = {
        /* Web pages */
        { .uri = "/",                      .method = HTTP_GET,  .handler = root_get_handler },
        { .uri = "/printer",               .method = HTTP_GET,  .handler = printer_get_handler },

        /* WiFi */
        { .uri = "/scan",                  .method = HTTP_GET,  .handler = scan_get_handler },
        { .uri = "/connect",               .method = HTTP_POST, .handler = connect_post_handler },
        { .uri = "/reset",                 .method = HTTP_POST, .handler = reset_post_handler },

        /* Printer status & init */
        { .uri = "/printer/status",        .method = HTTP_GET,  .handler = printer_status_get_handler },
        { .uri = "/printer/init",          .method = HTTP_POST, .handler = printer_init_post_handler },
        { .uri = "/printer/set-width",     .method = HTTP_POST, .handler = printer_set_width_post_handler },

        /* Text operations */
        { .uri = "/printer/text",          .method = HTTP_POST, .handler = printer_text_post_handler },
        { .uri = "/printer/text-aligned",  .method = HTTP_POST, .handler = printer_text_aligned_post_handler },
        { .uri = "/printer/bold",          .method = HTTP_POST, .handler = printer_set_bold_post_handler },
        { .uri = "/printer/underline",     .method = HTTP_POST, .handler = printer_set_underline_post_handler },
        { .uri = "/printer/font",          .method = HTTP_POST, .handler = printer_set_font_post_handler },
        { .uri = "/printer/text-size",     .method = HTTP_POST, .handler = printer_set_text_size_post_handler },
        { .uri = "/printer/align",         .method = HTTP_POST, .handler = printer_set_align_post_handler },
        { .uri = "/printer/reset-format",  .method = HTTP_POST, .handler = printer_reset_text_format_post_handler },

        /* Image operations */
        { .uri = "/printer/image",         .method = HTTP_POST, .handler = printer_image_post_handler },

        /* Barcode & QR */
        { .uri = "/printer/barcode",       .method = HTTP_POST, .handler = printer_barcode_post_handler },
        { .uri = "/printer/qr",            .method = HTTP_POST, .handler = printer_qr_post_handler },

        /* Paper control */
        { .uri = "/printer/feed",          .method = HTTP_POST, .handler = printer_feed_post_handler },
        { .uri = "/printer/cut",           .method = HTTP_POST, .handler = printer_cut_post_handler },
        { .uri = "/printer/beep",          .method = HTTP_POST, .handler = printer_beep_post_handler },
    };

    for (int i = 0; i < sizeof(uris) / sizeof(uris[0]); i++) {
        httpd_register_uri_handler(server, &uris[i]);
    }

    ESP_LOGI(TAG, "HTTP server started with %d endpoints", sizeof(uris) / sizeof(uris[0]));
    return server;
}