#include "esp_camera.h"
#include "WiFi.h"
#include "esp_http_server.h"
#include "esp_timer.h"
#include "Arduino.h"
#include "config.h"
#include "esp_system.h"
#include "esp_task_wdt.h"

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
      <p id="camInfo">Camera: OV2640 | Resolution: 640x480 (VGA) | PSRAM: 8MB</p>
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
  if (!res) return FRAMESIZE_VGA;
  if (strcasecmp(res, "qqvga") == 0) return FRAMESIZE_QQVGA;
  if (strcasecmp(res, "qvga") == 0) return FRAMESIZE_QVGA;
  if (strcasecmp(res, "cif") == 0) return FRAMESIZE_CIF;
  if (strcasecmp(res, "vga") == 0) return FRAMESIZE_VGA;
  if (strcasecmp(res, "svga") == 0) return FRAMESIZE_SVGA;
  if (strcasecmp(res, "xga") == 0) return FRAMESIZE_XGA;
  if (strcasecmp(res, "sxga") == 0) return FRAMESIZE_SXGA;
  if (strcasecmp(res, "uxga") == 0) return FRAMESIZE_UXGA;
  return FRAMESIZE_VGA; // Fallback
}

static esp_err_t capture_handler(httpd_req_t *req) {
  Serial.println("\n========================================");
  Serial.println("📸 CAPTURE REQUEST RECEIVED");
  Serial.printf("URI: %s\n", req->uri);
  Serial.printf("Method: %d\n", req->method);
  Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
  Serial.println("========================================");

  // Parse query before capture to allow dynamic quality/resolution control
  char query[128];
  bool download = false;
  int quality = -1; // -1 means unchanged
  framesize_t desired_fs = FRAMESIZE_VGA; // default fallback
  if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
    Serial.printf("   Query string: %s\n", query);
    char param[16];
    if (httpd_query_key_value(query, "download", param, sizeof(param)) == ESP_OK) {
      if (strcmp(param, "1") == 0) download = true;
    }
    if (httpd_query_key_value(query, "q", param, sizeof(param)) == ESP_OK) {
      int qv = atoi(param);
      if (qv >= 10 && qv <= 63) quality = qv; // JPEG quality range for esp32-camera
    }
    if (httpd_query_key_value(query, "res", param, sizeof(param)) == ESP_OK) {
      desired_fs = parse_frame_size(param);
    }
  }

  // Apply sensor changes if requested (quality or resolution)
  sensor_t *s = esp_camera_sensor_get();
  if (s) {
    if (quality != -1 && quality != s->status.quality) {
      Serial.printf("   🔧 Updating JPEG quality: %d -> %d\n", s->status.quality, quality);
      s->set_quality(s, quality);
    }
    if (desired_fs != s->status.framesize) {
      Serial.printf("   🔧 Updating frame size: %d -> %d\n", s->status.framesize, desired_fs);
      s->set_framesize(s, desired_fs);
      // Small delay to let sensor settle
      delay(50);
    }
  }

  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Camera capture failed!");
    const char *msg = "Camera capture failed";
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
    return ESP_FAIL;
  }
  
  Serial.printf("✅ Frame buffer captured:\n");
  Serial.printf("   Size: %u bytes\n", fb->len);
  Serial.printf("   Width: %d\n", fb->width);
  Serial.printf("   Height: %d\n", fb->height);
  Serial.printf("   Format: %d (JPEG=%d)\n", fb->format, PIXFORMAT_JPEG);
  Serial.printf("   Timestamp: %lld\n", fb->timestamp.tv_sec);
  
  // Verify it's JPEG format
  if (fb->format != PIXFORMAT_JPEG) {
    Serial.println("❌ Frame buffer is not JPEG format!");
    esp_camera_fb_return(fb);
    const char *msg = "Camera not in JPEG mode";
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
    return ESP_FAIL;
  }
  
  // Check JPEG signature and structure
  Serial.println("🔍 Analyzing JPEG data:");
  bool needs_header_fix = false;
  
  if (fb->len < 4) {
    Serial.println("   ❌ Buffer too small!");
  } else {
    Serial.printf("   First 16 bytes: ");
    for (int i = 0; i < 16 && i < fb->len; i++) {
      Serial.printf("%02X ", fb->buf[i]);
    }
    Serial.println();
    
    // Check JPEG SOI (Start of Image) marker
    if (fb->buf[0] == 0xFF && fb->buf[1] == 0xD8) {
      Serial.println("   ✅ Valid JPEG SOI marker (FF D8)");
      
      // OV2640 bug: generates FF D8 FF 10 instead of FF D8 FF E0
      if (fb->buf[2] == 0xFF && fb->buf[3] == 0x10) {
        Serial.println("   ⚠️  Detected invalid FF 10 marker (OV2640 bug)");
        needs_header_fix = true;
      }
    } else {
      Serial.printf("   ❌ Invalid JPEG header: %02X %02X (expected FF D8)\n", fb->buf[0], fb->buf[1]);
    }
    
    // Check JPEG EOI (End of Image) marker
    if (fb->len >= 2 && fb->buf[fb->len-2] == 0xFF && fb->buf[fb->len-1] == 0xD9) {
      Serial.println("   ✅ Valid JPEG EOI marker (FF D9)");
    } else {
      Serial.printf("   ⚠️  Last 2 bytes: %02X %02X (expected FF D9)\n", 
                    fb->buf[fb->len-2], fb->buf[fb->len-1]);
    }
  }
  
  // (Download flag already parsed earlier)
  
  // Set headers before sending
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
  
  Serial.printf("\n📤 Sending %u bytes to client...\n", fb->len);
  unsigned long send_start = millis();
  
  esp_err_t res;
  
  // Construct proper JFIF APP0 header if OV2640 malformed sequence detected
  // Expected fix pattern: FF D8 FF 10 FF DB ... -> FF D8 FF E0 00 10 'JFIF' 00 01 01 01 00 60 00 60 00 00 FF DB ...
  const uint8_t jfif_block[] = {
    0xFF, 0xE0, 0x00, 0x10, // APP0 marker + length (16 bytes including identifier)
    'J', 'F', 'I', 'F', 0x00,
    0x01, 0x01, // version 1.01
    0x01,       // density units: dots per inch
    0x00, 0x60, // X density 96
    0x00, 0x60, // Y density 96
    0x00,       // thumbnail width
    0x00        // thumbnail height
  };

  bool needs_jfif_insertion = false;
  if (fb->len > 5 && fb->buf[0] == 0xFF && fb->buf[1] == 0xD8) {
    // Case 1: malformed marker FF 10
    if (fb->buf[2] == 0xFF && fb->buf[3] == 0x10) {
      needs_jfif_insertion = true;
      Serial.println("   ⚠️  Detected OV2640 malformed header (FF 10) -> will rebuild with JFIF APP0");
    }
    // Case 2: previously patched to FF E0 but missing length (next byte 0xFF instead of length high byte)
    else if (fb->buf[2] == 0xFF && fb->buf[3] == 0xE0 && fb->buf[4] == 0xFF) {
      needs_jfif_insertion = true;
      Serial.println("   ⚠️  Detected missing APP0 length after FF E0 -> will rebuild with full JFIF block");
    }
  }

  if (needs_jfif_insertion) {
    // New length = original length - 4 bytes (FF D8 FF 10 or FF D8 FF E0) + size of inserted JFIF block
    size_t new_len = fb->len - 4 + sizeof(jfif_block);
    uint8_t *out_buf = (uint8_t*)ps_malloc(new_len);
    if (!out_buf) {
      Serial.println("   ❌ Failed to allocate buffer for JFIF fix, sending original");
      res = httpd_resp_send(req, (const char*)fb->buf, fb->len);
    } else {
      uint8_t *dst = out_buf;
      // Copy SOI
      dst[0] = fb->buf[0]; dst[1] = fb->buf[1];
      dst += 2;
      // Insert JFIF APP0 block
      memcpy(dst, jfif_block, sizeof(jfif_block));
      dst += sizeof(jfif_block);
      // Copy remainder skipping malformed 2-byte marker (FF 10 or FF E0 missing length)
  memcpy(dst, fb->buf + 4, fb->len - 4); // copy remainder after malformed 2-byte marker
      Serial.printf("   🔧 Rebuilt JPEG with JFIF APP0 (added %u bytes)\n", (unsigned)sizeof(jfif_block));
      res = httpd_resp_send(req, (const char*)out_buf, new_len);
      if (res == ESP_OK) Serial.println("   ✅ Sent repaired JPEG");
      free(out_buf);
    }
  } else {
    Serial.println("   ℹ️  Header already valid JFIF or acceptable, sending as-is");
    res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
    if (res == ESP_OK) Serial.println("   ✅ JPEG sent");
  }
  
  unsigned long send_time = millis() - send_start;
  
  // Save size before returning buffer
  size_t image_size = fb->len;
  
  // Return buffer AFTER sending
  esp_camera_fb_return(fb);
  
  Serial.printf("\n✅ Response completed:\n");
  Serial.printf("   Status: %s\n", res == ESP_OK ? "SUCCESS" : "FAILED");
  Serial.printf("   Error code: %d\n", res);
  Serial.printf("   Send time: %lu ms\n", send_time);
  if (send_time > 0) {
    Serial.printf("   Throughput: %.2f KB/s\n", (image_size / 1024.0) / (send_time / 1000.0));
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
  bool rebuilt_frame = false; // track if we allocated a rebuilt JPEG

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
      Serial.println("❌ Stream: Camera capture failed");
      res = ESP_FAIL;
      break;
    }

    _jpg_buf_len = fb->len;
    _jpg_buf = fb->buf;

    // Repair header for streaming frames if malformed (same logic as capture)
    if (_jpg_buf_len > 5 && _jpg_buf[0] == 0xFF && _jpg_buf[1] == 0xD8) {
      bool fix_stream = false;
      if (_jpg_buf[2] == 0xFF && _jpg_buf[3] == 0x10) fix_stream = true; // malformed OV2640 header
      else if (_jpg_buf[2] == 0xFF && _jpg_buf[3] == 0xE0 && _jpg_buf[4] == 0xFF) fix_stream = true; // missing length
      if (fix_stream) {
        const uint8_t jfif_block_stream[] = {
          0xFF, 0xE0, 0x00, 0x10,
          'J','F','I','F',0x00,
          0x01,0x01,0x01,0x00,0x60,0x00,0x60,0x00,0x00
        };
  size_t new_len = _jpg_buf_len - 4 + sizeof(jfif_block_stream);
        uint8_t *rebuilt = (uint8_t*)ps_malloc(new_len);
        if (rebuilt) {
          uint8_t *dst = rebuilt;
          dst[0] = _jpg_buf[0]; dst[1] = _jpg_buf[1];
          dst += 2;
          memcpy(dst, jfif_block_stream, sizeof(jfif_block_stream));
          dst += sizeof(jfif_block_stream);
          memcpy(dst, _jpg_buf + 4, _jpg_buf_len - 4);
          // Use rebuilt buffer for sending this frame only
          _jpg_buf = rebuilt;
          _jpg_buf_len = new_len;
          rebuilt_frame = true;
        }
      }
    }

    unsigned long now = millis();
    if (now - last_report_time >= 2000) { // report every ~2s
      float fps = (frame_count - last_report_count) * 1000.0f / (now - last_report_time);
      Serial.printf("📹 Frame %d: %u bytes, %dx%d, %.1f fps, Heap: %u\n", 
                    frame_count, _jpg_buf_len, fb->width, fb->height, fps, ESP.getFreeHeap());
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
    
    // Return original frame buffer (if we rebuilt, _jpg_buf points to temp allocated memory)
    esp_camera_fb_return(fb);
    fb = NULL;
    if (rebuilt_frame && _jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    
    if (res != ESP_OK) {
      Serial.printf("❌ Stream send failed at frame %d, error: %d\n", frame_count, res);
      break;
    }
    
    frame_count++;
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("⚠️  WiFi lost during stream, stopping.");
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
  // Increased timeouts to better tolerate very slow WiFi links
  config.recv_wait_timeout = 20;   // was 10
  config.send_wait_timeout = 20;   // was 10
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

bool initCamera() {
  Serial.println("\n📷 Initializing camera...");
  
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
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  // 640x480 (smaller for faster WiFi transmission)
  config.jpeg_quality = 30;  // Higher number = lower quality = smaller file (optimized for slow WiFi)
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  Serial.printf("  Resolution: 640x480 (VGA)\n");
  Serial.printf("  JPEG Quality: 30 (optimized for slow WiFi)\n");
  Serial.printf("  Frame Buffers: 2 (PSRAM)\n");

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
  delay(2000);
  
  Serial.println("\n================================================");
  Serial.println("ESP32-S3 Camera Web Server");
  Serial.println("================================================");
  
  // Print reset reason
  esp_reset_reason_t reason = esp_reset_reason();
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
  
  // Configure watchdog
  esp_task_wdt_init(30, true);
  esp_task_wdt_add(NULL);
  
  // Check PSRAM
  if (psramFound()) {
    Serial.printf("✅ PSRAM: %d bytes (%.2f MB)\n", 
                  ESP.getPsramSize(), ESP.getPsramSize() / 1024.0 / 1024.0);
  } else {
    Serial.println("❌ PSRAM not found!");
    return;
  }
  
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Chip ID: %llx\n\n", ESP.getEfuseMac());
  
  // Initialize camera
  Serial.println("Initializing camera...");
  if (!initCamera()) {
    Serial.println("❌ Camera initialization failed!");
    return;
  }
  Serial.println("✅ Camera initialized successfully!\n");
  
  // Test capture
  Serial.println("Testing camera capture...");
  camera_fb_t *fb = esp_camera_fb_get();
  if (fb) {
    Serial.printf("✅ Test capture OK: %d bytes, %dx%d\n", 
                  fb->len, fb->width, fb->height);
    esp_camera_fb_return(fb);
  } else {
    Serial.println("❌ Test capture failed!");
    return;
  }
  
  // Connect to WiFi with detailed diagnostics
  Serial.println("\n================================================");
  Serial.println("📡 WiFi Configuration");
  Serial.println("================================================");
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Password: %s\n", password);
  Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
  Serial.println("================================================\n");
  
  // Reset WiFi and configure mode
  WiFi.disconnect(true);
  delay(100);
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false); // Disable WiFi sleep for stability
  WiFi.setTxPower(WIFI_POWER_19_5dBm); // Max power for better signal
  delay(100);
  
  Serial.println("Starting WiFi connection...");
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 40) {
    delay(500);
    wl_status_t status = WiFi.status();
    
    // Print detailed status every 5 attempts
    if (attempts % 5 == 0) {
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
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n\n✅ WiFi connected!");
    Serial.printf("IP Address: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Stream URL: http://%s:81/stream\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal Strength: %d dBm\n", WiFi.RSSI());
    Serial.printf("Free heap after WiFi: %u bytes\n\n", ESP.getFreeHeap());
    
    // Start web server
    startCameraServer();
    
    Serial.println("\n================================================");
    Serial.println("🎥 Camera Server Ready!");
    Serial.println("================================================");
    Serial.printf("📱 Open in browser: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Free heap after servers: %u bytes\n", ESP.getFreeHeap());
    Serial.println("================================================\n");
  } else {
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
  delay(5000);
  esp_task_wdt_reset();
  
  static unsigned long last_status = 0;
  static bool was_connected = false;
  
  bool is_connected = (WiFi.status() == WL_CONNECTED);
  
  // Print periodic status
  if (millis() - last_status > 60000) {
    Serial.printf("💓 Alive - Free heap: %u, WiFi: %s, RSSI: %d dBm\n", 
                  ESP.getFreeHeap(), 
                  is_connected ? "Connected" : "Disconnected",
                  WiFi.RSSI());
    last_status = millis();
  }
  
  // Only attempt reconnection if we were previously connected
  if (is_connected) {
    was_connected = true;
    Serial.printf("[%lu sec] Server running | IP: %s | Free Heap: %d | PSRAM: %d\n",
                  millis()/1000,
                  WiFi.localIP().toString().c_str(),
                  ESP.getFreeHeap(),
                  ESP.getFreePsram());
  } else if (was_connected) {
    // Only reconnect if we had a connection before
    Serial.println("⚠️  WiFi connection lost! Attempting to reconnect...");
    WiFi.reconnect();
  }
  // If never connected, don't spam reconnect attempts
}
