#include "esp_camera.h"
#include "Arduino.h"

// Multiple pin configurations for GOOUUU ESP32-S3-CAM module
// We'll test these systematically since I2C detection didn't work

// Configuration 1: Freenove ESP32-S3 CAM style (common reference)
camera_config_t getCameraConfig1() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11;
  config.pin_d1 = 9;
  config.pin_d2 = 8;
  config.pin_d3 = 10;
  config.pin_d4 = 12;
  config.pin_d5 = 18;
  config.pin_d6 = 17;
  config.pin_d7 = 16;
  config.pin_xclk = 15;
  config.pin_pclk = 13;
  config.pin_vsync = 6;
  config.pin_href = 7;
  config.pin_sccb_sda = 4;   // I2C SDA
  config.pin_sccb_scl = 5;   // I2C SCL
  config.pin_pwdn = -1;      // Not connected
  config.pin_reset = -1;     // Not connected
  
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;  // 800x600 with PSRAM
  config.jpeg_quality = 10;
  config.fb_count = 2;      // Double buffer with PSRAM
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  return config;
}

// Configuration 2: Alternative ESP32-S3 mapping
camera_config_t getCameraConfig2() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 11;
  config.pin_d1 = 9;
  config.pin_d2 = 8;
  config.pin_d3 = 10;
  config.pin_d4 = 12;
  config.pin_d5 = 13;
  config.pin_d6 = 21;
  config.pin_d7 = 48;
  config.pin_xclk = 3;
  config.pin_pclk = 14;
  config.pin_vsync = 38;
  config.pin_href = 47;
  config.pin_sccb_sda = 4;   // I2C SDA for camera config
  config.pin_sccb_scl = 5;   // I2C SCL for camera config
  config.pin_pwdn = 1;       // Power down
  config.pin_reset = 2;      // Reset
  
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;  // 800x600 with PSRAM
  config.jpeg_quality = 10;
  config.fb_count = 2;      // Double buffer with PSRAM
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  return config;
}

// Configuration 3: GOOUUU specific mapping (based on typical layouts)
camera_config_t getCameraConfig3() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = 5;
  config.pin_d1 = 14;
  config.pin_d2 = 4;
  config.pin_d3 = 15;
  config.pin_d4 = 18;
  config.pin_d5 = 16;
  config.pin_d6 = 17;
  config.pin_d7 = 12;
  config.pin_xclk = 10;
  config.pin_pclk = 13;
  config.pin_vsync = 38;
  config.pin_href = 47;
  config.pin_sccb_sda = 40;  // I2C SDA for camera config
  config.pin_sccb_scl = 39;  // I2C SCL for camera config
  config.pin_pwdn = 1;       // Power down
  config.pin_reset = 2;      // Reset
  
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_SVGA;  // 800x600 with PSRAM
  config.jpeg_quality = 10;
  config.fb_count = 2;      // Double buffer with PSRAM
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  return config;
}

bool testCameraConfig(const char* configName, camera_config_t config) {
  Serial.printf("\n=== Testing %s ===\n", configName);
  Serial.printf("SCCB: SDA=%d, SCL=%d\n", config.pin_sccb_sda, config.pin_sccb_scl);
  Serial.printf("XCLK=%d, PCLK=%d, VSYNC=%d, HREF=%d\n", 
                config.pin_xclk, config.pin_pclk, config.pin_vsync, config.pin_href);
  Serial.printf("Data pins: D0=%d, D1=%d, D2=%d, D3=%d, D4=%d, D5=%d, D6=%d, D7=%d\n",
                config.pin_d0, config.pin_d1, config.pin_d2, config.pin_d3,
                config.pin_d4, config.pin_d5, config.pin_d6, config.pin_d7);
  Serial.printf("PWDN=%d, RESET=%d\n", config.pin_pwdn, config.pin_reset);

  esp_err_t err = esp_camera_init(&config);
  
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed with error 0x%x\n", err);
    
    // Translate common error codes
    switch(err) {
      case ESP_ERR_INVALID_ARG:
        Serial.println("   -> Invalid argument (check pin assignments)");
        break;
      case ESP_ERR_NOT_FOUND:
        Serial.println("   -> Camera not found (check wiring)");
        break;
      case ESP_ERR_NO_MEM:
        Serial.println("   -> Out of memory (PSRAM issue)");
        break;
      default:
        Serial.printf("   -> Unknown error: %s\n", esp_err_to_name(err));
        break;
    }
    return false;
  }
  
  Serial.println("✅ Camera initialized successfully!");
  
  // Test capturing a frame
  Serial.println("Attempting to capture a test frame...");
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Failed to capture frame");
    esp_camera_deinit();
    return false;
  }
  
  Serial.printf("✅ Frame captured! Size: %d bytes, Resolution: %dx%d\n", 
                fb->len, fb->width, fb->height);
  Serial.printf("   Format: %s\n", 
                (fb->format == PIXFORMAT_JPEG) ? "JPEG" : "RAW");
  
  esp_camera_fb_return(fb);
  esp_camera_deinit();
  
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("================================================");
  Serial.println("ESP32-S3 Camera Configuration Test");
  Serial.println("================================================");
  Serial.println();
  
  Serial.printf("ESP32-S3 Chip ID: %llx\n", ESP.getEfuseMac());
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  
  // Check for PSRAM
  if(psramFound()) {
    Serial.printf("✅ PSRAM found! Size: %d bytes\n", ESP.getPsramSize());
  } else {
    Serial.println("⚠️  PSRAM not found");
    Serial.println("   Using optimized settings for limited memory");
  }
  
  Serial.println();
  Serial.println("Testing different camera pin configurations...");
  
  // Test each configuration
  camera_config_t config1 = getCameraConfig1();
  if (testCameraConfig("Configuration 1 (Standard ESP32-S3-CAM)", config1)) {
    Serial.println("\n🎉 SUCCESS! Configuration 1 works!");
    Serial.println("You can now use this pin mapping for your camera project.");
    return;
  }
  
  delay(1000);
  
  camera_config_t config2 = getCameraConfig2();
  if (testCameraConfig("Configuration 2 (Alternative ESP32-S3)", config2)) {
    Serial.println("\n🎉 SUCCESS! Configuration 2 works!");
    Serial.println("You can now use this pin mapping for your camera project.");
    return;
  }
  
  delay(1000);
  
  camera_config_t config3 = getCameraConfig3();
  if (testCameraConfig("Configuration 3 (GOOUUU Specific)", config3)) {
    Serial.println("\n🎉 SUCCESS! Configuration 3 works!");
    Serial.println("You can now use this pin mapping for your camera project.");
    return;
  }
  
  Serial.println("\n❌ None of the configurations worked");
  Serial.println("\nTroubleshooting suggestions:");
  Serial.println("1. Check camera module connections");
  Serial.println("2. Verify power supply (camera needs adequate power)");
  Serial.println("3. Look for GOOUUU ESP32-S3-CAM schematic/pinout");
  Serial.println("4. Try different camera modules (OV5640, etc.)");
  Serial.println("5. Check if camera module requires external pull-ups");
  
  if (!psramFound()) {
    Serial.println("\n⚠️  PSRAM Missing Issues:");
    Serial.println("- Your board lacks PSRAM which limits camera functionality");
    Serial.println("- Consider getting an ESP32-S3 board with PSRAM");
    Serial.println("- Or use very small frame sizes (QQVGA)");
  }
}

void loop() {
  delay(10000);
  Serial.printf("System running. Free heap: %d bytes\n", ESP.getFreeHeap());
}