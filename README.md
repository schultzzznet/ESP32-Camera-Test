# ESP32-S3-CAM Web Server

Live camera streaming web server for the GOOUUU ESP32-S3-CAM module with OV2640 camera sensor.

## Features

- 📹 **Live Video Streaming** - Real-time MJPEG stream at 800x600
- 📸 **Photo Capture** - Capture and download JPEG images
- 🌐 **Web Interface** - Clean, responsive web UI
- 📱 **Mobile Friendly** - Works on phones and tablets
- ⚡ **Fast Performance** - Dual-buffering with 8MB PSRAM
- 🎨 **Camera Controls** - Adjustable brightness, contrast, and more

## Hardware Requirements

### GOOUUU ESP32-S3-CAM Module

- **Model**: ESP32-S3-N16R8
- **MCU**: ESP32-S3 @ 240MHz
- **Flash**: 16MB
- **PSRAM**: 8MB (Octal SPI)
- **Camera**: OV2640 2MP sensor
- **Resolution**: Up to 1600x1200 (UXGA), configured for 800x600 (SVGA)
- **WiFi**: 2.4GHz 802.11 b/g/n
- **USB**: USB-C (CDC for serial, JTAG debugging)
- **Power**: 5V via USB-C or VIN pin

### PlatformIO Board Configuration

```ini
[env:esp32cam]
platform = espressif32
board = 4d_systems_esp32s3_gen4_r8n16
framework = arduino
monitor_speed = 115200
board_build.arduino.memory_type = qio_opi
```

**Important**: The board definition `4d_systems_esp32s3_gen4_r8n16` is critical for proper PSRAM detection on this module.

## Quick Start

### 1. Configure WiFi

Edit `src/config.h` and update your WiFi credentials:

```cpp
const char* WIFI_SSID = "YourNetworkName";
const char* WIFI_PASSWORD = "YourPassword";
```

**Note**: `config.h` is git-ignored to keep your credentials private. Use `config.h.example` as a template.

### 2. Build and Upload

```bash
platformio run --target upload --target monitor
```

### 3. Access the Camera

After successful connection, the serial monitor will display:

```
📱 Open in browser: http://192.168.1.xxx
```

Open this URL in your web browser to access:
- **Main page**: `http://192.168.1.xxx` - Web interface with live stream
- **Stream only**: `http://192.168.1.xxx:81/stream` - Direct MJPEG stream
- **Capture**: `http://192.168.1.xxx/capture` - Single JPEG snapshot

## Pin Configuration

Verified working pin mapping for GOOUUU ESP32-S3-CAM:

| Function | GPIO | Description |
|----------|------|-------------|
| SIOD (SDA) | 4 | I2C Data for camera sensor |
| SIOC (SCL) | 5 | I2C Clock for camera sensor |
| XCLK | 15 | Camera master clock (20MHz) |
| PCLK | 13 | Pixel clock input |
| VSYNC | 6 | Vertical sync signal |
| HREF | 7 | Horizontal reference signal |
| D0 | 11 | Data Bit 0 |
| D1 | 9 | Data Bit 1 |
| D2 | 8 | Data Bit 2 |
| D3 | 10 | Data Bit 3 |
| D4 | 12 | Data Bit 4 |
| D5 | 18 | Data Bit 5 |
| D6 | 17 | Data Bit 6 |
| D7 | 16 | Data Bit 7 (MSB) |
| PWDN | -1 | Power down (not used) |
| RESET | -1 | Camera reset (not used) |

**Note**: This pin configuration is specific to the GOOUUU ESP32-S3-CAM and differs from other ESP32-CAM boards.

## Project Structure

```
ESP32-Camera-Test/
├── src/
│   ├── main.cpp           # Web server and camera code
│   ├── config.h           # WiFi credentials (git-ignored)
│   └── config.h.example   # Template for WiFi config
├── platformio.ini         # PlatformIO configuration
├── .gitignore            # Git ignore rules
└── README.md             # This file
```

## Troubleshooting

### Upload Issues
- **Boot mode**: Hold BOOT button while connecting USB if upload fails
- **Port not found**: Check USB cable supports data (not charge-only)
- **Permission denied**: On macOS/Linux, add yourself to dialout group

### Camera Not Initializing
- Verify PSRAM is detected (should show ~8MB in serial output)
- Check board configuration is set to `4d_systems_esp32s3_gen4_r8n16`
- Ensure camera ribbon cable is properly seated
- Power supply must provide adequate current (500mA+ recommended)

### WiFi Connection Issues
- Double-check SSID and password in `config.h`
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check WiFi signal strength (RSSI shown in logs)
- Try disabling WiFi power saving in router settings
- Some enterprise WiFi networks may block ESP32 devices

### Stream Not Loading / Broken Images
- Clear browser cache and refresh
- Try different browser (Chrome/Edge recommended)
- Check firewall settings on computer/router
- Verify ESP32 and computer are on same network
- Monitor serial output for error messages
- WiFi disconnections shown as "BEACON_TIMEOUT" or "ASSOC_LEAVE"

## Advanced Configuration

### Change Resolution

Edit `main.cpp` in the `initCamera()` function:

```cpp
config.frame_size = FRAMESIZE_SVGA;  // Options: QQVGA, QVGA, VGA, SVGA, XGA, SXGA, UXGA
```

### Adjust Image Quality

```cpp
config.jpeg_quality = 12;  // 10-63, lower is better quality
```

### Camera Settings

The camera sensor supports various adjustments in the `initCamera()` function:
- Brightness, Contrast, Saturation
- White Balance (auto/manual modes)
- Exposure Control (AEC/AEC2)
- Auto Gain Control (AGC)
- Mirror/Flip orientation
- Special effects and color bar test pattern

## Performance

- **Frame Rate**: ~15-20 fps at 800x600 (SVGA)
- **Latency**: <200ms on local network
- **Memory Usage**: 
  - RAM: ~47KB used (14% of 320KB)
  - Flash: ~770KB used (24% of 3MB)
  - PSRAM: ~8MB available for frame buffers
- **Image Size**: ~85-95KB per JPEG frame (quality 12)
- **Power Consumption**: ~250-300mA during streaming

## Technical Details

### Memory Management
- **Dual Frame Buffers**: Located in PSRAM for smooth streaming
- **Buffer Mode**: `CAMERA_GRAB_LATEST` - always returns newest frame
- **JPEG Compression**: Hardware accelerated, quality adjustable 10-63

### Network
- **HTTP Server**: ESP-IDF native httpd (dual instances on ports 80, 81)
- **Stream Format**: MJPEG multipart/x-mixed-replace
- **Max Connections**: 7 simultaneous sockets
- **Timeouts**: 10s send/receive for reliability

### Dependencies
- **Platform**: Espressif32 6.10.0
- **Framework**: Arduino-ESP32 3.20017
- **Libraries**: ESP32-Camera (built-in), WiFi, esp_http_server

## License

MIT License - Feel free to use and modify for your projects.

## Contributing

Issues and pull requests welcome at [github.com/schultzzznet/ESP32-Camera-Test](https://github.com/schultzzznet/ESP32-Camera-Test)

## Acknowledgments

- Based on ESP32-Camera library examples
- Pin configuration verified through hardware testing
- Optimized for GOOUUU ESP32-S3-CAM hardware


