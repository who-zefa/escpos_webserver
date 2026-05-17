# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added
- Support for additional ESC/POS commands
- WebSocket support for real-time updates
- HTTPS/TLS support with self-signed certificates
- Printer presets for common thermal printers
- Print history and logs

### Changed
- Improved USB device detection
- Enhanced error messages and logging
- Updated UI with better mobile responsiveness

### Fixed
- USB disconnection handling edge cases
- Memory leak in printer manager
- WiFi reconnection timing issues

## [1.0.0] - 2026-05-18

### Added
- Initial release
- WiFi management with AP+STA dual-mode support
- WiFi network scanning with RSSI signal strength
- Easy WiFi connection interface
- USB printer control via ESC/POS protocol
- Web-based user interface with real-time status
- REST API endpoints for WiFi and printer control
- Thread-safe printer manager with mutex protection
- Printer operations: print text, feed paper, cut paper, beep
- Responsive design supporting desktop and mobile
- Auto-refresh functionality for status monitoring
- Keyboard shortcuts for web interface
- Comprehensive documentation and API reference
- MIT License
- Contributing guidelines

### Features

#### WiFi Management
- Access Point (AP) mode for local connections
- Station (STA) mode for external network connectivity
- Network scanning with security information
- One-click connection to available networks
- Manual SSID and password entry
- Quick reset to default configuration
- Auto-reconnection on disconnection
- Real-time connection status display

#### USB Printer Control
- ESC/POS printer protocol support
- Text printing with encoding support
- Paper feed control (1-255 lines)
- Full and partial cut operations
- Printer initialization
- Beep control for feedback
- Real-time connection status monitoring
- Thread-safe concurrent access

#### Web Interface
- Responsive dark-themed UI
- Real-time status indicators
- Live network discovery
- Printer command interface
- Error and success notifications
- Auto-updating status (3-5 second refresh)
- Keyboard shortcuts (Ctrl+Enter)
- Mobile-optimized layout

#### API
- 11 REST endpoints
- JSON request/response format
- Standard HTTP status codes
- Comprehensive error messages
- URL-encoded form data support

### Technical Details
- Built on ESP-IDF v5.5
- Targets ESP32-S3
- Uses FreeRTOS for task management
- Embedded HTML/CSS/JavaScript UI
- Binary data embedding for web pages
- Modular component architecture

### Supported Hardware
- ESP32-S3 Development Board
- ESC/POS compatible thermal printers
  - 58mm printers
  - 80mm printers
  - USB-connected models
- USB Hub support

### Known Limitations
- Default WiFi credentials are public
- No authentication on HTTP endpoints
- Not recommended for production without modifications
- USB printer must be powered on when connected

---

## Version Numbering

We follow Semantic Versioning:
- MAJOR version for incompatible API changes
- MINOR version for new backwards-compatible functionality
- PATCH version for backwards-compatible bug fixes

## Future Roadmap

### Short Term (v1.1.0)
- [ ] Add authentication to HTTP endpoints
- [ ] Implement HTTPS support
- [ ] Add printer presets for common models
- [ ] Improve error recovery

### Medium Term (v1.2.0)
- [ ] WebSocket support for real-time updates
- [ ] Print job history and logging
- [ ] Advanced text formatting options
- [ ] Multi-printer support

### Long Term (v2.0.0)
- [ ] MQTT integration
- [ ] Cloud connectivity
- [ ] Scheduled printing
- [ ] Barcode and QR code generation
- [ ] Multiple language support

---

**Last Updated**: 2026-05-18

For more information, see [README.md](README.md) and [CONTRIBUTING.md](CONTRIBUTING.md).
