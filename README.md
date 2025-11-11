# ESP32-S3-CAM OV2640 Camera Test

PlatformIO project for testing the GOOUUU ESP32-S3-CAM module with OV2640 camera sensor.

## Hardware

- **Board**: GOOUUU ESP32-S3-CAM Module
- **MCU**: ESP32-S3-N16R8 (16MB Flash, 8MB PSRAM)
- **Camera**: OV2640 sensor
- **Resolution**: Up to 800x600 (SVGA) with PSRAM

## Pin Configuration

The project has been tested and verified with the following pin mapping:

```cpp
I2C (SCCB):
  - SDA: GPIO 4
  - SCL: GPIO 5

Camera Timing:
  - XCLK: GPIO 15
  - PCLK: GPIO 13
  - VSYNC: GPIO 6
  - HREF: GPIO 7

Data Pins:
  - D0: GPIO 11
  - D1: GPIO 9
  - D2: GPIO 8
  - D3: GPIO 10
  - D4: GPIO 12
  - D5: GPIO 18
  - D6: GPIO 17
  - D7: GPIO 16

Power Control:
  - PWDN: Not connected (-1)
  - RESET: Not connected (-1)
```

## Features

- ✅ PSRAM support (8MB)
- ✅ OV2640 camera initialization
- ✅ JPEG image capture at 800x600
- ✅ Double buffering for smooth operation
- ✅ Automatic pin configuration testing

## Building

This project uses PlatformIO. To build and upload:

```bash
platformio run --target upload
```

To monitor serial output:

```bash
platformio run --target monitor
```

Or combine both:

```bash
platformio run --target upload --target monitor
```

## Configuration

The project is configured in `platformio.ini`:

- Board: `4d_systems_esp32s3_gen4_r8n16` (ESP32-S3 with 16MB Flash + 8MB PSRAM)
- Framework: Arduino
- Upload/Monitor Speed: 115200 baud
- PSRAM: Enabled with proper build flags

## Serial Output

On successful camera initialization, you should see:

```
✅ Camera initialized successfully!
✅ Frame captured! Size: 83437 bytes, Resolution: 800x600
🎉 SUCCESS! Configuration 1 works!
```

## License

MIT License - Feel free to use and modify for your projects.
