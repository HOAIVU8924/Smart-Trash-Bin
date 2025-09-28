#include "esp_camera.h"
#include <WiFi.h>
#include <HTTPClient.h>

// Select camera model
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// WiFi credentials
const char* ssid = "...";
const char* password = "08092004";

WiFiServer server(80);

// Server Node.js IP (chỉnh lại đúng IP máy bạn chạy Node)
const char* serverUrl = "http://192.168.23.156:3000/uploads";

// HTML content
const char* htmlContent = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>Hoc Ky Thuat Channel</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      text-align: center;
      background-color: #f0f0f0;
    }
    h1 { color: #333; }
    .button {
      padding: 15px 30px;
      margin: 10px;
      font-size: 16px;
      cursor: pointer;
      background-color: #4CAF50;
      color: white;
      border: none;
      border-radius: 5px;
    }
    .button:hover { background-color: #45a049; }
    #camImage { max-width: 100%; height: auto; margin-top: 20px; }
  </style>
</head>
<body>
  <h1>Hoc Ky Thuat Channel</h1>
  <button class="button" onclick="capturePhoto()">Capture</button>
  <br>
  <img id="camImage" src="">
  <script>
    function capturePhoto() {
      fetch('/capture')
        .then(response => response.text())
        .then(data => {
          document.getElementById('camImage').src = '/photo?' + new Date().getTime();
        });
    }
  </script>
</body>
</html>
)rawliteral";

// Hàm gửi ảnh JPEG lên server
bool sendPhotoToServer(camera_fb_t *fb) {
  if (!fb) return false;

  WiFiClient client;
  HTTPClient http;

  String boundary = "123456789";
  String startRequest = "--" + boundary + "\r\n";
  startRequest += "Content-Disposition: form-data; name=\"image\"; filename=\"photo.jpg\"\r\n";
  startRequest += "Content-Type: image/jpeg\r\n\r\n";

  String endRequest = "\r\n--" + boundary + "--\r\n";

  int totalLen = startRequest.length() + fb->len + endRequest.length();

  // Tạo buffer chứa toàn bộ body
  uint8_t *body = (uint8_t *)malloc(totalLen);
  if (!body) {
    Serial.println("Không đủ bộ nhớ để tạo buffer");
    return false;
  }

  // Copy startRequest
  memcpy(body, startRequest.c_str(), startRequest.length());
  // Copy ảnh JPEG
  memcpy(body + startRequest.length(), fb->buf, fb->len);
  // Copy endRequest
  memcpy(body + startRequest.length() + fb->len, endRequest.c_str(), endRequest.length());

  http.begin(client, serverUrl);
  http.addHeader("Content-Type", "multipart/form-data; boundary=" + boundary);

  int responseCode = http.POST(body, totalLen);
  free(body); // giải phóng bộ nhớ

  if (responseCode > 0) {
    Serial.printf("Upload response code: %d\n", responseCode);
  } else {
    Serial.printf("HTTP request failed: %d\n", responseCode);
  }

  http.end();
  return responseCode == 200;
}


void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  // Camera config
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;
  config.jpeg_quality = 12;
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;

  if (psramFound()) {
    config.jpeg_quality = 10;
    config.fb_count = 2;
    config.grab_mode = CAMERA_GRAB_LATEST;
  }

  // Init camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return;
  }

  // WiFi connect
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
  Serial.println(WiFi.localIP());

  // Start server
  server.begin();
}

void loop() {
  WiFiClient client = server.available();
  if (client) {
    String req = "";
    bool currentLineIsBlank = true;

    while (client.connected()) {
      if (client.a()) {
        char c = client.read();
        req += c;

        // Xử lý yêu cầu GET /capture
        if (req.indexOf("GET /capture") >= 0) {
          camera_fb_t * fb = esp_camera_fb_get();
          if (fb) {
            sendPhotoToServer(fb);
            esp_camera_fb_return(fb);
          }
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/plain");
          client.println();
          client.println("Photo captured");
          break;
        }

        // Xử lý yêu cầu GET /photo
        if (req.indexOf("GET /photo") >= 0) {
          camera_fb_t * fb = esp_camera_fb_get();
          if (fb) {
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:image/jpeg");
            client.println("Content-Length: " + String(fb->len));
            client.println();
            client.write(fb->buf, fb->len);
            esp_camera_fb_return(fb);
          } else {
            client.println("HTTP/1.1 500 Internal Server Error");
          }
          break;
        }

        // Xử lý yêu cầu mặc định (GET /)
        if (req.indexOf("GET / ") >= 0 || req.indexOf("GET / HTTP") >= 0) {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.print(htmlContent);
          break;
        }

        // Kết thúc yêu cầu HTTP
        if (c == '\n' && currentLineIsBlank) break;
        if (c == '\n') currentLineIsBlank = true;
        else if (c != '\r') currentLineIsBlank = false;
      }
    }

    delay(1);
    client.stop();
  }
}