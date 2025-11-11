// Put additional camera utility functions here if needed
#ifndef CAMERA_UTILS_H
#define CAMERA_UTILS_H

#include "esp_camera.h"

// Function declarations for camera utilities
bool initializeCamera();
void configureCameraSettings();
void printCameraInfo();
bool captureAndSaveImage(const char* filename);

#endif