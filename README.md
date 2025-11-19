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
| 🔄 **Dual-Mode System** | **RGB565 + software JPEG** (≤SVGA) + **Hardware JPEG** (XGA+) |
| 🎯 **Full Resolution** | All 7 resolutions supported: QVGA through **UXGA (1600×1200)** |
| 🌐 **Web Interface** | Clean, responsive HTML interface accessible from any device |
| 📱 **Mobile Ready** | Optimized for phones, tablets, and desktop browsers |
| ⚡ **High Performance** | Dual-buffered frames in 8MB PSRAM for smooth streaming |
| 🎨 **Camera Controls** | Adjustable brightness, contrast, saturation, white balance |
| 🔧 **Configurable** | Easy WiFi setup, 7 resolution options (320×240 to 1600×1200) |
| 🔒 **Secure Config** | WiFi credentials stored in git-ignored config file |
| 🛠 **OV2640 Workaround** | Dual-mode architecture bypasses hardware JPEG encoder bugs |

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
| `http://192.168.1.xxx/capture?res=qvga` | Capture at QVGA (320×240) - RGB565 mode |
| `http://192.168.1.xxx/capture?res=vga` | Capture at VGA (640×480) - RGB565 mode |
| `http://192.168.1.xxx/capture?res=svga` | Capture at SVGA (800×600) - RGB565 mode |
| `http://192.168.1.xxx/capture?res=xga` | Capture at XGA (1024×768) - Hardware JPEG |
| `http://192.168.1.xxx/capture?res=hd` | Capture at HD (1280×720) - Hardware JPEG |
| `http://192.168.1.xxx/capture?res=sxga` | Capture at SXGA (1280×1024) - Hardware JPEG |
| `http://192.168.1.xxx/capture?res=uxga` | Capture at UXGA (1600×1200) - Hardware JPEG |

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

#### ✅ Dual-Mode Resolution Support

This project uses an **intelligent dual-mode system** to bypass OV2640 hardware JPEG bugs while utilizing the full 2MP sensor capability:

**Mode Selection Strategy:**
- **Resolutions ≤ SVGA (800×600)**: RGB565 format + software JPEG encoding
- **Resolutions > SVGA**: Hardware JPEG encoder + header patching

| Resolution | Mode | JPEG Size | Status | Use Case |
|-----------|------|-----------|--------|----------|
| **QVGA** (320×240) | RGB565 | ~2.8KB | ✅ **Validated** | Low bandwidth |
| **VGA** (640×480) | RGB565 | ~7.7KB | ✅ **Validated** | Good quality |
| **SVGA** (800×600) | RGB565 | ~11KB | ✅ **Validated** | Best for streaming |
| **XGA** (1024×768) | Hardware JPEG | ~46KB | ✅ **Validated** | High quality |
| **HD** (1280×720) | Hardware JPEG | ~52KB | ✅ **Validated** | Widescreen |
| **SXGA** (1280×1024) | Hardware JPEG | ~81KB | ✅ **Validated** | High detail |
| **UXGA** (1600×1200) | Hardware JPEG | ~138KB | ✅ **Validated** | **Full 2MP sensor** |

> ✅ **All resolutions validated** with `jpeginfo` - 100% compliant JPEGs
> ⚠️ **Network Note**: High-resolution transfers (UXGA) may take 30-60 seconds on slow WiFi (3-5 KB/s)

#### Technical Details: Dual-Mode Architecture

The OV2640's **hardware JPEG encoder has a firmware bug** that produces malformed JPEG headers (`FF D8 FF 10` instead of `FF D8 FF E0`). The dual-mode system provides the best of both worlds:

**🔵 RGB565 Mode (≤ SVGA):**
- Captures in **PIXFORMAT_RGB565** (raw uncompressed format)
- Uses ESP32's **software JPEG encoder** (`frame2jpg()` from `img_converters.h`)
- Produces **100% valid JPEGs** with no header issues
- Best for streaming and medium resolutions
- Buffer size: 154KB (QVGA) to 960KB (SVGA)

**🟢 Hardware JPEG Mode (XGA+):**
- Uses OV2640's **hardware JPEG encoder** for efficiency
- Applies **automatic header patching** (FF D8 FF 10 → FF D8 FF E0)
- Enables full **2MP sensor capability** (1600×1200 UXGA)
- Efficient encoding with smaller file sizes
- No RGB565 buffer overhead

**Performance Comparison**:
- SVGA RGB565: ~960KB buffer → ~11KB JPEG in 420ms
- XGA Hardware: Direct JPEG → ~46KB in 180ms  
- UXGA Hardware: Direct JPEG → ~138KB in 350ms

**Automatic Mode Switching:**
```cpp
bool shouldUseRGB565Mode(framesize_t fs) {
    return fs <= FRAMESIZE_SVGA;  // Auto-select based on resolution
}
```

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
| Low FPS (<10) | High resolution | Use SVGA or lower for streaming |
| Laggy stream | WiFi interference | Move closer to router |
| Memory errors | Memory leak | Power cycle ESP32 |
| Slow capture (RGB565) | Software encoding | Normal (65-420ms based on resolution) |
| Slow transfer (UXGA) | WiFi speed | Normal on slow networks (30-60s @ 3KB/s) |
| Timeout on high-res | Network speed | Increase HTTP timeout in code |
| Mode switch delay | Camera reinit | Normal 500ms stabilization time |

---

## 📊 Performance Metrics

### Measured Performance (Dual-Mode System)

#### RGB565 Mode Performance

| Resolution | Capture Time | JPEG Size | Network Speed | Buffer Size |
|-----------|-------------|-----------|---------------|-------------|
| **QVGA** (320×240) | 65ms | ~2.8KB | 21 KB/s | 154KB |
| **VGA** (640×480) | 270ms | ~7.7KB | 27 KB/s | 614KB |
| **SVGA** (800×600) | 420ms | ~11KB | 21 KB/s | 960KB |

#### Hardware JPEG Mode Performance

| Resolution | Capture Time | JPEG Size | Network Speed | Transfer Time |
|-----------|-------------|-----------|---------------|---------------|
| **XGA** (1024×768) | 180ms | ~46KB | 4.9 KB/s | ~10s |
| **HD** (1280×720) | 220ms | ~52KB | 1.3 KB/s | ~40s |
| **SXGA** (1280×1024) | 300ms | ~81KB | 4.5 KB/s | ~18s |
| **UXGA** (1600×1200) | 350ms | ~138KB | 3.2 KB/s | ~45s |

> ⚠️ **Note**: Network speeds shown reflect WiFi limitations (3-5 KB/s on Netgear WNR3500L v2), not ESP32 performance. HTTP timeouts set to 120s to accommodate slow transfers.

#### System Resources

| Metric | Value |
|--------|-------|
| **RAM Usage** | 55KB / 320KB (16.9%) |
| **Flash Usage** | 792KB / 3MB (25.2%) |
| **Boot Time** | ~3-4 seconds to WiFi |
| **WiFi Stability** | 802.11b/g, Netgear WNR3500L v2 |
| **Power Draw** | 250-300mA active, 150mA idle |
| **Heap Free** | 222KB during operation |
| **HTTP Timeout** | 120 seconds (recv/send) |

### Memory Architecture (Dual-Mode System)

```
┌─────────────────────────────────────┐
│ ESP32-S3 Memory Layout              │
├─────────────────────────────────────┤
│ Internal RAM (320KB)                │
│  ├─ Firmware code & vars: ~55KB    │
│  └─ Stack & heap: ~265KB free      │
├─────────────────────────────────────┤
│ PSRAM (8MB) - Dynamic Buffers       │
│                                     │
│  RGB565 Mode (≤SVGA):              │
│  • QVGA: 154KB × 2 = 308KB         │
│  • VGA:  614KB × 2 = 1.2MB         │
│  • SVGA: 960KB × 2 = 1.9MB         │
│                                     │
│  Hardware JPEG Mode (XGA+):        │
│  • XGA:  ~150KB × 2 = ~300KB       │
│  • HD:   ~180KB × 2 = ~360KB       │
│  • SXGA: ~250KB × 2 = ~500KB       │
│  • UXGA: ~350KB × 2 = ~700KB       │
│                                     │
│  ✅ All resolutions fully supported │
├─────────────────────────────────────┤
│ Flash (16MB)                        │
│  ├─ Bootloader: ~15KB              │
│  ├─ Partition table: ~3KB          │
│  ├─ Application: ~792KB            │
│  └─ Available: ~15.2MB             │
└─────────────────────────────────────┘

Mode switching via camera reinit:
• Deinit → Change format → Reinit
• 500ms delay for sensor stabilization
```

---

## 🔬 Technical Architecture

### Software Stack (Dual-Mode Architecture)

```
┌──────────────────────────────────────────┐
│  Web Browser (Client)                    │
├──────────────────────────────────────────┤
│  HTTP/MJPEG Stream                       │
├──────────────────────────────────────────┤
│  WiFi (802.11 b/g @ 2.4GHz)             │
│  • Netgear WNR3500L v2 (SSID: INTERNET1)│
│  • 3-5 KB/s throughput                   │
│  • 120s HTTP timeout for large images   │
├──────────────────────────────────────────┤
│  ESP-IDF HTTP Server                     │
│  ├─ Port 80: UI + Capture (7 res modes) │
│  └─ Port 81: MJPEG Stream (SVGA)        │
├──────────────────────────────────────────┤
│  🔵 RGB565 Mode (≤SVGA)                  │
│  • frame2jpg() software encoder         │
│  • 100% valid JPEG output               │
│  • QVGA/VGA/SVGA resolutions            │
├──────────────────────────────────────────┤
│  🟢 Hardware JPEG Mode (XGA+)            │
│  • OV2640 hardware encoder              │
│  • Automatic header patching            │
│  • XGA/HD/SXGA/UXGA resolutions         │
│  • patchJPEGHeader() fixes FF D8 FF 10  │
├──────────────────────────────────────────┤
│  Mode Selector (shouldUseRGB565Mode)     │
│  • Automatic based on resolution        │
│  • Camera reinit on mode switch         │
│  • 500ms stabilization delay            │
├──────────────────────────────────────────┤
│  Camera Driver (esp_camera.h)            │
│  ├─ Dynamic pixel format (RGB565/JPEG) │
│  ├─ Resolution switching via reinit     │
│  ├─ Dual buffering in PSRAM             │
│  └─ GRAB_LATEST mode                    │
├──────────────────────────────────────────┤
│  OV2640 Sensor (I2C + Parallel)          │
│  • 2MP sensor (1600×1200 max)           │
│  • Dual-mode operation                  │
│  • Full resolution capability unlocked  │
└──────────────────────────────────────────┘
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

### Frame Buffer Strategy (Dual-Mode Configuration)

**Automatic pixel format selection based on resolution:**

```cpp
// RGB565 Mode (≤SVGA)
if (shouldUseRGB565Mode(framesize)) {
    config.pixel_format = PIXFORMAT_RGB565;  // Software JPEG encoding
    config.jpeg_quality = 12;                // For frame2jpg()
    config.fb_count = 2;                     // Dual buffering
}
// Hardware JPEG Mode (XGA+)
else {
    config.pixel_format = PIXFORMAT_JPEG;    // Hardware encoder
    config.jpeg_quality = 6;                 // OV2640 quality
    config.fb_count = 2;                     // Dual buffering
}

// Common settings
config.fb_location = CAMERA_FB_IN_PSRAM;     // Use 8MB PSRAM
config.grab_mode = CAMERA_GRAB_LATEST;       // Skip old frames
```

**Mode Switching Process:**
1. Detect resolution change in `/capture?res=xxx`
2. Call `esp_camera_deinit()` to release current mode
3. Wait 500ms for sensor stabilization
4. Call `initCamera(new_resolution)` with appropriate pixel format
5. Capture frame in new mode

**Benefits:**
- ✅ **100% valid JPEG output** in both modes
- ✅ Full 2MP sensor capability (UXGA 1600×1200)
- ✅ Optimized memory usage per resolution
- ✅ Smooth streaming without frame drops
- ✅ Automatic mode selection (transparent to user)

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


