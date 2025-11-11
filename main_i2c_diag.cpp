#include "Arduino.h"
#include "Wire.h"

// Test I2C connectivity for camera detection
void scanI2C(uint8_t sda, uint8_t scl) {
  Serial.printf("Scanning I2C on SDA:%d, SCL:%d\n", sda, scl);
  
  Wire.begin(sda, scl);
  delay(100);
  
  int deviceCount = 0;
  for(int address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();
    
    if (error == 0) {
      Serial.printf("I2C device found at address 0x%02X\n", address);
      deviceCount++;
      
      // Check if it might be OV2640 (common addresses: 0x30, 0x60)
      if(address == 0x30 || address == 0x60) {
        Serial.println("  -> This might be an OV2640 camera sensor!");
      }
    }
  }
  
  if(deviceCount == 0) {
    Serial.println("No I2C devices found");
  } else {
    Serial.printf("Found %d I2C device(s)\n", deviceCount);
  }
  
  Wire.end();
  Serial.println();
}

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("=================================");
  Serial.println("ESP32-S3 Camera Pin Diagnostic");
  Serial.println("=================================");
  Serial.println();
  
  Serial.printf("ESP32-S3 Chip ID: %llx\n", ESP.getEfuseMac());
  Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
  
  // Check for PSRAM
  if(psramFound()){
    Serial.printf("PSRAM found! Size: %d bytes\n", ESP.getPsramSize());
  } else {
    Serial.println("PSRAM not found");
    Serial.println("Note: Camera modules typically require PSRAM for frame buffers");
  }
  
  Serial.println();
  Serial.println("Testing potential I2C configurations for camera detection...");
  Serial.println();
  
  // Test different I2C pin combinations that might be used on GOOUUU ESP32-S3-CAM
  // Using valid ESP32-S3 GPIO pins for I2C
  Serial.println("=== Testing Configuration 1: SDA=4, SCL=5 ===");
  scanI2C(4, 5);
  
  Serial.println("=== Testing Configuration 2: SDA=8, SCL=9 ===");
  scanI2C(8, 9);
  
  Serial.println("=== Testing Configuration 3: SDA=14, SCL=15 ===");
  scanI2C(14, 15);
  
  Serial.println("=== Testing Configuration 4: SDA=40, SCL=41 ===");
  scanI2C(40, 41);
  
  Serial.println("=== Testing Configuration 5: SDA=42, SCL=2 ===");
  scanI2C(42, 2);
  
  Serial.println("==========================================");
  Serial.println("I2C scan complete!");
  Serial.println("==========================================");
  Serial.println();
  
  if(psramFound()) {
    Serial.println("Next steps:");
    Serial.println("1. If camera sensor was detected, we can proceed with camera initialization");
    Serial.println("2. PSRAM is available for frame buffers");
  } else {
    Serial.println("Issues detected:");
    Serial.println("1. No PSRAM found - this will limit camera functionality");
    Serial.println("2. Consider using a board with PSRAM for camera applications");
    Serial.println("3. We may need to configure PSRAM or use a different approach");
  }
  
  Serial.println();
  Serial.println("Diagnostic complete. The board will continue running safely.");
  Serial.println("Please report which I2C configuration detected the camera sensor.");
}

void loop() {
  delay(5000);
  Serial.printf("System running. Free heap: %d bytes, Uptime: %lu seconds\n", ESP.getFreeHeap(), millis()/1000);
}