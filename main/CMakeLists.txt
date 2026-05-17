# =============================================================================
# Main Application Component CMakeLists.txt
# =============================================================================
#
# This file configures the build for the main application component.
# It specifies:
# - Source files to compile
# - Include directories
# - Component dependencies
# - Embedded binary data (HTML files)
#
# Component Responsibilities:
# - main.c: Application entry point
# - wifi_manager.c: WiFi AP+STA mode management
# - http_server.c: REST API server implementation
# - printer_manager.c: USB printer control and management
#
# External Dependencies:
# - nvs_flash: Non-volatile storage
# - esp_wifi: WiFi driver
# - esp_netif: Network interface
# - esp_event: Event system
# - esp_http_server: HTTP server library
# - usb_host_cdc_acm: USB serial communication
# - usb: USB driver
# - esp_escpos: ESC/POS printer library (custom component)
#
# Embedded Resources:
# - index.html: WiFi configuration UI
# - printer.html: Printer control UI
#
# These HTML files are compiled into the firmware binary and serve as
# web pages accessed by clients connecting to the HTTP server.
#

idf_component_register(
    SRCS
        "main.c"
        "wifi_manager.c"
        "http_server.c"
        "printer_manager.c"
    INCLUDE_DIRS
        "."
    REQUIRES
        nvs_flash
        esp_wifi
        esp_netif
        esp_event
        esp_http_server
        usb_host_cdc_acm
        usb
        esp_escpos
)

# Embed index.html into the firmware binary.
# ESP-IDF will expose two symbols in your C code:
#   extern const uint8_t index_html_start[] asm("_binary_index_html_start");
#   extern const uint8_t index_html_end[]   asm("_binary_index_html_end");
target_add_binary_data(${COMPONENT_TARGET} "index.html" TEXT)

# Embed printer.html into the firmware binary.
# ESP-IDF will expose two symbols in your C code:
#   extern const uint8_t printer_html_start[] asm("_binary_printer_html_start");
#   extern const uint8_t printer_html_end[]   asm("_binary_printer_html_end");
target_add_binary_data(${COMPONENT_TARGET} "printer.html" TEXT)
