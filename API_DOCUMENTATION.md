# ESP32-S3 ESC/POS Printer Web API Documentation

## Project Overview

This is a complete ESP32-S3 web-based ESC/POS thermal printer controller with comprehensive REST API endpoints for all printer functionality.

**Features:**
- ✅ WiFi management (AP+STA dual mode - unchanged)
- ✅ Text printing with formatting
- ✅ Image printing (BMP with dithering)
- ✅ Barcodes (all types)
- ✅ QR codes
- ✅ Paper control
- ✅ Printer configuration

---

## REST API Endpoints

### WiFi Management (Unchanged)

```
GET  /scan         - Scan available WiFi networks
POST /connect      - Connect to WiFi network (ssid, password)
POST /reset        - Reset to AP+STA mode
```

### Printer Status

```
GET /printer/status
- Returns: {"connected": true/false}
```

### Printer Initialization & Configuration

```
POST /printer/init
- Initialize/reset printer

POST /printer/set-width
- Set paper width in millimeters
- Parameters: width (int, 1-255)
- Common: 58mm (384 dots), 80mm (576 dots)
```

### Text Operations

```
POST /printer/text
- Send plain text
- Parameters: text (string)

POST /printer/text-aligned
- Send aligned text
- Parameters: text (string), align (0=left, 1=center, 2=right)

POST /printer/bold
- Toggle bold formatting
- Parameters: enabled (0 or 1)

POST /printer/underline
- Set underline thickness
- Parameters: thickness (0=off, 1=1-dot, 2=2-dot)

POST /printer/font
- Set font
- Parameters: font (0=Font A, 1=Font B)

POST /printer/text-size
- Set text size
- Parameters: width (1-8), height (1-8)

POST /printer/align
- Set text alignment
- Parameters: align (0=left, 1=center, 2=right)

POST /printer/reset-format
- Reset all text formatting to default
```

### Image Printing

```
POST /printer/image
- Load and print image
- Parameters:
  - path (string): File path (e.g., /sdcard/logo.bmp)
  - dither (int): 0=none, 1=threshold, 2=floyd-steinberg
  - mode (int): 0=normal, 1=double-width, 2=double-height, 3=double-both
```

### Barcode Operations

```
POST /printer/barcode
- Print barcode
- Parameters:
  - data (string): Barcode data
  - type (int): Barcode type
    - 65: UPC-A
    - 66: UPC-E
    - 67: EAN13
    - 68: EAN8
    - 69: CODE39
    - 70: I25
    - 71: CODEBAR
    - 72: CODE93
    - 73: CODE128 (default)
  - width (int): Module width (1-10)
  - height (int): Height in dots (1-255)
  - hri (int): Human readable interpretation
    - 0: None
    - 1: Above
    - 2: Below
    - 3: Both
```

### QR Code Operations

```
POST /printer/qr
- Print QR code
- Parameters:
  - data (string): QR data (max 26 bytes for version 2)
  - size (int): Dot size (1-16)
  - ec (int): Error correction level
    - 48: ~7% (EC-L)
    - 49: ~15% (EC-M, default)
    - 50: ~25% (EC-Q)
    - 51: ~30% (EC-H)
```

### Paper Control

```
POST /printer/feed
- Feed paper
- Parameters: lines (1-255, default 1)

POST /printer/cut
- Cut paper
- Parameters: partial (string) - if "partial", partial cut; else full cut

POST /printer/beep
- Trigger printer beep
```

---

## JSON Response Format

All responses use standard JSON format:

**Success:**
```json
{
  "status": "ok",
  "message": "Operation description"
}
```

**Error:**
```json
{
  "error": "Error description"
}
```

**Status:**
```json
{
  "connected": true
}
```

---

## Usage Examples

### Example 1: Print Receipt

```bash
# Set width
curl -X POST http://192.168.1.100/printer/set-width -d "width=58"

# Initialize printer
curl -X POST http://192.168.1.100/printer/init

# Print header (bold, centered)
curl -X POST http://192.168.1.100/printer/bold -d "enabled=1"
curl -X POST http://192.168.1.100/printer/text-aligned -d "text=RECEIPT&align=1"
curl -X POST http://192.168.1.100/printer/bold -d "enabled=0"

# Print items
curl -X POST http://192.168.1.100/printer/text -d "text=Item%201..................5.00"
curl -X POST http://192.168.1.100/printer/text -d "text=Item%202..................3.50"

# Print QR code
curl -X POST http://192.168.1.100/printer/qr -d "data=https://example.com&size=6&ec=49"

# Feed and cut
curl -X POST http://192.168.1.100/printer/feed -d "lines=3"
curl -X POST http://192.168.1.100/printer/cut
```

### Example 2: Print Image

```bash
curl -X POST http://192.168.1.100/printer/image \
  -d "path=/sdcard/logo.bmp&dither=2&mode=0"
```

### Example 3: Print Barcode

```bash
curl -X POST http://192.168.1.100/printer/barcode \
  -d "data=TXN123456&type=73&width=2&height=80&hri=2"
```

---

## Web Interface

Access the comprehensive web UI at:

**WiFi Settings:** `http://192.168.1.100/` or `http://192.168.100.1/`

**Printer Control:** `http://192.168.1.100/printer` or `http://192.168.100.1/printer`

The UI includes:
- Text tab: Plain and aligned text printing
- Formatting tab: Bold, underline, font, size, alignment
- Barcode/QR tab: Generate barcodes and QR codes
- Image tab: Print images with dithering options
- Paper tab: Feed and cut controls
- Config tab: Printer settings

---

## Architecture

### Components

1. **main.c** - Application entry point
2. **wifi_manager.c/h** - WiFi management (AP+STA)
3. **http_server.c/h** - HTTP REST API server
4. **printer_manager.c/h** - Printer handle and lifecycle management
5. **components/esp_escpos** - ESC/POS protocol library

### Library Integration

The esp_escpos library is included in printer_manager.h. Direct escpos function calls are made on the printer handle obtained via `printer_manager_get_printer()`.

### Thread Safety

All printer operations are protected by FreeRTOS mutexes in the printer_manager layer.

---

## Default Credentials

**WiFi AP:**
- SSID: `ESP32_S3_AP`
- Password: `who-zefa`
- Max Connections: 4

---

## Building & Flashing

```bash
cd /path/to/escpos_webserver

# Build
idf.py build

# Flash
idf.py flash

# Monitor
idf.py monitor
```

---

## Notes

- WiFi functionality remains unchanged from original implementation
- All new endpoints use consistent error handling
- HTML UI is responsive and touch-friendly
- Endpoints are validated before execution (printer must be connected)
- Image printing requires BMP format files on SD card or embedded

---

## API Endpoint Summary (21 Total)

### Web Pages (2)
- GET /
- GET /printer

### WiFi (3)
- GET /scan
- POST /connect
- POST /reset

### Printer Status & Config (3)
- GET /printer/status
- POST /printer/init
- POST /printer/set-width

### Text Operations (7)
- POST /printer/text
- POST /printer/text-aligned
- POST /printer/bold
- POST /printer/underline
- POST /printer/font
- POST /printer/text-size
- POST /printer/align
- POST /printer/reset-format

### Advanced (8)
- POST /printer/image
- POST /printer/barcode
- POST /printer/qr
- POST /printer/feed
- POST /printer/cut
- POST /printer/beep
