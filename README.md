# ESP32-S3 WiFi & USB Printer Manager

[![ESP-IDF](https://img.shields.io/badge/ESP--IDF-v5.5-blue)](https://github.com/espressif/esp-idf)
[![ESP32-S3](https://img.shields.io/badge/Target-ESP32--S3-brightgreen)](https://www.espressif.com/en/products/socs/esp32-s3)
[![License](https://img.shields.io/badge/License-MIT-yellow)](LICENSE)

A comprehensive firmware solution for ESP32-S3 that provides WiFi management and USB thermal printer (ESC/POS) control through an intuitive web interface.

##  Features

### WiFi Management
- **Dual-Mode WiFi** - AP (Access Point) + STA (Station) simultaneously
- **Network Scanning** - Discover available WiFi networks with signal strength
- **Easy Connection** - One-click connection to WiFi networks
- **Manual Configuration** - Direct SSID and password entry
- **Reset Functionality** - Quick reset to default AP+STA mode
- **Auto-Reconnection** - Automatic reconnection on disconnection

### USB Printer Control
- **ESC/POS Support** - Full compatibility with thermal printers
- **Text Printing** - Send formatted text to printer
- **Paper Control** - Feed paper and partial/full cut operations
- **Printer Status** - Real-time printer connection monitoring
- **Beep Control** - Trigger printer beep for feedback
- **Initialization** - Reset printer to default state
- **Thread-Safe** - Mutex-protected concurrent access

### Web Interface
- **Responsive Design** - Works on desktop, tablet, and mobile
- **Dark Theme UI** - Easy on the eyes, modern appearance
- **Real-Time Status** - Live printer and WiFi connection indicators
- **Auto-Refresh** - Automatic status updates every 3-5 seconds
- **Keyboard Shortcuts** - Ctrl+Enter to send text
- **Error Handling** - Clear feedback for all operations

##  Requirements

### Hardware
- **ESP32-S3 Development Board** (or compatible)
- **USB Thermal Printer** (ESC/POS compatible)
  - Recommended: 58mm or 80mm thermal printer
  - Examples: Epson TM-T20, Star TSP100, BIXOLON SRP-Q series
- **USB Cable** - USB-B to USB-A or matching connector
- **USB Hub** (optional) - For multiple USB devices

### Software
- **ESP-IDF** v5.1 or later
- **Python** 3.7 or later
- **Git** (for cloning the repository)

### Dependencies
The project uses the following ESP-IDF components:
- `esp_wifi` - WiFi driver
- `esp_netif` - Network interface
- `esp_event` - Event system
- `esp_http_server` - HTTP server
- `usb` - USB driver
- `usb_host_cdc_acm` - USB CDC-ACM support
- `nvs_flash` - Non-volatile storage

##  Quick Start

### Installation

1. **Clone the Repository**
   ```bash
   git clone https://github.com/your-username/esp32-printer-manager.git
   cd esp32-printer-manager
   ```

2. **Set ESP-IDF Environment**
   ```bash
   # On Windows (PowerShell)
   $env:IDF_PATH = 'C:\path\to\esp-idf'
   
   # On Linux/macOS
   export IDF_PATH=~/esp/esp-idf
   source $IDF_PATH/export.sh
   ```

3. **Build the Project**
   ```bash
   idf.py set-target esp32s3
   idf.py build
   ```

4. **Flash to Device**
   ```bash
   idf.py -p COM3 flash monitor
   ```
   Replace `COM3` with your actual serial port.

### Initial Setup

1. **Connect to ESP32 Access Point**
   - Look for WiFi network: `ESP32-AP`
   - Password: `12345678`

2. **Open Web Interface**
   - Open browser and navigate to: `http://192.168.4.1/`

3. **Configure WiFi (Optional)**
   - Click "Scan" to find available networks
   - Select a network and enter password
   - Device will connect and maintain connection

4. **Control Printer**
   - Navigate to `http://192.168.4.1/printer`
   - Printer must be connected via USB
   - Send commands through the web interface

##  Web Interface

### WiFi Manager Page (`/`)
Located at `http://192.168.4.1/`

- **Network List** - Shows all available WiFi networks
- **Signal Strength** - RSSI signal indicator
- **Quick Connect** - Click to connect to network
- **Manual Entry** - Enter SSID and password manually
- **Reset Button** - Reset to AP+STA mode

### Printer Manager Page (`/printer`)
Located at `http://192.168.4.1/printer`

**Status Section**
- Green indicator when printer is connected
- Red indicator when printer is disconnected

**Print Text Section**
- Text input area (max 256 characters)
- "Send to Printer" button
- Character count validation

**Printer Controls**
- Initialize - Reset printer to default state
- Beep - Trigger audio feedback

**Paper Controls**
- Feed Paper - Advance paper (1-255 lines)
- Partial Cut - Semi-cut paper
- Full Cut - Complete paper cut

##  REST API

### WiFi Endpoints

```bash
# Get available networks
GET /scan

# Connect to network
POST /connect
Content-Type: application/x-www-form-urlencoded

ssid=MyNetwork&password=MyPassword

# Reset to AP+STA
POST /reset
```

### Printer Endpoints

```bash
# Get printer connection status
GET /printer/status

# Send text to printer
POST /printer/print
Content-Type: application/x-www-form-urlencoded

text=Your%20text%20here

# Feed paper
POST /printer/feed
Content-Type: application/x-www-form-urlencoded

lines=5

# Cut paper
POST /printer/cut
Content-Type: application/x-www-form-urlencoded

partial=1    # partial=1 for partial cut, partial=0 for full cut

# Initialize printer
POST /printer/init

# Beep printer
POST /printer/beep
```

### Response Format

All endpoints return JSON responses:

**Success Response**
```json
{
  "status": "ok"
}
```

**Status Response**
```json
{
  "connected": true
}
```

**Error Response**
```json
{
  "error": "Printer not connected"
}
```

##  Configuration

### WiFi Configuration
Edit `main/wifi_manager.h`:

```c
#define AP_SSID      "ESP32-AP"      // Access Point name
#define AP_PASS      "12345678"      // Access Point password
#define AP_CHANNEL   1               // WiFi channel (1-13)
#define AP_MAX_CONN  4               // Max simultaneous connections
```

### Printer Configuration
Edit `components/esp_escpos/include/escpos_config.h`:

```c
#define ESCPOS_USB_VID             0x0000  // USB Vendor ID (0x0000 = any)
#define ESCPOS_USB_PID             0x0000  // USB Product ID (0x0000 = any)
#define ESCPOS_USB_CONNECT_TIMEOUT_MS 2000 // USB connection timeout
#define ESCPOS_USB_OUT_BUFFER_SIZE 512     // USB output buffer size
#define ESCPOS_USB_IN_BUFFER_SIZE  64      // USB input buffer size
```

### HTTP Server Configuration
Edit `main/http_server.c`:

```c
config.max_uri_handlers = 17;  // Increase for more endpoints
```

##  Project Structure

```
escpos_webserver/
├── main/
│   ├── main.c                 # Application entry point
│   ├── main.h                 # Main header
│   ├── http_server.c          # HTTP server implementation
│   ├── http_server.h          # HTTP server interface
│   ├── wifi_manager.c         # WiFi management
│   ├── wifi_manager.h         # WiFi interface
│   ├── printer_manager.c      # Printer manager implementation
│   ├── printer_manager.h      # Printer manager interface
│   ├── index.html             # WiFi manager UI
│   ├── printer.html           # Printer manager UI
│   ├── CMakeLists.txt         # Component build configuration
│   └── idf_component.yml      # Component manifest
├── components/
│   └── esp_escpos/            # ESC/POS printer library
│       ├── src/               # Library source files
│       ├── include/           # Library headers
│       └── CMakeLists.txt     # Component configuration
├── CMakeLists.txt             # Project configuration
├── sdkconfig                  # ESP-IDF configuration
└── README.md                  # This file
```

##  Building & Development

### Build Commands

```bash
# Full build
idf.py build

# Clean build
idf.py fullclean && idf.py build

# Build with specific target
idf.py set-target esp32s3
idf.py build

# Monitor serial output
idf.py monitor

# Combined build, flash, and monitor
idf.py build flash monitor
```

### Development Tips

1. **Serial Monitor Output**
   ```bash
   idf.py monitor -p COM3
   ```

2. **View Build Logs**
   ```bash
   idf.py build 2>&1 | tee build.log
   ```

3. **Configure Project**
   ```bash
   idf.py menuconfig
   ```

4. **Check Component Dependencies**
   ```bash
   idf.py dependency-tree
   ```

##  Troubleshooting

### Printer Not Detected

1. **Check USB Connection**
   - Verify USB cable is properly connected
   - Try a different USB port or cable
   - Ensure printer is powered on

2. **View Printer Logs**
   ```bash
   idf.py monitor | grep -i "escpos\|usb"
   ```

3. **Verify USB Host**
   - Check ESP-IDF USB host driver logs
   - Ensure CDC-ACM driver is enabled in sdkconfig

### WiFi Connection Issues

1. **Can't Connect to Access Point**
   - Check SSID and password in `wifi_manager.h`
   - Verify WiFi antenna is properly connected
   - Check for channel conflicts

2. **Can't Access Web Interface**
   - Verify IP address: `192.168.4.1`
   - Check device logs for network errors
   - Try accessing from different browser/device

### Build Errors

1. **Missing Dependencies**
   ```bash
   idf.py reconfigure
   idf.py build
   ```

2. **Port Already in Use**
   ```bash
   idf.py -p COM3 erase_flash
   idf.py -p COM3 flash
   ```

##  API Documentation

### Thread Safety
All printer manager functions are thread-safe and protected by FreeRTOS mutexes. Operations timeout after 1 second to prevent deadlocks.

### Error Codes
- `ESP_OK` - Operation successful
- `ESP_ERR_INVALID_STATE` - Printer not initialized or unavailable
- `ESP_ERR_TIMEOUT` - Operation timed out
- `ESP_ERR_INVALID_ARG` - Invalid arguments provided

### Memory Requirements
- Flash: ~500KB (firmware + HTML pages)
- RAM: ~100KB runtime allocation

##  Security Considerations

⚠️ **Important**: This firmware is designed for local network use only.

- Default WiFi credentials are publicly known
- No authentication on HTTP endpoints
- Consider implementing:
  - HTTP Basic Authentication
  - HTTPS/TLS encryption
  - Custom access control

For production deployment:
1. Change default WiFi password
2. Implement authentication
3. Use HTTPS with self-signed certificates
4. Implement input validation and sanitization

##  License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

##  Contributing

Contributions are welcome! Please follow these guidelines:

1. **Fork the Repository**
   ```bash
   git clone https://github.com/your-username/esp32-printer-manager.git
   ```

2. **Create Feature Branch**
   ```bash
   git checkout -b feature/amazing-feature
   ```

3. **Commit Changes**
   ```bash
   git commit -m 'Add amazing feature'
   ```

4. **Push to Branch**
   ```bash
   git push origin feature/amazing-feature
   ```

5. **Open Pull Request**

### Code Style
- Follow ESP-IDF coding conventions
- Use Doxygen-style comments for functions
- Maintain consistent formatting
- Test your changes before submitting

##  References

- [ESP-IDF Documentation](https://docs.espressif.com/projects/esp-idf/en/stable/)
- [ESP32-S3 Datasheet](https://www.espressif.com/en/products/socs/esp32-s3)
- [ESC/POS Reference Manual](https://en.wikipedia.org/wiki/ESC/P)
- [FreeRTOS Documentation](https://www.freertos.org/)

##  Support

- **Issues**: Please create an issue on GitHub with detailed description
- **Documentation**: Check the [docs](docs/) folder for additional guides
- **Discussions**: Use GitHub Discussions for questions and ideas

##  Learning Resources

This project demonstrates:
- ESP32 WiFi programming with AP+STA mode
- HTTP server implementation with ESP-IDF
- USB communication with thermal printers
- Thread-safe embedded programming with FreeRTOS
- Web UI development for embedded systems
- RESTful API design for IoT devices

##  Changelog

### v1.0.0 (2026-05-18)
- Initial release
- WiFi management (AP+STA mode)
- USB printer control via ESC/POS library
- Web-based user interface
- REST API endpoints
- Thread-safe printer manager

##  Acknowledgments

- [Espressif](https://www.espressif.com/) - ESP32-S3 and ESP-IDF
- [ESC/POS Printer Manufacturer Community](https://en.wikipedia.org/wiki/ESC/P)
- Open source community contributors

---

**Last Updated**: 2026-05-18  
**Maintained By**: Who-zefa 
**Status**: Active Development
