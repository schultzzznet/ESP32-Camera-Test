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
    img { max-width: 100%; height: auto; border: 2px solid #333; border-radius: 8px; }
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
    .info { 
      background: #2a2a2a; 
      padding: 10px; 
      margin: 10px auto; 
      max-width: 600px;
      border-radius: 4px;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🎥 ESP32-S3 Camera</h1>
    <div class="info">
      <p>Camera: OV2640 | Resolution: 800x600 | PSRAM: 8MB</p>
    </div>
    <div>
      <img id="stream" src="">
    </div>
    <div class="controls">
      <button onclick="location.href='/capture'">📸 Capture Photo</button>
      <button onclick="startStream()">▶️ Start Stream</button>
      <button onclick="stopStream()">⏹️ Stop Stream</button>
    </div>
  </div>
  <script>
    function startStream() {
      document.getElementById('stream').src = window.location.origin + ':81/stream';
    }
    function stopStream() {
      document.getElementById('stream').src = '';
    }
    // Auto-start stream on load
    window.addEventListener('load', startStream);
  </script>
</body>
</html>
)rawliteral";

static esp_err_t index_handler(httpd_req_t *req) {
  Serial.println("📄 Index page request");
  httpd_resp_set_type(req, "text/html");
  esp_err_t res = httpd_resp_send(req, INDEX_HTML, strlen(INDEX_HTML));
  Serial.printf("📤 Index sent: %s\n", res == ESP_OK ? "OK" : "FAILED");
  return res;
}

static esp_err_t capture_handler(httpd_req_t *req) {
  Serial.println("📸 Capture request received");
  
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("❌ Camera capture failed!");
    const char *msg = "Camera capture failed";
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, msg);
    return ESP_FAIL;
  }
  
  Serial.printf("✅ Captured: %u bytes, %dx%d\n", fb->len, fb->width, fb->height);
  
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);
  esp_camera_fb_return(fb);
  
  Serial.printf("📤 Sent capture: %s\n", res == ESP_OK ? "OK" : "FAILED");
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
  unsigned long last_frame_time = millis();
  
  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("❌ Stream: Camera capture failed");
      res = ESP_FAIL;
      break;
    }

    _jpg_buf_len = fb->len;
    _jpg_buf = fb->buf;

    if (frame_count % 30 == 0) {
      unsigned long now = millis();
      float fps = frame_count == 0 ? 0 : 30000.0 / (now - last_frame_time);
      Serial.printf("📹 Frame %d: %u bytes, %dx%d, %.1f fps, Heap: %u\n", 
                    frame_count, _jpg_buf_len, fb->width, fb->height, fps, ESP.getFreeHeap());
      last_frame_time = now;
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
    
    esp_camera_fb_return(fb);
    fb = NULL;
    _jpg_buf = NULL;
    
    if (res != ESP_OK) {
      Serial.printf("❌ Stream send failed at frame %d, error: %d\n", frame_count, res);
      break;
    }
    
    frame_count++;
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
  config.recv_wait_timeout = 10;
  config.send_wait_timeout = 10;
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
  config.frame_size = FRAMESIZE_SVGA;  // 800x600
  config.jpeg_quality = 12;
  config.fb_count = 2;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.grab_mode = CAMERA_GRAB_LATEST;

  Serial.printf("  Resolution: 800x600 (SVGA)\n");
  Serial.printf("  JPEG Quality: 12\n");
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
  
  // Connect to WiFi
  Serial.println("\n================================================");
  Serial.println("📡 WiFi Configuration");
  Serial.println("================================================");
  Serial.printf("SSID: %s\n", ssid);
  Serial.printf("Password: %s\n", password);
  Serial.println("================================================\n");
  
  Serial.print("Connecting to WiFi");
  
  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
    esp_task_wdt_reset();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n✅ WiFi connected!");
    Serial.printf("IP Address: http://%s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Stream URL: http://%s:81/stream\n", WiFi.localIP().toString().c_str());
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
    Serial.println("\n❌ WiFi connection failed!");
    Serial.println("Please check your WiFi credentials in config.h");
  }
}

void loop() {
  delay(5000);
  esp_task_wdt_reset();
  
  static unsigned long last_status = 0;
  if (millis() - last_status > 60000) {
    Serial.printf("💓 Alive - Free heap: %u, WiFi: %s, RSSI: %d dBm\n", 
                  ESP.getFreeHeap(), 
                  WiFi.status() == WL_CONNECTED ? "Connected" : "Disconnected",
                  WiFi.RSSI());
    last_status = millis();
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("[%lu sec] Server running | IP: %s | Free Heap: %d | PSRAM: %d\n",
                  millis()/1000,
                  WiFi.localIP().toString().c_str(),
                  ESP.getFreeHeap(),
                  ESP.getFreePsram());
  } else {
    Serial.println("WiFi disconnected! Reconnecting...");
    WiFi.reconnect();
  }
}
