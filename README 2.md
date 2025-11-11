# ESP32 OV2640 Camera Test

This is a PlatformIO project for testing the ESP32 with an OV2640 camera module.

## Hardware Requirements

- ESP32-CAM board (AI-Thinker or similar)
- OV2640 camera module (usually included with ESP32-CAM)
- FTDI programmer or ESP32-CAM-MB programmer board
- MicroSD card (optional, for image storage)

## Pin Configuration

The project is configured for the standard AI-Thinker ESP32-CAM pin layout:

| Camera Pin | ESP32 Pin |
|------------|-----------|
| PWDN       | GPIO 32   |
| RESET      | -1 (N/A)  |
| XCLK       | GPIO 0    |
| SIOD (SDA) | GPIO 26   |
| SIOC (SCL) | GPIO 27   |
| Y9         | GPIO 35   |
| Y8         | GPIO 34   |
| Y7         | GPIO 39   |
| Y6         | GPIO 36   |
| Y5         | GPIO 21   |
| Y4         | GPIO 19   |
| Y3         | GPIO 18   |
| Y2         | GPIO 5    |
| VSYNC      | GPIO 25   |
| HREF       | GPIO 23   |
| PCLK       | GPIO 22   |

## Features

- OV2640 camera initialization and configuration
- PSRAM detection and usage for higher quality images
- Automatic camera sensor detection
- Image quality optimization
- Serial output for debugging

## Usage

1. Connect your ESP32-CAM to your computer using an FTDI programmer
2. Make sure to connect GPIO0 to GND during programming
3. Build and upload the project using PlatformIO
4. Open the serial monitor at 115200 baud
5. Reset the ESP32-CAM (disconnect GPIO0 from GND first)
6. The camera will capture images every 5 seconds and report statistics

## Configuration

The camera is configured with:
- UXGA resolution (1600x1200) if PSRAM is available
- SVGA resolution (800x600) if no PSRAM
- JPEG format for efficient storage
- Optimized quality settings

## Next Steps

This basic example can be extended to:
- Save images to SD card
- Stream video over WiFi
- Implement motion detection
- Add web interface for remote viewing
- Integrate with IoT platforms

## Troubleshooting

- If camera initialization fails, check wiring connections
- Ensure PSRAM is properly soldered (for higher resolution)
- Verify power supply can handle camera current requirements
- Check serial output for detailed error messages