
#include <WiFi.h>
#include "img_converters.h"
#include <TFT_eSPI.h>
#include "ui.h"   
#include <lvgl.h>   

extern TFT_eSPI tft;  

extern "C" {
    void camera_setup();
    void camera_open();
    void camera_loop();
    int  camera_is_active();
    void camera_request_open();
    int  camera_consume_open_request();
}


static const char* CAM_AP_SSID   = "ESP32CAM-Stream";
static const char* CAM_AP_PASS   = "camera1234";
static const char* CAM_SERVER_IP = "192.168.4.1";  
static const uint16_t CAM_SERVER_PORT = 8080;


static const int CAM_FRAME_W = 320;
static const int CAM_FRAME_H = 240;

#define CAM_MAX_JPEG_SIZE  32768
#define CAM_WIFI_CONNECT_TIMEOUT_MS   8000
#define CAM_TCP_CONNECT_RETRY_MS      1000

#define CAM_DEBUG 1
#if CAM_DEBUG
  #define CAM_LOG(...) Serial.printf(__VA_ARGS__)
#else
  #define CAM_LOG(...)
#endif

// ── App state ──────────────────────────────────────────────
static bool camera_active         = false;
static bool camera_open_requested = false;

enum CamState { CAM_ST_CONNECTING_WIFI, CAM_ST_CONNECTING_TCP, CAM_ST_STREAMING, CAM_ST_ERROR };
static CamState cam_state = CAM_ST_CONNECTING_WIFI;
static String   cam_errMsg = "";
static unsigned long cam_lastTcpAttempt = 0;

static uint8_t* cam_jpegBuf   = nullptr;
static uint8_t* cam_rgb565Buf = nullptr;
static bool     cam_buffersOk = false;

static WiFiClient camClient;

// Layout
#define CAM_SCR_W 480
#define CAM_SCR_H 320
#define CAM_BACK_X 4
#define CAM_BACK_Y 6
#define CAM_BACK_W 38
#define CAM_BACK_H 28


static int  cam_touchX = 0, cam_touchY = 0;
static bool cam_touchValid = false;

static void cam_updateTouch() {
  uint16_t tx, ty;
  cam_touchValid = tft.getTouch(&tx, &ty, 600);
  if (cam_touchValid) { cam_touchX = tx; cam_touchY = ty; }
}

static bool cam_touchIn(int x, int y, int w, int h) {
  return cam_touchValid && cam_touchX >= x && cam_touchX < x + w &&
         cam_touchY >= y && cam_touchY < y + h;
}

static void cam_drawBackButton() {
 
  tft.fillRoundRect(CAM_BACK_X, CAM_BACK_Y, CAM_BACK_W, CAM_BACK_H, 5, TFT_BLACK);
  tft.drawRoundRect(CAM_BACK_X, CAM_BACK_Y, CAM_BACK_W, CAM_BACK_H, 5, TFT_CYAN);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(1);
  tft.setCursor(CAM_BACK_X + 8, CAM_BACK_Y + 10);
  tft.print("< ");
}

static void cam_drawStatus(const String& line1, const String& line2 = "") {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.setTextSize(2);
  tft.setCursor(20, 130);
  tft.print(line1);
  if (line2.length()) {
    tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
    tft.setTextSize(1);
    tft.setCursor(20, 160);
    tft.print(line2);
  }
  cam_drawBackButton();
}

static unsigned long cam_wifiStartedAt = 0;

static void cam_beginWifiConnect() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true, true); 
  delay(50);
  WiFi.begin(CAM_AP_SSID, CAM_AP_PASS);
  cam_wifiStartedAt = millis();
  cam_state = CAM_ST_CONNECTING_WIFI;
  CAM_LOG("[camera] connecting to %s...\n", CAM_AP_SSID);
  cam_drawStatus("Connecting to camera...", CAM_AP_SSID);
}

static bool cam_tryConnectTcp() {
  if (millis() - cam_lastTcpAttempt < CAM_TCP_CONNECT_RETRY_MS) return false;
  cam_lastTcpAttempt = millis();
  CAM_LOG("[camera] attempting TCP connect to %s:%d\n", CAM_SERVER_IP, CAM_SERVER_PORT);
  if (camClient.connect(CAM_SERVER_IP, CAM_SERVER_PORT)) {
    CAM_LOG("[camera] TCP connected\n");
    return true;
  }
  CAM_LOG("[camera] TCP connect failed, will retry\n");
  return false;
}


static bool cam_readExact(uint8_t* dest, size_t len) {
  size_t got = 0;
  unsigned long start = millis();
  while (got < len) {
    if (!camClient.connected()) return false;
    int avail = camClient.available();
    if (avail > 0) {
      int toRead = min((size_t)avail, len - got);
      int n = camClient.read(dest + got, toRead);
      if (n > 0) got += n;
    }
  
    if (millis() - start > 2000) return false;
  }
  return true;
}

static void cam_pumpFrame() {
  if (!camClient.connected()) {
    CAM_LOG("[camera] stream disconnected, will retry TCP\n");
    cam_state = CAM_ST_CONNECTING_TCP;
    cam_drawStatus("Stream lost", "Reconnecting...");
    return;
  }

  uint8_t header[4];
  if (!cam_readExact(header, 4)) return;   

  uint32_t jpegLen = header[0] | (header[1] << 8) | (header[2] << 16) | ((uint32_t)header[3] << 24);
  if (jpegLen == 0 || jpegLen > CAM_MAX_JPEG_SIZE) {
    CAM_LOG("[camera] bad frame length %u, resyncing\n", jpegLen);
    camClient.stop();
    return;
  }

  if (!cam_readExact(cam_jpegBuf, jpegLen)) return;  

  if (jpg2rgb565(cam_jpegBuf, jpegLen, cam_rgb565Buf, JPG_SCALE_NONE)) {
    int16_t x_offset = (tft.width()  - CAM_FRAME_W) / 2;
    int16_t y_offset = (tft.height() - CAM_FRAME_H) / 2;
    if (x_offset < 0) x_offset = 0;
    if (y_offset < 0) y_offset = 0;

    tft.setAddrWindow(x_offset, y_offset, CAM_FRAME_W, CAM_FRAME_H);
    tft.pushColors((uint16_t*)cam_rgb565Buf, CAM_FRAME_W * CAM_FRAME_H);
  } else {
    CAM_LOG("[camera] jpg2rgb565 decode failed (%u bytes)\n", jpegLen);
  }


  cam_drawBackButton();
}


void camera_setup() {
  cam_buffersOk = false;
  cam_jpegBuf   = (uint8_t*)ps_malloc(CAM_MAX_JPEG_SIZE);
  cam_rgb565Buf = (uint8_t*)ps_malloc(CAM_FRAME_W * CAM_FRAME_H * 2);

  if (!cam_jpegBuf || !cam_rgb565Buf) {
    Serial.println("[camera] setup: PSRAM buffer allocation FAILED "
                    "(check Tools > PSRAM > OPI PSRAM is enabled)");
    return;
  }
  cam_buffersOk = true;
  CAM_LOG("[camera] setup: OK, buffers allocated\n");
}

void camera_request_open() {
  camera_open_requested = true;
}

int camera_consume_open_request() {
  if (camera_open_requested) {
    camera_open_requested = false;
    return 1;
  }
  return 0;
}

void camera_open() {
  if (!cam_buffersOk) {
    cam_drawStatus("Camera unavailable", "Buffer alloc failed - see log"); 
    camera_active = true;  
    cam_state = CAM_ST_ERROR;
    return;
  }
  camera_active = true;
  cam_beginWifiConnect();
}

void camera_close() {
  camera_active = false;
  if (camClient.connected()) camClient.stop();
  WiFi.disconnect(true, true);

  lv_obj_invalidate(objects.main);
}

void camera_loop() {
  if (!camera_active) return;

  cam_updateTouch();
  static bool wasTouched = false;
  static unsigned long lastTouch = 0;
  bool backTapped = false;

  if (cam_touchValid && !wasTouched && (millis() - lastTouch > 200)) {
    wasTouched = true;
    lastTouch  = millis();
    if (cam_touchIn(CAM_BACK_X, CAM_BACK_Y, CAM_BACK_W, CAM_BACK_H)) {
      backTapped = true;
    }
  }
  if (!cam_touchValid) wasTouched = false;

  if (backTapped) {
    camera_close();
    return;
  }

  switch (cam_state) {
    case CAM_ST_ERROR:
    
      break;

    case CAM_ST_CONNECTING_WIFI:
      if (WiFi.status() == WL_CONNECTED) {
        CAM_LOG("[camera] WiFi connected\n");
        cam_state = CAM_ST_CONNECTING_TCP;
        cam_lastTcpAttempt = 0;
        cam_drawStatus("Connecting to stream...", CAM_SERVER_IP);
      } else if (millis() - cam_wifiStartedAt > CAM_WIFI_CONNECT_TIMEOUT_MS) {
        CAM_LOG("[camera] WiFi connect TIMED OUT\n");
        cam_state = CAM_ST_ERROR;
        cam_drawStatus("Camera not found", "Check it's powered on, tap < to exit");
      }
      break;

    case CAM_ST_CONNECTING_TCP:
      if (cam_tryConnectTcp()) {
        cam_state = CAM_ST_STREAMING;
        tft.fillScreen(TFT_BLACK);
        cam_drawBackButton();
      }
      break;

    case CAM_ST_STREAMING:
      cam_pumpFrame();
      break;
  }
}

int camera_is_active() {
  return camera_active ? 1 : 0;
}
