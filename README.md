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
| 📸 **Photo Capture** | On-demand JPEG snapshots with dynamic resolution switching |
| 🔄 **Dynamic Resolution** | Change resolution on-the-fly via URL parameters (QVGA/VGA/SVGA) |
| 🌐 **Web Interface** | Clean, responsive HTML interface accessible from any device |
| 📱 **Mobile Ready** | Optimized for phones, tablets, and desktop browsers |
| ⚡ **High Performance** | Dual-buffered frames in 8MB PSRAM for smooth streaming |
| 🎨 **Camera Controls** | Adjustable brightness, contrast, saturation, white balance |
| 🔧 **Configurable** | Easy WiFi setup, multiple resolution options |
| 🔒 **Secure Config** | WiFi credentials stored in git-ignored config file |
| 🛠 **OV2640 Workaround** | RGB565 format with software JPEG encoding bypasses hardware bugs |

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
| `http://192.168.1.xxx/capture` | Single JPEG snapshot (default SVGA) |
| `http://192.168.1.xxx/capture?res=vga` | Capture at VGA resolution (640x480) |
| `http://192.168.1.xxx/capture?res=qvga` | Capture at QVGA resolution (320x240) |
| `http://192.168.1.xxx/capture?res=svga` | Capture at SVGA resolution (800x600) |

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

**Default Resolution**: Set in `src/main.cpp` in the `initCamera()` function:

```cpp
config.frame_size = FRAMESIZE_SVGA;  // Default: 800x600
```

**Dynamic Resolution Switching**: Change resolution per capture request using URL parameter:

```bash
curl "http://192.168.1.29/capture?res=vga" -o photo_640x480.jpg
curl "http://192.168.1.29/capture?res=qvga" -o photo_320x240.jpg
curl "http://192.168.1.29/capture?res=svga" -o photo_800x600.jpg
```

#### ⚠️ Supported Resolutions (RGB565 Format)

Due to **OV2640 hardware JPEG encoder bugs**, this project uses **RGB565 + software JPEG encoding**. This limits maximum resolution to **SVGA (800x600)** due to buffer size constraints:

| Setting | Resolution | Buffer Size | Status | Use Case |
|---------|-----------|-------------|--------|----------|
| `FRAMESIZE_96X96` | 96×96 | 18KB | ✅ **Works** | Thumbnail |
| `FRAMESIZE_QQVGA` | 160×120 | 38KB | ✅ **Works** | Very low bandwidth |
| `FRAMESIZE_QCIF` | 176×144 | 51KB | ✅ **Works** | Legacy format |
| `FRAMESIZE_QVGA` | 320×240 | 154KB | ✅ **Works** | Low bandwidth (~2KB JPEG) |
| `FRAMESIZE_CIF` | 400×296 | 237KB | ✅ **Works** | Medium quality |
| `FRAMESIZE_VGA` | 640×480 | 614KB | ✅ **Works** | Good quality (~6KB JPEG) |
| `FRAMESIZE_SVGA` | 800×600 | 960KB | ✅ **Works** | **Best quality (~10KB JPEG)** |
| `FRAMESIZE_XGA` | 1024×768 | 1.57MB | ❌ **Crashes** | Stack overflow |
| `FRAMESIZE_HD` | 1280×720 | 1.84MB | ❌ **Crashes** | Stack overflow |
| `FRAMESIZE_SXGA` | 1280×1024 | 2.62MB | ❌ **Crashes** | Stack overflow |
| `FRAMESIZE_UXGA` | 1600×1200 | 3.84MB | ❌ **Crashes** | Stack overflow |

> 🔴 **Important**: Resolutions above SVGA cause stack canary watchpoint errors due to RGB565 buffer size exceeding camera task stack allocation. The camera will crash and reboot if XGA or higher is requested.

#### Technical Details: Why RGB565 Format?

The OV2640's **hardware JPEG encoder has a firmware bug** that produces malformed JPEG headers (`FF D8 FF 10` instead of `FF D8 FF E0`). Attempts to patch headers or manipulate quantization tables all failed validation.

**Solution**: 
- Capture in **PIXFORMAT_RGB565** (raw uncompressed format)
- Use ESP32's **software JPEG encoder** (`frame2jpg()` from `img_converters.h`)
- Produces **100% valid JPEGs** that pass `jpeginfo` validation
- Trade-off: Higher memory usage limits maximum resolution to SVGA

**Performance Impact**:
- SVGA (800x600): ~960KB RGB565 → ~10KB JPEG in 420ms
- VGA (640x480): ~614KB RGB565 → ~6KB JPEG in 270ms  
- QVGA (320x240): ~154KB RGB565 → ~2KB JPEG in 65ms

### Image Quality Settings

Adjust JPEG compression quality in the software encoder (lower = better quality, larger files):

```cpp
// In capture_handler() function
if (!frame2jpg_cb(fb, 80, jpg_encode_stream, &jctx, 12)) {
    //                                              ^^ Quality setting
}
```

**Software JPEG Encoder Quality** (RGB565 → JPEG conversion):

| Quality | SVGA File Size | VGA File Size | QVGA File Size | Use Case |
|---------|----------------|---------------|----------------|----------|
| `10-12` | ~9-10KB | ~6KB | ~2KB | **Production (default)** |
| `15-20` | ~7-8KB | ~4-5KB | ~1.5KB | Good balance |
| `25-35` | ~5-6KB | ~3KB | ~1KB | Lower bandwidth |
| `40-63` | ~3-4KB | ~2KB | <1KB | Testing only |

> 💡 **Note**: Quality values are for the **software JPEG encoder**, not the OV2640 hardware encoder (which is bypassed due to bugs).

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
| Slow capture | RGB565 conversion | Normal for software JPEG (420ms @ SVGA) |
| Crash on XGA/HD/UXGA | Buffer too large | **Use SVGA or lower only** |
| "Stack canary watchpoint" | Resolution too high | Limit to SVGA maximum |

---

## 📊 Performance Metrics

### Measured Performance (RGB565 + Software JPEG Encoding)

#### SVGA Resolution (800x600, Quality 12)

| Metric | Value |
|--------|-------|
| **Capture Time** | 420ms (RGB565 → JPEG conversion) |
| **JPEG Size** | ~10KB per frame |
| **Compression Ratio** | 1.0% (960KB RGB565 → 10KB JPEG) |
| **Network Throughput** | 87-93 KB/s |
| **RAM Usage** | 55KB / 320KB (16.9%) |
| **Flash Usage** | 792KB / 3MB (25.2%) |
| **PSRAM Buffer** | 960KB (RGB565 frame buffer) |

#### VGA Resolution (640x480, Quality 12)

| Metric | Value |
|--------|-------|
| **Capture Time** | 270ms (RGB565 → JPEG conversion) |
| **JPEG Size** | ~6KB per frame |
| **Compression Ratio** | 1.0% (614KB RGB565 → 6KB JPEG) |
| **Network Throughput** | 78 KB/s |
| **PSRAM Buffer** | 614KB (RGB565 frame buffer) |

#### QVGA Resolution (320x240, Quality 12)

| Metric | Value |
|--------|-------|
| **Capture Time** | 65ms (RGB565 → JPEG conversion) |
| **JPEG Size** | ~2KB per frame |
| **Compression Ratio** | 1.4% (154KB RGB565 → 2KB JPEG) |
| **Network Throughput** | 695 KB/s |
| **PSRAM Buffer** | 154KB (RGB565 frame buffer) |

#### System Resources

| Metric | Value |
|--------|-------|
| **Boot Time** | ~3-4 seconds to WiFi |
| **WiFi Stability** | 802.11b/g, Google WiFi optimized |
| **Power Draw** | 250-300mA active, 150mA idle |
| **Heap Free** | 222KB during operation |

### Memory Architecture (RGB565 Mode)

```
┌─────────────────────────────────────┐
│ ESP32-S3 Memory Layout              │
├─────────────────────────────────────┤
│ Internal RAM (320KB)                │
│  ├─ Firmware code & vars: ~55KB    │
│  └─ Stack & heap: ~265KB free      │
├─────────────────────────────────────┤
│ PSRAM (8MB) - RGB565 Buffers        │
│  ├─ Frame Buffer 1: 960KB (SVGA)   │
│  ├─ Frame Buffer 2: 960KB (SVGA)   │
│  └─ Available: ~6.1MB              │
│                                     │
│  Buffer sizes by resolution:       │
│  • QVGA: 154KB × 2 = 308KB         │
│  • VGA:  614KB × 2 = 1.2MB         │
│  • SVGA: 960KB × 2 = 1.9MB         │
│  • XGA:  1.57MB × 2 = 3.1MB (⚠️)   │
├─────────────────────────────────────┤
│ Flash (16MB)                        │
│  ├─ Bootloader: ~15KB              │
│  ├─ Partition table: ~3KB          │
│  ├─ Application: ~792KB            │
│  └─ Available: ~15.2MB             │
└─────────────────────────────────────┘

Note: XGA and higher resolutions exceed 
camera task stack limits and will crash.
```

---

## 🔬 Technical Architecture

### Software Stack (RGB565 + Software JPEG)

```
┌──────────────────────────────────────┐
│  Web Browser (Client)                │
├──────────────────────────────────────┤
│  HTTP/MJPEG Stream                   │
├──────────────────────────────────────┤
│  WiFi (802.11 b/g @ 2.4GHz)         │
│  • Google WiFi optimized             │
│  • Channel pre-scan                  │
│  • Power save disabled               │
├──────────────────────────────────────┤
│  ESP-IDF HTTP Server                 │
│  ├─ Port 80: UI + Capture (dynamic) │
│  └─ Port 81: MJPEG Stream           │
├──────────────────────────────────────┤
│  Software JPEG Encoder ⭐ NEW        │
│  • frame2jpg() from img_converters  │
│  • Bypasses OV2640 hardware bugs    │
│  • 100% valid JPEG output           │
│  • Quality adjustable (10-63)       │
├──────────────────────────────────────┤
│  Camera Driver (esp_camera.h)        │
│  ├─ RGB565 frame grabber            │
│  ├─ Dynamic resolution switching     │
│  ├─ Camera deinit/reinit support    │
│  └─ PSRAM buffer management         │
├──────────────────────────────────────┤
│  OV2640 Sensor (I2C + Parallel)      │
│  • RGB565 raw format                │
│  • Hardware JPEG encoder disabled   │
└──────────────────────────────────────┘
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

### Frame Buffer Strategy (RGB565 Mode)

```cpp
config.pixel_format = PIXFORMAT_RGB565;  // Raw RGB format (bypasses JPEG bugs)
config.frame_size = FRAMESIZE_SVGA;      // 800x600 default
config.fb_count = 2;                     // Dual buffering
config.fb_location = CAMERA_FB_IN_PSRAM; // Use PSRAM (8MB)
config.grab_mode = CAMERA_GRAB_LATEST;   // Skip old frames
```

**Benefits:**
- ✅ **100% valid JPEG output** (software encoder)
- ✅ Smooth streaming without frame drops
- ✅ Latest frame always available
- ✅ No internal RAM pressure
- ✅ Dynamic resolution switching via camera reinit

**Trade-offs:**
- ⚠️ Larger buffer sizes (960KB vs ~90KB for hardware JPEG)
- ⚠️ Software JPEG encoding adds 65-420ms per capture
- ⚠️ Maximum resolution limited to SVGA (800x600)
- ⚠️ Higher resolutions (XGA+) cause stack overflow crashes

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


