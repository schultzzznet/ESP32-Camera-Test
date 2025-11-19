#include "esp_camera.h"
#include "WiFi.h"
#include "esp_wifi.h"  // For WiFi protocol configuration
#include "esp_http_server.h"
#include "esp_timer.h"
#include "Arduino.h"
#include "config.h"
#include "esp_system.h"
#include "esp_task_wdt.h"
#include "img_converters.h"  // For frame2jpg() software JPEG encoder

// WiFi credentials from config.h
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// Camera pins for GOOUUU ESP32-S3-CAM (verified working configuration)
#define CAM_PIN_PWDN    -1
#define CAM_PIN_RESET   -1
#define CAM_PIN_XCLK    15
#define CAM_PIN_SIOD    4
#define CAM_PIN_SIOC    5

#define CAM_PIN_D7      16
#define CAM_PIN_D6      17
#define CAM_PIN_D5      18
#define CAM_PIN_D4      12
#define CAM_PIN_D3      10
#define CAM_PIN_D2      8
#define CAM_PIN_D1      9
#define CAM_PIN_D0      11
#define CAM_PIN_VSYNC   6
#define CAM_PIN_HREF    7
#define CAM_PIN_PCLK    13

httpd_handle_t camera_httpd = NULL;
httpd_handle_t stream_httpd = NULL;

// Forward declaration of camera initialization function
bool initCamera(framesize_t framesize = FRAMESIZE_SVGA);

// Helper function to determine if resolution should use RGB565 or JPEG mode
bool shouldUseRGB565Mode(framesize_t fs) {
  // RGB565 mode: Safe for resolutions ≤ SVGA (800x600)
  // JPEG mode: Required for XGA+ (high res) due to buffer size
  return (fs <= FRAMESIZE_SVGA);
}

// Patch OV2640 malformed JPEG header (FF D8 FF 10 -> FF D8 FF E0)
void patchJPEGHeader(uint8_t *buf, size_t len) {
  if (len >= 4 && buf[0] == 0xFF && buf[1] == 0xD8 && buf[2] == 0xFF && buf[3] == 0x10) {
    buf[3] = 0xE0;  // Fix: FF 10 -> FF E0 (JFIF marker)
    Serial.println("   🔧 Patched OV2640 JPEG header: FF D8 FF 10 -> FF D8 FF E0");
  }
}

// HTML page for camera controls
static const char PROGMEM INDEX_HTML[] = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>ESP32-S3 Camera</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial; text-align: center; margin: 0; background: #1a1a1a; color: #fff; }
    .container { padding: 20px; }
    h1 { margin: 20px 0; }
    img { max-width: 100%; height: auto; border: 2px solid #333; border-radius: 8px; background: #000; }
    .controls { margin: 20px; }
    button { 
      padding: 12px 24px; 
      margin: 5px; 
      font-size: 16px; 
      cursor: pointer;
      background: #007bff;
      color: white;
      border: none;
      border-radius: 4px;
    }
    button:hover { background: #0056b3; }
    button:disabled { background: #555; cursor: not-allowed; }
    .info { 
      background: #2a2a2a; 
      padding: 10px; 
      margin: 10px auto; 
      max-width: 600px;
      border-radius: 4px;
    }
    .status { color: #28a745; margin: 10px; }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎥 ESP32-S3 Camera</h1>
    <div class="info">
      <p id="camInfo">ESP32-S3 | OV2640 Camera | Auto-fix malformed headers | PSRAM: 8MB</p>
      <p class="status" id="status">Ready</p>
    </div>
    <div>
      <img id="stream" src="" alt="Camera feed will appear here">
    </div>
    <div class="controls">
      <button onclick="capturePhoto()">📸 Capture Photo</button>
      <button onclick="startStream()" id="btnStart">▶️ Start Stream</button>
      <button onclick="stopStream()" id="btnStop" disabled>⏹️ Stop Stream</button>
      <button onclick="downloadPhoto()">💾 Download</button>
    </div>
  </div>
  <script>
    let streaming = false;
    
    function capturePhoto() {
      document.getElementById('status').innerText = 'Capturing... (may take 10-15 sec)';
      const img = document.getElementById('stream');
      const url = '/capture?t=' + new Date().getTime();
      
      img.onload = function() {
        document.getElementById('status').innerText = 'Photo captured!';
        document.getElementById('status').style.color = '#28a745';
        setTimeout(() => {
          document.getElementById('status').innerText = 'Ready';
        }, 2000);
      };
      
      img.onerror = function() {
        document.getElementById('status').innerText = 'Capture failed! Try again.';
        document.getElementById('status').style.color = '#dc3545';
      };
      
      // Direct image load (simpler, more reliable)
      img.src = url;
    }
    
    function startStream() {
      const img = document.getElementById('stream');
      img.src = window.location.protocol + '//' + window.location.hostname + ':81/stream';
      streaming = true;
      document.getElementById('btnStart').disabled = true;
      document.getElementById('btnStop').disabled = false;
      document.getElementById('status').innerText = 'Streaming...';
    }
    
    function stopStream() {
      document.getElementById('stream').src = '';
      streaming = false;
      document.getElementById('btnStart').disabled = false;
      document.getElementById('btnStop').disabled = true;
      document.getElementById('status').innerText = 'Ready';
    }
    
    function downloadPhoto() {
      window.open('/capture?download=1', '_blank');
    }
  </script>
</body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req) {
  Serial.println("\n📄 INDEX PAGE REQUEST");
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  
  httpd_resp_set_type(req, "text/html");
  esp_err_t res = httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
  Serial.printf("📤 Index sent: %s (size=%d bytes)\n\n", res == ESP_OK ? "OK" : "FAILED", strlen(INDEX_HTML));
  return res;
}

static framesize_t parse_frame_size(const char *res) {
  if (!res) return FRAMESIZE_SVGA;
  // All OV2640 supported resolutions - dual mode system:
  // RGB565 mode: ≤SVGA (reliable, software JPEG)
  // JPEG mode: XGA+ (hardware JPEG with header patch)
  if (strcasecmp(res, "96x96") == 0) return FRAMESIZE_96X96;       // 96x96 [RGB565]
  if (strcasecmp(res, "qqvga") == 0) return FRAMESIZE_QQVGA;      // 160x120 [RGB565]
  if (strcasecmp(res, "qcif") == 0) return FRAMESIZE_QCIF;        // 176x144 [RGB565]
  if (strcasecmp(res, "hqvga") == 0) return FRAMESIZE_HQVGA;      // 240x176 [RGB565]
  if (strcasecmp(res, "240x240") == 0) return FRAMESIZE_240X240;  // 240x240 [RGB565]
  if (strcasecmp(res, "qvga") == 0) return FRAMESIZE_QVGA;        // 320x240 [RGB565]
  if (strcasecmp(res, "cif") == 0) return FRAMESIZE_CIF;          // 400x296 [RGB565]
  if (strcasecmp(res, "hvga") == 0) return FRAMESIZE_HVGA;        // 480x320 [RGB565]
  if (strcasecmp(res, "vga") == 0) return FRAMESIZE_VGA;          // 640x480 [RGB565]
  if (strcasecmp(res, "svga") == 0) return FRAMESIZE_SVGA;        // 800x600 [RGB565] ✅ Max RGB565
  if (strcasecmp(res, "xga") == 0) return FRAMESIZE_XGA;          // 1024x768 [JPEG] ✅ Hardware JPEG
  if (strcasecmp(res, "hd") == 0) return FRAMESIZE_HD;            // 1280x720 [JPEG] ✅ Hardware JPEG
  if (strcasecmp(res, "sxga") == 0) return FRAMESIZE_SXGA;        // 1280x1024 [JPEG] ✅ Hardware JPEG
  if (strcasecmp(res, "uxga") == 0) return FRAMESIZE_UXGA;        // 1600x1200 [JPEG] ✅ Max resolution
  return FRAMESIZE_SVGA; // Fallback to safe default
}

static esp_err_t capture_handler(httpd_req_t *req) {
  printf("[CAPTURE] Request received\n");
  Serial.println("\n========================================");
  Serial.println("📸 CAPTURE REQUEST RECEIVED");
  Serial.printf("URI: %s\n", req->uri);
  Serial.printf("Method: %d\n", req->method);
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("========================================");

  // Parse query before capture to allow dynamic quality/resolution control
  char query[128];
  bool download = false;
  int quality = 12; // Default JPEG quality for software encoder
  framesize_t desired_fs = FRAMESIZE_VGA; // default fallback
  
  printf("[CAPTURE] Parsing query string...\n");
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    Serial.printf("   Query string: %s\n", query);
    char param[16];
    if (httpd_query_key_value(query, "download", param, sizeof(param)) == ESP_OK) {
      if (strcmp(param, "1") == 0) download = true;
    }
    if (httpd_query_key_value(query, "q", param, sizeof(param)) == ESP_OK) {
      int qv = atoi(param);
      if (qv >= 10 && qv <= 63) quality = qv;
      printf("[CAPTURE] Quality set to: %d\n", quality);
    }
    if (httpd_query_key_value(query, "res", param, sizeof(param)) == ESP_OK) {
      desired_fs = parse_frame_size(param);
      printf("[CAPTURE] Resolution requested: %d\n", desired_fs);
    }
  }

  // Apply sensor changes if requested (resolution)
  // Reinitialize camera with new resolution to avoid buffer reallocation crashes
  sensor_t *s = esp_camera_sensor_get();
  if (s && desired_fs != s->status.framesize) {
    printf("[CAPTURE] Resolution change: %d -> %d - REINITIALIZING CAMERA\n", s->status.framesize, desired_fs);
    Serial.printf("   Mode: %s\n", shouldUseRGB565Mode(desired_fs) ? "RGB565 (software JPEG)" : "JPEG (hardware + patch)");
    Serial.printf("   Deinitializing camera...\n");
    
    // Deinitialize current camera
    esp_camera_deinit();
    vTaskDelay(pdMS_TO_TICKS(500));  // Increased delay for proper cleanup
    
    // Feed watchdog during reinit
    esp_task_wdt_reset();
    
    // Reinitialize with new resolution
    Serial.printf("   Reinitializing at resolution %d...\n", desired_fs);
    if (!initCamera(desired_fs)) {
      printf("[CAPTURE] ERROR: Failed to reinitialize camera\n");
      Serial.println("   ❌ Camera reinit failed - attempting recovery");
      // Try to recover with default SVGA
      if (!initCamera(FRAMESIZE_SVGA)) {
        const char *msg = "Camera reinitialization failed";
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
        return ESP_FAIL;
      }
    } else {
      Serial.println("   ✅ Camera reinitialized successfully");
      // Additional delay after successful reinit
      vTaskDelay(pdMS_TO_TICKS(200));
    }
  }

  printf("[CAPTURE] Acquiring frame buffer...\n");
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    printf("[CAPTURE] ERROR: esp_camera_fb_get() returned NULL\n");
    Serial.println("❌ Camera capture failed!");
    const char *msg = "Camera capture failed";
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
    return ESP_FAIL;
  }
  
  printf("[CAPTURE] Frame buffer OK: %u bytes, %dx%d, format=%d\n", 
         fb->len, fb->width, fb->height, fb->format);
  Serial.printf("✅ Frame buffer captured:\n");
  Serial.printf("   Size: %u bytes\n", fb->len);
  Serial.printf("   Width: %d\n", fb->width);
  Serial.printf("   Height: %d\n", fb->height);
  Serial.printf("   Format: %d (RGB565=%d, JPEG=%d)\n", fb->format, PIXFORMAT_RGB565, PIXFORMAT_JPEG);
  Serial.printf("   Timestamp: %lld\n", fb->timestamp.tv_sec);
  
  // Handle both RGB565 and JPEG modes
  uint8_t *jpg_buf = NULL;
  size_t jpg_len = 0;
  unsigned long convert_start = millis();
  bool needs_free = false;
  
  if (fb->format == PIXFORMAT_RGB565) {
    // RGB565 mode: Convert to JPEG using software encoder
    printf("[CAPTURE] Converting RGB565 to JPEG with quality=%d...\n", quality);
    Serial.println("   🔧 Converting RGB565 -> JPEG with software encoder");
    
    // Keep watchdog happy during conversion
    esp_task_wdt_reset();
    
    // Clamp quality to valid range (10-63 for ESP32)
    if (quality < 10) quality = 10;
    if (quality > 63) quality = 63;
    
    bool converted = frame2jpg(fb, quality, &jpg_buf, &jpg_len);
    unsigned long convert_time = millis() - convert_start;
    needs_free = true;
    
    if (!converted || jpg_buf == NULL || jpg_len == 0) {
      printf("[CAPTURE] ERROR: frame2jpg() failed - converted=%d, buf=%p, len=%u\n", 
             converted, jpg_buf, jpg_len);
      Serial.println("   ❌ RGB565 -> JPEG conversion failed!");
      esp_camera_fb_return(fb);
      if (jpg_buf) free(jpg_buf);
      const char *msg = "JPEG encoding failed";
      httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
      return ESP_FAIL;
    }
    
    printf("[CAPTURE] Conversion successful: %u bytes RGB565 -> %u bytes JPEG (%.1f%% compression) in %lu ms\n",
           fb->len, jpg_len, (100.0 * jpg_len / fb->len), convert_time);
    Serial.printf("   ✅ Conversion OK: %u -> %u bytes (%.1f%%) in %lu ms\n",
                  fb->len, jpg_len, (100.0 * jpg_len / fb->len), convert_time);
  } 
  else if (fb->format == PIXFORMAT_JPEG) {
    // JPEG mode: Use hardware JPEG directly, patch header if needed
    printf("[CAPTURE] Hardware JPEG mode - using direct JPEG output\n");
    Serial.println("   📸 Hardware JPEG encoder output");
    
    jpg_buf = fb->buf;
    jpg_len = fb->len;
    needs_free = false;
    
    // Patch malformed OV2640 JPEG header (FF D8 FF 10 -> FF D8 FF E0)
    patchJPEGHeader(jpg_buf, jpg_len);
    
    unsigned long convert_time = millis() - convert_start;
    printf("[CAPTURE] Hardware JPEG: %u bytes in %lu ms\n", jpg_len, convert_time);
    Serial.printf("   ✅ Hardware JPEG: %u bytes\n", jpg_len);
  }
  else {
    printf("[CAPTURE] ERROR: Unexpected format=%d\n", fb->format);
    Serial.printf("❌ Unexpected frame buffer format: %d\n", fb->format);
    esp_camera_fb_return(fb);
    const char *msg = "Unexpected camera format";
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
    return ESP_FAIL;
  }
  
  // Set headers
  printf("[CAPTURE] Setting HTTP headers...\n");
  Serial.println("\n📡 Setting HTTP headers:");
  httpd_resp_set_type(req, "image/jpeg");
  Serial.println("   Content-Type: image/jpeg");
  
  if (download) {
    httpd_resp_set_hdr(req, "Content-Disposition", "attachment; filename=capture.jpg");
    Serial.println("   Content-Disposition: attachment");
  } else {
    httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
    Serial.println("   Content-Disposition: inline");
  }
  
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "Cache-Control", "no-cache, no-store, must-revalidate");
  httpd_resp_set_hdr(req, "Pragma", "no-cache");
  httpd_resp_set_hdr(req, "Expires", "0");
  
  printf("[CAPTURE] Sending %u bytes JPEG to client...\n", jpg_len);
  Serial.printf("\n📤 Sending %u bytes to client...\n", jpg_len);
  unsigned long send_start = millis();
  
  // Send image in one shot (chunking is actually slower on ESP32)
  esp_err_t res = httpd_resp_send(req, (const char *)jpg_buf, jpg_len);
  
  unsigned long send_time = millis() - send_start;
  
  // Free resources - CRITICAL: Must free in correct order!
  // For hardware JPEG: jpg_buf points to fb->buf, so DON'T free jpg_buf separately
  // For software JPEG: jpg_buf is separately allocated, must free it first
  if (needs_free && jpg_buf) {
    free(jpg_buf);  // Free software-encoded JPEG buffer
  }
  esp_camera_fb_return(fb);  // Return frame buffer AFTER send completes
  
  printf("[CAPTURE] Complete: status=%d, send_time=%lu ms", res, send_time);
  if (send_time > 0) {
    printf(", throughput=%.2f KB/s\n", (jpg_len / 1024.0) / (send_time / 1000.0));
  } else {
    printf("\n");
  }
  
  if (res == ESP_OK) {
    Serial.println("   ✅ JPEG sent");
  } else {
    Serial.printf("   ❌ Send failed: %d\n", res);
  }
  
  Serial.printf("\n✅ Response completed:\n");
  Serial.printf("   Status: %s\n", res == ESP_OK ? "SUCCESS" : "FAILED");
  Serial.printf("   Error code: %d\n", res);
  Serial.printf("   Send time: %lu ms\n", send_time);
  if (send_time > 0) {
    Serial.printf("   Throughput: %.2f KB/s\n", (jpg_len / 1024.0) / (send_time / 1000.0));
  }
  Serial.println("========================================\n");
  return res;
}

static esp_err_t stream_handler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char part_buf[128];

  static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=frame";
  static const char* _STREAM_BOUNDARY = "\r\n--frame\r\n";
  static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

  Serial.println("🎥 Stream request received");
  
  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    Serial.printf("❌ Failed to set stream content type: %d\n", res);
    return res;
  }
  
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "X-Framerate", "60");

  int frame_count = 0;
  unsigned long start_time = millis();
  unsigned long last_report_time = start_time;
  int last_report_count = 0;
  
  while (true) {
    esp_task_wdt_reset(); // keep watchdog happy during long stream
    
    fb = esp_camera_fb_get();
    if (!fb) {
      printf("[STREAM] ERROR: esp_camera_fb_get() failed\n");
      Serial.println("❌ Stream: Camera capture failed");
      res = ESP_FAIL;
      break;
    }

    // Convert RGB565 to JPEG
    if (fb->format == PIXFORMAT_RGB565) {
      bool converted = frame2jpg(fb, 12, &_jpg_buf, &_jpg_buf_len);
      if (!converted || !_jpg_buf) {
        printf("[STREAM] ERROR: frame2jpg() failed\n");
        esp_camera_fb_return(fb);
        res = ESP_FAIL;
        break;
      }
      // Return RGB565 buffer, we now have JPEG in _jpg_buf
      esp_camera_fb_return(fb);
      fb = NULL;
    } else {
      // Fallback for JPEG mode (shouldn't happen)
      _jpg_buf_len = fb->len;
      _jpg_buf = fb->buf;
    }

    unsigned long now = millis();
    if (now - last_report_time >= 2000) { // report every ~2s
      float fps = (frame_count - last_report_count) * 1000.0f / (now - last_report_time);
      printf("[STREAM] Frame %d: %u bytes JPEG, %.1f fps, Heap: %u\n", 
             frame_count, _jpg_buf_len, fps, ESP.getFreeHeap());
      Serial.printf("📹 Frame %d: %u bytes, %.1f fps, Heap: %u\n", 
                    frame_count, _jpg_buf_len, fps, ESP.getFreeHeap());
      last_report_time = now;
      last_report_count = frame_count;
    }

    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf(part_buf, sizeof(part_buf), _STREAM_PART, _jpg_buf_len);
      res = httpd_resp_send_chunk(req, part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    
    // Clean up: free JPEG buffer (from frame2jpg) and frame buffer if still held
    if (_jpg_buf && fb == NULL) {
      // We converted RGB565->JPEG, so free the JPEG buffer
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (fb) {
      // Still holding frame buffer (fallback JPEG mode)
      esp_camera_fb_return(fb);
      fb = NULL;
    }
    
    if (res != ESP_OK) {
      printf("[STREAM] Send failed at frame %d, error: %d\n", frame_count, res);
      Serial.printf("❌ Stream send failed at frame %d, error: %d\n", frame_count, res);
      if (_jpg_buf) {
        free(_jpg_buf);
        _jpg_buf = NULL;
      }
      if (fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
      }
      break;
    }
    
    frame_count++;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️  WiFi lost during stream, stopping.");
      if (fb) {
        esp_camera_fb_return(fb);
        fb = NULL;
      }
      break;
    }
    yield();
  }
  
  Serial.println("🛑 Stream ended");
  return res;
}

void startCameraServer() {
  Serial.println("\n🌐 Starting web servers...");
  
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;
  config.ctrl_port = 32768;
  config.max_open_sockets = 7;
  config.lru_purge_enable = true;
  // Very long timeouts for slow WiFi and large high-res images
  config.recv_wait_timeout = 120;   // 2 minutes for large uploads
  config.send_wait_timeout = 120;   // 2 minutes for slow downloads
  config.max_resp_headers = 8;
  config.max_uri_handlers = 8;
  config.backlog_conn = 5;
  config.stack_size = 8192;

  httpd_uri_t index_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = index_handler,
    .user_ctx  = NULL
  };

  httpd_uri_t capture_uri = {
    .uri       = "/capture",
    .method    = HTTP_GET,
    .handler   = capture_handler,
    .user_ctx  = NULL
  };

  Serial.println("  Starting main HTTP server (port 80)...");
  if (httpd_start(&camera_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(camera_httpd, &index_uri);
    httpd_register_uri_handler(camera_httpd, &capture_uri);
    Serial.println("  ✅ Main server started");
  } else {
    Serial.println("  ❌ Failed to start main server");
  }

  config.server_port = 81;
  config.ctrl_port = 32769;
  httpd_uri_t stream_uri = {
    .uri       = "/stream",
    .method    = HTTP_GET,
    .handler   = stream_handler,
    .user_ctx  = NULL
  };

  Serial.println("  Starting stream server (port 81)...");
  if (httpd_start(&stream_httpd, &config) == ESP_OK) {
    httpd_register_uri_handler(stream_httpd, &stream_uri);
    Serial.println("  ✅ Stream server started");
  } else {
    Serial.println("  ❌ Failed to start stream server");
  }
}

bool initCamera(framesize_t framesize) {
  Serial.printf("\n📷 Initializing camera at resolution %d...\n", framesize);
  
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = CAM_PIN_D0;
  config.pin_d1 = CAM_PIN_D1;
  config.pin_d2 = CAM_PIN_D2;
  config.pin_d3 = CAM_PIN_D3;
  config.pin_d4 = CAM_PIN_D4;
  config.pin_d5 = CAM_PIN_D5;
  config.pin_d6 = CAM_PIN_D6;
  config.pin_d7 = CAM_PIN_D7;
  config.pin_xclk = CAM_PIN_XCLK;
  config.pin_pclk = CAM_PIN_PCLK;
  config.pin_vsync = CAM_PIN_VSYNC;
  config.pin_href = CAM_PIN_HREF;
  config.pin_sccb_sda = CAM_PIN_SIOD;
  config.pin_sccb_scl = CAM_PIN_SIOC;
  config.pin_pwdn = CAM_PIN_PWDN;
  config.pin_reset = CAM_PIN_RESET;
  config.xclk_freq_hz = 20000000;
  
  // Dual-mode system:
  // RGB565 (≤SVGA): Software JPEG encoding, bypasses OV2640 bugs, larger buffers
  // JPEG (XGA+): Hardware JPEG encoding, small buffers, header patch required
  if (shouldUseRGB565Mode(framesize)) {
    config.pixel_format = PIXFORMAT_RGB565;
    config.jpeg_quality = 12;  // Used by software encoder
    config.fb_count = 2;       // Dual buffering for large RGB565 frames
    Serial.printf("  Mode: RGB565 + Software JPEG\n");
    Serial.printf("  Reason: ≤SVGA, reliable software encoding\n");
  } else {
    config.pixel_format = PIXFORMAT_JPEG;
    config.jpeg_quality = 6;   // Hardware JPEG quality (lower=better, 0-63, use 6 for high quality)
    config.fb_count = 2;       // Dual buffering for stability
    Serial.printf("  Mode: Hardware JPEG + Header Patch\n");
    Serial.printf("  Reason: XGA+, high resolution needs hardware encoder\n");
  }
  
  config.frame_size = framesize;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  Serial.printf("  Resolution: %d\n", framesize);
  Serial.printf("  JPEG Quality: %d\n", config.jpeg_quality);
  Serial.printf("  Frame Buffers: %d (PSRAM)\n", config.fb_count);

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("❌ Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  Serial.println("✅ Camera initialized successfully");
  
  // Get camera sensor
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    Serial.println("⚙️  Configuring sensor settings...");
    
    // For hardware JPEG mode, set quality explicitly
    if (config.pixel_format == PIXFORMAT_JPEG) {
      s->set_quality(s, config.jpeg_quality);
      Serial.printf("   JPEG Quality: %d (hardware encoder)\n", config.jpeg_quality);
    }
    
    s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 0);       // -2 to 2
    s->set_saturation(s, 0);     // -2 to 2
    s->set_special_effect(s, 0); // 0 to 6 (0 - No Effect)
    s->set_whitebal(s, 1);       // 0 = disable , 1 = enable
    s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 0);        // 0 to 4
    s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    s->set_agc_gain(s, 0);       // 0 to 30
    s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, 0);        // 0 = disable , 1 = enable
    s->set_vflip(s, 0);          // 0 = disable , 1 = enable
    s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    s->set_colorbar(s, 0);       // 0 = disable , 1 = enable
    Serial.println("✅ Sensor configured");
  }
  
  // Test capture
  Serial.println("🧪 Testing initial capture...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    Serial.printf("✅ Test capture OK: %u bytes, %dx%d\n", fb->len, fb->width, fb->height);
    esp_camera_fb_return(fb);
  } else {
    Serial.println("⚠️  Test capture failed");
  }
  
  return true;
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  delay(3000);  // Longer delay for serial to stabilize
  
  // Use printf directly to bypass any filtering
  printf("\n\n\n================================================\n");
  printf("ESP32-S3 Camera Web Server STARTING\n");
  printf("================================================\n");
  
  printf("Step 1: Serial init OK\n");
  
  Serial.println("\n\n\n================================================");
  Serial.println("ESP32-S3 Camera Web Server");
  Serial.println("================================================");
  Serial.flush();
  
  printf("Step 2: Serial.println OK\n");
  
  // Print reset reason
  printf("Step 3: Getting reset reason...\n");
  esp_reset_reason_t reason = esp_reset_reason();
  printf("Reset reason: %d\n", reason);
  Serial.printf("Reset reason: %d ", reason);
  switch(reason) {
    case ESP_RST_POWERON: Serial.println("(Power-on)"); break;
    case ESP_RST_SW: Serial.println("(Software reset)"); break;
    case ESP_RST_PANIC: Serial.println("(Exception/panic)"); break;
    case ESP_RST_INT_WDT: Serial.println("(Interrupt watchdog)"); break;
    case ESP_RST_TASK_WDT: Serial.println("(Task watchdog)"); break;
    case ESP_RST_WDT: Serial.println("(Other watchdog)"); break;
    case ESP_RST_DEEPSLEEP: Serial.println("(Deep sleep)"); break;
    case ESP_RST_BROWNOUT: Serial.println("(Brownout)"); break;
    case ESP_RST_SDIO: Serial.println("(SDIO)"); break;
    default: Serial.println("(Unknown)"); break;
  }
  
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.printf("PSRAM: %u bytes\n\n", ESP.getPsramSize());
  
  printf("Step 4: Configuring watchdog...\n");
  // Configure watchdog (60s to handle slow WiFi uploads)
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);
  printf("Step 5: Watchdog OK\n");
  
  printf("Step 6: Checking PSRAM...\n");
  // Check PSRAM
  if (psramFound()) {
    printf("PSRAM found: %d bytes\n", ESP.getPsramSize());
    Serial.printf("✅ PSRAM: %d bytes (%.2f MB)\n", 
                  ESP.getPsramSize(), ESP.getPsramSize() / 1024.0 / 1024.0);
  } else {
    printf("ERROR: PSRAM not found!\n");
    Serial.println("❌ PSRAM not found!");
    return;
  }
  
  printf("Step 7: Getting heap info...\n");
  printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  printf("Chip ID: %llx\n", ESP.getEfuseMac());
  
  printf("Step 8: Initializing camera...\n");
  // Initialize camera
  Serial.println("Initializing camera...");
  if (!initCamera()) {
    printf("ERROR: Camera init failed!\n");
    Serial.println("❌ Camera initialization failed!");
    return;
  }
  printf("Step 9: Camera OK\n");
  Serial.println("✅ Camera initialized successfully!\n");
  
  // Test capture
  printf("Step 10: Testing camera capture...\n");
  Serial.println("Testing camera capture...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    printf("Step 11: Test capture OK - %d bytes\n", fb->len);
    Serial.printf("✅ Test capture OK: %d bytes, %dx%d\n", 
                  fb->len, fb->width, fb->height);
    esp_camera_fb_return(fb);
  } else {
    printf("ERROR: Test capture failed!\n");
    Serial.println("❌ Test capture failed!");
    return;
  }
  
  // Connect to WiFi with detailed diagnostics
  printf("Step 12: Starting WiFi setup...\n");
  Serial.println("\n================================================");
  Serial.println("📡 WiFi Configuration");
  Serial.println("================================================");
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Password: %s\n", password);
  Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
  Serial.println("================================================\n");
  
  printf("Step 13: Resetting WiFi...\n");
  // Reset WiFi and configure mode
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  delay(1000); // Longer delay for complete reset
  
  printf("Step 14: Configuring WiFi settings...\n");
  // Configure WiFi settings BEFORE mode change (critical for Google WiFi)
  WiFi.persistent(false);
  WiFi.setAutoReconnect(true);
  
  WiFi.mode(WIFI_STA);
  
  // Google WiFi AUTH_EXPIRE fix: Disable power save and use max power
  WiFi.setSleep(WIFI_PS_NONE);
  WiFi.setTxPower(WIFI_POWER_19_5dBm);
  
  // Use simple 802.11b/g for faster auth (avoid 802.11n negotiation delays)
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G);
  
  delay(500);
  
  printf("Step 15: Scanning for network...\n");
  // Scan for network to get channel (speeds up connection)
  Serial.println("Scanning for network...");
  int n = WiFi.scanNetworks();
  printf("Found %d networks\n", n);
  int8_t target_channel = 0;
  uint8_t *target_bssid = nullptr;
  bool found = false;
  
  for (int i = 0; i < n; i++) {
    if (WiFi.SSID(i) == String(ssid)) {
      target_channel = WiFi.channel(i);
      target_bssid = WiFi.BSSID(i);
      printf("Step 16: Found network on channel %d\n", target_channel);
      Serial.printf("Found '%s' on channel %d (RSSI: %d dBm)\n", 
                    ssid, target_channel, WiFi.RSSI(i));
      found = true;
      break;
    }
  }
  
  if (!found) {
    printf("WARNING: Network not found in scan\n");
    Serial.println("⚠️  Network not found in scan! Trying anyway...");
  }
  
  printf("Step 17: Connecting to WiFi...\n");
  Serial.println("Starting WiFi connection...");
  // Connect with specific channel if found (faster - skips scanning)
  if (found && target_channel > 0) {
    WiFi.begin(ssid, password, target_channel);
    Serial.printf("Connecting on channel %d...\n", target_channel);
  } else {
    WiFi.begin(ssid, password);
  }
  
  printf("Step 18: Waiting for WiFi connection...\n");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    wl_status_t status = WiFi.status();
    
    // Print detailed status every 5 attempts
    if (attempts % 5 == 0) {
      printf("[Attempt %d/40] WiFi status: %d\n", attempts, status);
      Serial.printf("\n[Attempt %d/40] Status: ", attempts);
      switch(status) {
        case WL_IDLE_STATUS:     Serial.print("IDLE"); break;
        case WL_NO_SSID_AVAIL:   Serial.print("NO_SSID - Network not found!"); break;
        case WL_SCAN_COMPLETED:  Serial.print("SCAN_COMPLETED"); break;
        case WL_CONNECTED:       Serial.print("CONNECTED"); break;
        case WL_CONNECT_FAILED:  Serial.print("CONNECT_FAILED - Wrong password?"); break;
        case WL_CONNECTION_LOST: Serial.print("CONNECTION_LOST"); break;
        case WL_DISCONNECTED:    Serial.print("DISCONNECTED"); break;
        default:                 Serial.printf("UNKNOWN (%d)", status); break;
      }
      Serial.print(" ");
    } else {
      Serial.print(".");
    }
    attempts++;
  }
  
  printf("Step 19: WiFi connection attempt complete\n");
  if (WiFi.status() == WL_CONNECTED) {
    printf("Step 20: WiFi connected! IP: %s\n", WiFi.localIP().toString().c_str());
    Serial.println("\n\n✅ WiFi connected!");
    Serial.printf("IP Address: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Stream URL: http://%s:81/stream\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    Serial.printf("Free heap after WiFi: %u bytes\n\n", ESP.getFreeHeap());
    
    printf("Step 21: Starting web servers...\n");
    // Start web server
    startCameraServer();
    
    printf("Step 22: Servers started!\n");
    Serial.println("\n================================================");
    Serial.println("🎥 Camera Server Ready!");
    Serial.println("================================================");
    Serial.printf("📱 Open in browser: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Free heap after servers: %u bytes\n", ESP.getFreeHeap());
    Serial.println("================================================\n");
    printf("Step 23: Setup complete - entering loop\n");
  } else {
    printf("ERROR: WiFi connection failed - status: %d\n", WiFi.status());
    Serial.println("\n\n❌ WiFi connection failed!");
    Serial.print("Final status: ");
    wl_status_t status = WiFi.status();
    switch(status) {
      case WL_NO_SSID_AVAIL:   Serial.println("NO_SSID - Network not found! Check SSID name."); break;
      case WL_CONNECT_FAILED:  Serial.println("CONNECT_FAILED - Wrong password or network security issue!"); break;
      case WL_DISCONNECTED:    Serial.println("DISCONNECTED - Cannot associate with network!"); break;
      default:                 Serial.printf("UNKNOWN (%d)\n", status); break;
    }
    Serial.println("\nTroubleshooting:");
    Serial.println("1. Verify SSID and password in config.h");
    Serial.println("2. Ensure WiFi is 2.4GHz (ESP32 doesn't support 5GHz)");
    Serial.println("3. Check if router has MAC filtering enabled");
    Serial.println("4. Try moving ESP32 closer to router");
    Serial.println("5. Check power supply - use external 5V/2A if needed");
  }
}

void loop() {
  static unsigned long loop_count = 0;
  loop_count++;
  
  if (loop_count % 20 == 1) {  // Every ~100 seconds
    printf("[LOOP %lu] Running at %lu sec\n", loop_count, millis()/1000);
  }
  
  delay(5000);
  esp_task_wdt_reset();
  
  static unsigned long last_status = 0;
  static bool was_connected = false;
  static int reconnect_attempts = 0;
  static unsigned long last_reconnect = 0;
  static int backoff_delay = 5000; // Start with 5 seconds
  
  bool is_connected = (WiFi.status() == WL_CONNECTED);
  
  // Print periodic status
  if (millis() - last_status > 60000) {
    printf("[HEARTBEAT] Uptime: %lu sec, Heap: %u, WiFi: %s\n",
           millis()/1000, ESP.getFreeHeap(), is_connected ? "OK" : "DOWN");
    Serial.printf("💓 Alive - Free heap: %u, WiFi: %s, RSSI: %d dBm\n", 
                  ESP.getFreeHeap(), 
                  is_connected ? "Connected" : "Disconnected",
                  WiFi.RSSI());
    last_status = millis();
  }
  
  // Only attempt reconnection if we were previously connected
  if (is_connected) {
    was_connected = true;
    reconnect_attempts = 0;  // Reset counter on successful connection
    backoff_delay = 5000;     // Reset backoff
    
    if (loop_count % 20 == 0) {  // Every ~100 seconds
      printf("[STATUS] IP: %s | Heap: %d | PSRAM: %d\n",
             WiFi.localIP().toString().c_str(),
             ESP.getFreeHeap(),
             ESP.getFreePsram());
    }
    
    Serial.printf("[%lu sec] Server running | IP: %s | Free Heap: %d | PSRAM: %d\n",
                  millis()/1000,
                  WiFi.localIP().toString().c_str(),
                  ESP.getFreeHeap(),
                  ESP.getFreePsram());
  } else if (was_connected) {
    // WiFi disconnected - use exponential backoff to avoid Google WiFi rate limiting
    unsigned long now = millis();
    if (now - last_reconnect > backoff_delay) {
      reconnect_attempts++;
      last_reconnect = now;
      
      printf("[RECONNECT] Attempt %d (backoff: %ds)\n", reconnect_attempts, backoff_delay/1000);
      Serial.printf("⚠️  WiFi disconnected! Reconnect attempt %d (backoff: %ds)\n", 
                    reconnect_attempts, backoff_delay/1000);
      
      // Google WiFi has aggressive rate limiting - after 3-4 reconnects, do full reboot
      if (reconnect_attempts >= 4) {
        printf("[REBOOT] Too many reconnect failures - restarting ESP32\n");
        Serial.println("🔄 Too many reconnect failures - rebooting to reset WiFi state...");
        Serial.println("(Google WiFi Pods have aggressive rate limiting)");
        delay(2000);
        ESP.restart();
      }
      
      WiFi.reconnect();
      
      // Exponential backoff: 5s → 10s → 20s → 40s
      backoff_delay = min(backoff_delay * 2, 60000); // Cap at 60 seconds
    }
  }
  // If never connected, don't spam reconnect attempts
}
