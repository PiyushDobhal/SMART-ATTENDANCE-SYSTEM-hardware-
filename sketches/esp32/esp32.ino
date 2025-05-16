#include <WiFi.h>
#include <HTTPClient.h>
#include "esp_camera.h"
#include <base64.h>

/* adjust these for your backend */
#define BACKEND_HOST "https://smart-attendance-system-l486.onrender.com"
#define ENROL_POLL_MS 2000
#define ARD_BAUD 57600  // to Arduino: Serial

const char* SSIDS[] = { "Piyush", "SaurabhMishra", "DIT_CHANAKYA" };
const char* PASSES[] = { "12345678", "12345678", "12345678" };
const uint8_t NET_COUNT = sizeof(SSIDS) / sizeof(SSIDS[0]);

unsigned long lastPoll = 0;

void connectWiFi() {
  for (uint8_t i = 0; i < NET_COUNT; i++) {
    WiFi.begin(SSIDS[i], PASSES[i]);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 5000) {
      delay(200);
    }
    if (WiFi.status() == WL_CONNECTED) return;
  }
  while (WiFi.status() != WL_CONNECTED) delay(1000);
}

bool snapJpeg(String& out) {
  camera_fb_t* fb = esp_camera_fb_get();
  if (!fb || fb->format != PIXFORMAT_JPEG) return false;
  out = base64::encode(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  return true;
}

void setup() {
  Serial.begin(ARD_BAUD);
  connectWiFi();

  camera_config_t cfg = {};
  cfg.ledc_channel = LEDC_CHANNEL_0;
  cfg.ledc_timer = LEDC_TIMER_0;
  cfg.pin_d0 = 5;
  cfg.pin_d1 = 18;
  cfg.pin_d2 = 19;
  cfg.pin_d3 = 21;
  cfg.pin_d4 = 36;
  cfg.pin_d5 = 39;
  cfg.pin_d6 = 34;
  cfg.pin_d7 = 35;
  cfg.pin_xclk = 0;
  cfg.pin_pclk = 22;
  cfg.pin_vsync = 25;
  cfg.pin_href = 23;
  cfg.pin_sscb_sda = 26;
  cfg.pin_sscb_scl = 27;
  cfg.pin_pwdn = 32;
  cfg.pin_reset = -1;
  cfg.xclk_freq_hz = 20000000;
  cfg.pixel_format = PIXFORMAT_JPEG;
  cfg.frame_size = FRAMESIZE_VGA;
  cfg.jpeg_quality = 30;
  cfg.fb_count = 1;

  if (esp_camera_init(&cfg) != ESP_OK) {
    while (1) delay(100);
  }
}

void loop() {
  // 1) Grab a frame
  String jpeg;
  if (!snapJpeg(jpeg)) return;

  // 2) Send to face/identify
  HTTPClient http;
  http.begin(String(BACKEND_HOST) + "/api/face/identify");
  http.addHeader("Content-Type", "application/json");
  String payload = "{\"image\":\"" + jpeg + "\"}";
  int code = http.POST(payload);
  String resp = http.getString();
  http.end();

  // 3) If recognized, parse slot & sapId, and send ENROL: or CHECK:
  if (code == 200 && resp.indexOf("\"recognized\":true") >= 0) {
    // parse sapId
    int a = resp.indexOf("\"sapId\":\"") + 9;
    int b = resp.indexOf("\"", a);
    String sap = resp.substring(a, b);

    // parse fingerprintId *robustly*, look for the number after "fingerprintId":
    int fidKey = resp.indexOf("\"fingerprintId\":");
    if (fidKey >= 0) {
      int start = resp.indexOf(":", fidKey) + 1;
      int end = resp.indexOf("}", start);  // last brace
      if (end < 0) end = resp.length();
      String slotStr = resp.substring(start, end);
      slotStr.trim();              // remove any spaces/newlines
      int slot = slotStr.toInt();  // now will correctly be -1, 0, 1, 2, …

      if (slot < 0) {
        // no slot → enroll
        Serial.print("ENROL:");
        Serial.println(sap);
      } else {
        // has slot → check
        Serial.print("CHECK:");
        Serial.println(slot);
      }
    }
  }


  // 4) Relay Arduino’s response back to backend
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    HTTPClient h2;
    h2.begin(String(BACKEND_HOST) + "/api/enrol/from-device");
    h2.addHeader("Content-Type", "text/plain");
    h2.POST(line);
    h2.end();
  }
}
