# 📷 ESP32-S3-CAM Web Server

A high-performance camera web server for the **GOOUUU ESP32-S3-CAM** module, featuring live MJPEG streaming, photo capture, and a responsive web interface.

<p align="center">
  <img src="https://img.shields.io/badge/ESP32--S3-N16R8-blue" alt="ESP32-S3">
  <img src="https://img.shields.io/badge/Camera-OV2640-green" alt="OV2640">
  <img src="https://img.shields.io/badge/PSRAM-8MB-orange" alt="PSRAM">
  <img src="https://img.shields.io/badge/Platform-PlatformIO-blue" alt="PlatformIO">
</p>

---

## ✨ Features

| Feature | Description |
|---------|-------------|
| 📹 **Live Streaming** | Real-time MJPEG video at 800x600 resolution, 15-20 fps |
| 📸 **Photo Capture** | On-demand JPEG snapshots (~85-95KB per image) |
| 🌐 **Web Interface** | Clean, responsive HTML interface accessible from any device |
| 📱 **Mobile Ready** | Optimized for phones, tablets, and desktop browsers |
| ⚡ **High Performance** | Dual-buffered frames in 8MB PSRAM for smooth streaming |
| 🎨 **Camera Controls** | Adjustable brightness, contrast, saturation, white balance |
| 🔧 **Configurable** | Easy WiFi setup, multiple resolution options |
| 🔒 **Secure Config** | WiFi credentials stored in git-ignored config file |

---

## 🛠 Hardware Specifications

### GOOUUU ESP32-S3-CAM Module

| Component | Specification |
|-----------|--------------|
| **Microcontroller** | ESP32-S3 dual-core Xtensa LX7 @ 240MHz |
| **Flash Memory** | 16MB QSPI Flash |
| **PSRAM** | 8MB Octal SPI PSRAM |
| **Camera Sensor** | OV2640 2MP (1600x1200 max) |
| **WiFi** | 2.4GHz 802.11 b/g/n (2.4GHz only) |
| **Bluetooth** | BLE 5.0 (not used in this project) |
| **USB** | USB-C with CDC serial and JTAG debugging |
| **Power Input** | 5V via USB-C or VIN pin |
| **Operating Voltage** | 3.3V (regulated onboard) |
| **Current Draw** | ~250-300mA during streaming, ~150mA idle |

### 📌 Pin Configuration (CRITICAL)

This specific pin mapping is **verified for GOOUUU ESP32-S3-CAM** and differs from other ESP32-CAM variants:

| Function | GPIO | Type | Description |
|----------|------|------|-------------|
| **SIOD** | `4` | I2C | Camera sensor I2C data line (SDA) |
| **SIOC** | `5` | I2C | Camera sensor I2C clock line (SCL) |
| **XCLK** | `15` | Output | Camera master clock @ 20MHz |
| **PCLK** | `13` | Input | Pixel clock from camera |
| **VSYNC** | `6` | Input | Vertical synchronization signal |
| **HREF** | `7` | Input | Horizontal reference signal |
| **D0** | `11` | Input | Parallel data bit 0 (LSB) |
| **D1** | `9` | Input | Parallel data bit 1 |
| **D2** | `8` | Input | Parallel data bit 2 |
| **D3** | `10` | Input | Parallel data bit 3 |
| **D4** | `12` | Input | Parallel data bit 4 |
| **D5** | `18` | Input | Parallel data bit 5 |
| **D6** | `17` | Input | Parallel data bit 6 |
| **D7** | `16` | Input | Parallel data bit 7 (MSB) |
| **PWDN** | `-1` | N/A | Power down (not connected) |
| **RESET** | `-1` | N/A | Hardware reset (not connected) |

> ⚠️ **Warning**: Using incorrect pins will cause camera initialization failures or system crashes!

---

## 🚀 Quick Start Guide

### Prerequisites

- **Hardware**: GOOUUU ESP32-S3-CAM module
- **Software**: [PlatformIO](https://platformio.org/) (VS Code extension or CLI)
- **USB Cable**: USB-C data cable (not charge-only)
- **WiFi**: 2.4GHz network (5GHz not supported)

### 1️⃣ Clone the Repository

```bash
git clone https://github.com/schultzzznet/ESP32-Camera-Test.git
cd ESP32-Camera-Test
```

### 2️⃣ Configure WiFi Credentials

Create your WiFi configuration file:

```bash
cp src/config.h.example src/config.h
```

Edit `src/config.h` with your WiFi details:

```cpp
#ifndef CONFIG_H
#define CONFIG_H

const char* WIFI_SSID = "YourNetworkName";      // Replace with your SSID
const char* WIFI_PASSWORD = "YourPassword";      // Replace with your password

#endif
```

> 🔒 **Security Note**: `config.h` is automatically git-ignored to protect your credentials.

### 3️⃣ Build and Upload

Using PlatformIO in VS Code:
1. Open project folder
2. Click "Upload" button in PlatformIO toolbar
3. Hold **BOOT** button on ESP32 if upload fails to connect

Or via command line:

```bash
platformio run --target upload --target monitor
```

### 4️⃣ Find Your Camera's IP Address

Watch the serial monitor output (115200 baud) for:

```
================================================
🎥 Camera Server Ready!
================================================
📱 Open in browser: http://192.168.1.13
================================================
```

### 5️⃣ Access the Web Interface

Open your browser and navigate to the displayed IP address:

| Endpoint | Function |
|----------|----------|
| `http://192.168.1.xxx` | Main web interface with controls |
| `http://192.168.1.xxx:81/stream` | Direct MJPEG stream (no HTML) |
| `http://192.168.1.xxx/capture` | Single JPEG snapshot |

---

---

## 📁 Project Structure

```
ESP32-Camera-Test/
├── 📂 src/
│   ├── main.cpp              # Main application code
│   ├── config.h              # WiFi credentials (git-ignored)
│   └── config.h.example      # Template for WiFi configuration
├── 📂 include/               # Header files (empty for now)
├── 📂 lib/                   # Custom libraries (empty for now)
├── 📂 test/                  # Unit tests (empty for now)
├── platformio.ini            # PlatformIO build configuration
├── .gitignore                # Git ignore patterns
└── README.md                 # This documentation
```

### Key Files Explained

| File | Purpose |
|------|---------|
| `src/main.cpp` | Complete web server implementation with camera initialization, HTTP handlers, and WiFi management |
| `src/config.h` | **User-created file** containing WiFi SSID and password (never committed to git) |
| `src/config.h.example` | Template showing the format for `config.h` |
| `platformio.ini` | Build settings, board configuration, dependencies |

---

## ⚙️ Configuration & Customization

### PlatformIO Board Settings

The `platformio.ini` file contains critical configuration:

```ini
[env:esp32cam]
platform = espressif32@6.10.0
board = 4d_systems_esp32s3_gen4_r8n16      # CRITICAL for PSRAM detection
framework = arduino
monitor_speed = 115200
board_build.arduino.memory_type = qio_opi  # Octal PSRAM mode
build_flags = 
    -DBOARD_HAS_PSRAM
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
board_build.partitions = huge_app.csv      # 3MB app partition
```

> ⚠️ **Critical**: The `4d_systems_esp32s3_gen4_r8n16` board definition is essential. Other ESP32-S3 boards may report "No PSRAM" errors.

### Camera Resolution Options

Edit `src/main.cpp` in the `initCamera()` function:

```cpp
config.frame_size = FRAMESIZE_SVGA;  // Current: 800x600
```

Available resolutions:

| Setting | Resolution | Aspect Ratio | Notes |
|---------|-----------|--------------|-------|
| `FRAMESIZE_QQVGA` | 160×120 | 4:3 | Very low quality |
| `FRAMESIZE_QVGA` | 320×240 | 4:3 | Low quality |
| `FRAMESIZE_VGA` | 640×480 | 4:3 | Good for testing |
| `FRAMESIZE_SVGA` | 800×600 | 4:3 | **Default - balanced** |
| `FRAMESIZE_XGA` | 1024×768 | 4:3 | High quality |
| `FRAMESIZE_SXGA` | 1280×1024 | 5:4 | Very high quality |
| `FRAMESIZE_UXGA` | 1600×1200 | 4:3 | Maximum, slower fps |

### Image Quality Settings

Adjust JPEG compression quality (lower = better quality, larger files):

```cpp
config.jpeg_quality = 12;  // Range: 10-63
```

| Quality | File Size | Use Case |
|---------|-----------|----------|
| `10-12` | ~85-95KB | Production (default) |
| `15-20` | ~40-60KB | Good balance |
| `25-35` | ~20-30KB | Lower bandwidth |
| `40-63` | <15KB | Testing only |

### Advanced Camera Sensor Settings

In `initCamera()`, you can adjust:

```cpp
sensor_t *s = esp_camera_sensor_get();

// Image adjustments
s->set_brightness(s, 0);      // -2 to +2
s->set_contrast(s, 0);        // -2 to +2  
s->set_saturation(s, 0);      // -2 to +2

// Auto settings
s->set_whitebal(s, 1);        // 0=off, 1=on (auto white balance)
s->set_awb_gain(s, 1);        // 0=off, 1=on (auto WB gain)
s->set_exposure_ctrl(s, 1);   // 0=off, 1=on (auto exposure)
s->set_gain_ctrl(s, 1);       // 0=off, 1=on (auto gain)

// Orientation
s->set_hmirror(s, 0);         // 0=off, 1=on (horizontal mirror)
s->set_vflip(s, 0);           // 0=off, 1=on (vertical flip)
```

---

## 🔧 Troubleshooting

### Upload & Connection Issues

| Problem | Solution |
|---------|----------|
| **Upload fails** | Hold **BOOT** button during "Connecting..." message, then release |
| **Port not detected** | Ensure USB cable supports data transfer (not charge-only) |
| **Permission denied** (macOS/Linux) | `sudo usermod -a -G dialout $USER` then logout/login |
| **Slow upload** | Normal for ESP32-S3, takes 5-10 seconds |

### Camera Initialization Failures

| Symptom | Possible Cause | Fix |
|---------|---------------|-----|
| "Camera init failed 0x105" | Wrong pins | Verify pin configuration matches GOOUUU board |
| "No PSRAM detected" | Wrong board config | Use `4d_systems_esp32s3_gen4_r8n16` board |
| "Camera capture failed" | Loose ribbon cable | Reseat camera ribbon cable |
| Crash/reboot on init | Insufficient power | Use USB port with 500mA+ capability |

**Verify PSRAM in serial output:**
```
✅ PSRAM: 8386295 bytes (8.00 MB)
```

### WiFi Connection Problems

| Issue | Check |
|-------|-------|
| Won't connect | Verify 2.4GHz network (ESP32 doesn't support 5GHz) |
| "BEACON_TIMEOUT" | Move closer to router, check signal strength |
| "AUTH_FAIL" | Double-check password in `config.h` |
| "ASSOC_LEAVE" | Router may be blocking device, check MAC filtering |
| Connects then disconnects | Disable WiFi power saving on router |

**Monitor WiFi status in serial output:**
```
RSSI: -50 dBm  // Signal strength (>-70 is good)
```

### Web Interface Issues

| Problem | Solution |
|---------|----------|
| Stream shows broken image | Clear browser cache, try Chrome/Edge |
| Stream freezes | Check WiFi stability in serial monitor |
| Can't access from phone | Ensure phone on same network, no VPN |
| Slow loading | Reduce resolution or increase `jpeg_quality` |
| HTTP error 45062 | Network timeout, check WiFi signal |

### Performance Issues

| Symptom | Cause | Improvement |
|---------|-------|-------------|
| Low FPS (<10) | High resolution | Switch to VGA or lower |
| Laggy stream | WiFi interference | Move closer to router |
| Memory errors | Memory leak | Power cycle ESP32 |
| Slow capture | Quality too high | Increase `jpeg_quality` value |

---

## 📊 Performance Metrics

### Measured Performance (SVGA @ Quality 12)

| Metric | Value |
|--------|-------|
| **Frame Rate** | 15-20 fps (local network) |
| **Latency** | 100-200ms end-to-end |
| **JPEG Size** | 85-95KB per frame |
| **Network Bandwidth** | ~1.5-2 Mbps |
| **RAM Usage** | 47KB / 320KB (14%) |
| **Flash Usage** | 770KB / 3MB (24%) |
| **PSRAM Usage** | Dynamic (frame buffers) |
| **Boot Time** | ~3-4 seconds to WiFi |
| **Power Draw** | 250-300mA streaming, 150mA idle |

### Memory Architecture

```
┌─────────────────────────────────────┐
│ ESP32-S3 Memory Layout              │
├─────────────────────────────────────┤
│ Internal RAM (320KB)                │
│  ├─ Firmware code & vars: ~47KB    │
│  └─ Stack & heap: ~273KB free      │
├─────────────────────────────────────┤
│ PSRAM (8MB)                         │
│  ├─ Frame Buffer 1: ~95KB          │
│  ├─ Frame Buffer 2: ~95KB          │
│  └─ Available: ~8.17MB             │
├─────────────────────────────────────┤
│ Flash (16MB)                        │
│  ├─ Bootloader: ~15KB              │
│  ├─ Partition table: ~3KB          │
│  ├─ Application: ~770KB            │
│  └─ Available: ~15.2MB             │
└─────────────────────────────────────┘
```

---

## 🔬 Technical Architecture

### Software Stack

```
┌──────────────────────────────────┐
│  Web Browser (Client)            │
├──────────────────────────────────┤
│  HTTP/MJPEG Stream               │
├──────────────────────────────────┤
│  WiFi (802.11 b/g/n @ 2.4GHz)   │
├──────────────────────────────────┤
│  ESP-IDF HTTP Server             │
│  ├─ Port 80: Main UI + Capture  │
│  └─ Port 81: MJPEG Stream        │
├──────────────────────────────────┤
│  Camera Driver (esp_camera.h)    │
│  ├─ Frame grabber                │
│  ├─ JPEG encoder                 │
│  └─ Buffer management            │
├──────────────────────────────────┤
│  OV2640 Sensor (I2C + Parallel)  │
└──────────────────────────────────┘
```

### Network Configuration

| Setting | Value | Purpose |
|---------|-------|---------|
| **HTTP Server Instances** | 2 | Separate ports for UI and stream |
| **Port 80** | Main server | Web interface + single capture |
| **Port 81** | Stream server | Dedicated MJPEG streaming |
| **Max Sockets** | 7 | Concurrent connections |
| **Send Timeout** | 10s | Prevent hanging on slow clients |
| **Receive Timeout** | 10s | Client request timeout |
| **LRU Purge** | Enabled | Close oldest idle connections |

### Frame Buffer Strategy

```cpp
config.fb_count = 2;                    // Dual buffering
config.fb_location = CAMERA_FB_IN_PSRAM; // Use PSRAM (8MB)
config.grab_mode = CAMERA_GRAB_LATEST;   // Skip old frames
```

**Benefits:**
- Smooth streaming without frame drops
- Latest frame always available
- No internal RAM pressure

---

---

## 🛣 Development Roadmap

- [ ] Web UI improvements (brightness/contrast sliders)
- [ ] Motion detection capability
- [ ] SD card storage for photos
- [ ] Time-lapse mode
- [ ] Multiple camera support
- [ ] WebSocket streaming (lower latency)
- [ ] OTA firmware updates

---

## 📚 Dependencies & Tools

### Build Dependencies

| Package | Version | Purpose |
|---------|---------|---------|
| **PlatformIO** | Latest | Build system and toolchain |
| **Espressif32 Platform** | 6.10.0 | ESP32-S3 SDK and tools |
| **Arduino-ESP32** | 3.20017 | Arduino framework for ESP32 |
| **ESP32-Camera** | Built-in | Camera driver library |

### Required Libraries (Auto-installed)

```ini
lib_deps = 
    Wire          # I2C communication
    WiFi          # WiFi networking
```

### Development Tools

- **VS Code** + PlatformIO Extension (recommended)
- **Serial Monitor** for debugging (115200 baud)
- **Any modern browser** for web interface

---

## 🧪 Testing & Validation

### Serial Output Checklist

Look for these key messages during boot:

```
✅ PSRAM: 8386295 bytes (8.00 MB)           # PSRAM detected
✅ Camera initialized successfully          # Camera ready
✅ Test capture OK: 92796 bytes, 800x600   # Initial test passed
✅ WiFi connected!                          # Network connected
IP Address: http://192.168.1.13            # Your IP address
🎥 Camera Server Ready!                     # All systems go
```

### Quick Tests

1. **Camera Test**: Check for test capture in serial output
2. **WiFi Test**: Verify IP address is displayed
3. **Web UI Test**: Open IP in browser, see camera interface
4. **Stream Test**: Click "Start Stream" button
5. **Capture Test**: Open `/capture` endpoint directly

---

## 🤝 Contributing

Contributions are welcome! Here's how you can help:

### Bug Reports

When reporting issues, please include:
- **Serial output** (full boot log)
- **Hardware version** (check board markings)
- **PlatformIO version**: `pio --version`
- **Steps to reproduce**

### Pull Requests

1. Fork the repository
2. Create a feature branch: `git checkout -b feature/amazing-feature`
3. Commit changes: `git commit -m 'Add amazing feature'`
4. Push to branch: `git push origin feature/amazing-feature`
5. Open a Pull Request

### Code Style

- Follow existing formatting
- Comment complex logic
- Test on actual hardware before submitting

---

## 📄 License

```
MIT License

Copyright (c) 2025 schultzzznet

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```

---

## 🙏 Acknowledgments

- **Espressif Systems** - ESP32-S3 platform and camera drivers
- **ESP32-Camera Library** - Camera integration examples
- **PlatformIO** - Excellent build system and tooling
- **Arduino-ESP32 Community** - Framework and support
- **GOOUUU** - Hardware manufacturer

### Special Thanks

- Pin configuration verified through extensive hardware testing
- Optimized specifically for GOOUUU ESP32-S3-CAM module
- Community feedback and bug reports

---

## 📞 Support & Links

- **Repository**: [github.com/schultzzznet/ESP32-Camera-Test](https://github.com/schultzzznet/ESP32-Camera-Test)
- **Issues**: [GitHub Issues](https://github.com/schultzzznet/ESP32-Camera-Test/issues)
- **Discussions**: [GitHub Discussions](https://github.com/schultzzznet/ESP32-Camera-Test/discussions)
- **ESP32-S3 Docs**: [Espressif Documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/)
- **PlatformIO**: [platformio.org](https://platformio.org/)

---

## ⚡ Quick Reference

### Common Commands

```bash
# Build only
platformio run

# Upload to device
platformio run --target upload

# Open serial monitor
platformio device monitor

# Build + Upload + Monitor
platformio run --target upload --target monitor

# Clean build files
platformio run --target clean

# Update dependencies
platformio pkg update
```

### Common URLs (replace IP)

```
http://192.168.1.13/          # Main interface
http://192.168.1.13:81/stream # Stream only
http://192.168.1.13/capture   # Single photo
```

### File Paths

```
src/config.h        # Your WiFi credentials
src/main.cpp        # Main application code
platformio.ini      # Build configuration
```

---

<p align="center">
  <b>Made with ❤️ for the ESP32 community</b><br>
  <sub>If this project helped you, consider giving it a ⭐ on GitHub!</sub>
</p>


