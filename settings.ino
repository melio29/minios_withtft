
#include <TFT_eSPI.h>
#include <SPI.h>
#include "ui.h"      // for objects.main
#include <lvgl.h>    // for lv_obj_invalidate

#include <WiFi.h>
#include <time.h>


#include <BLEDevice.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

extern TFT_eSPI tft;  // the single tft instance created in main.ino

// These are called from actions.c (plain C) and main.ino (C++).
extern "C" {
    void set_setup();
    void set_open();
    void set_loop();
    int  set_is_active();
    void set_request_open();
    int  set_consume_open_request();
}

#define SET_BUZZER_PIN 27


struct WifiCred { const char* ssid; const char* pass; };
static WifiCred set_knownWifi[] = {
  // {"YourHomeWiFi", "yourpassword"},
  // {"OfficeWiFi",   "anotherpassword"},
};
static const int set_knownWifiCount = sizeof(set_knownWifi) / sizeof(set_knownWifi[0]);

// 2) Set your timezone offset for the Date & Time page (seconds from UTC).
//    Example: India (UTC+5:30) = 19800, UK = 0, US Eastern = -18000.
static long   set_gmtOffsetSec      = 19800;
static int    set_daylightOffsetSec = 0;
static const char* set_ntpServer    = "pool.ntp.org";

// 3) Sound: right now SET_BUZZER_PIN drives a passive buzzer via PWM, so
//    "volume" is approximated by PWM duty cycle. When you wire up a real
//    speaker/amp module (e.g. MAX98357A or PAM8403 + I2S DAC), keep using
//    the `set_volume` (0-100) value to scale your audio samples for real
//    loudness control — the UI and storage already do the rest.
// ============================================================

// ================= NEW COLOR PALETTE (RGB565) =================
// Deep-space indigo/teal/coral theme instead of the old flat cyan look.
#define C_BG        0x1084   // near-black indigo background
#define C_HEADER    0x28A8   // deep violet header bar
#define C_CARD      0x2989   // slate-blue card
#define C_CARD_TAP  0x414B   // brighter violet, used for pressed feedback
#define C_TEAL      0x0656   // primary accent - teal
#define C_CORAL     0xFB6C   // secondary accent - coral
#define C_GOLD      0xFEA9   // highlight accent - gold
#define C_PURPLE    0x9ADA   // tertiary accent - violet
#define C_SKY       0x4D5E   // accent - sky blue
#define C_MINT      0x4ED4   // accent - mint green
#define C_ROSE      0xEA2D   // accent - rose / danger
#define C_SUBTEXT   0xAD79   // muted light gray-blue for secondary text
#define C_TEXT      TFT_WHITE

struct SetMenuItem {
  String name;
  uint16_t color;
};

static SetMenuItem set_menu[] = {
  {"System",       C_TEAL},
  {"Display",      C_GOLD},
  {"Sound",        C_MINT},
  {"WiFi",         C_SKY},
  {"Bluetooth",    C_PURPLE},
  {"Date & Time",  C_CORAL},
  {"Battery",      C_MINT},
  {"Apps",         C_SKY},
  {"Security",     C_ROSE},
  {"About",        C_GOLD}
};

static const int set_totalItems = sizeof(set_menu) / sizeof(set_menu[0]);

static int  set_scrollOffset = 0;
static bool set_touching = false;
static int  set_lastY = 0;

static int  set_brightness = 80;
static int  set_volume     = 60;

// ── App lifecycle state ───────────
static bool set_active = false;
static bool set_open_requested = false;

// ── Page state ───────────
enum SetPage {
  PAGE_MENU = 0,
  PAGE_SYSTEM,
  PAGE_DISPLAY,
  PAGE_SOUND,
  PAGE_WIFI,
  PAGE_BLUETOOTH,
  PAGE_DATETIME,
  PAGE_BATTERY,
  PAGE_APPS,
  PAGE_SECURITY,
  PAGE_ABOUT
};
static SetPage set_currentPage = PAGE_MENU;

// ================= FORWARD DECLARATIONS =================
void set_close();
void set_drawMenu();
void set_drawSystemPage();
void set_handleSystemTouch(uint16_t x, uint16_t y);
void set_drawDisplayPage();
void set_drawSoundPage();
void set_drawWifiPage();
void set_drawBluetoothPage();
void set_drawDateTimePage();
void set_drawBatteryPage();
void set_drawAppsPage();
void set_drawAppInfoPopup(int idx);
void set_drawSecurityPage();
void set_drawAboutPage();
void set_goMenu();
bool set_isBackTap(uint16_t x, uint16_t y);
int  set_volumeDuty();

// ================= SOUND (PWM) =================
// Core 3.x's LEDC API attaches directly to a GPIO pin (no manual channel
// bookkeeping needed) - ledcSetup()/ledcAttachPin() were removed in core 3.x.
#define SOUND_FREQ 3000
#define SOUND_RES  8

int set_volumeDuty() {
  return map(set_volume, 0, 100, 0, 255);
}

void set_beep() {
  ledcWrite(SET_BUZZER_PIN, set_volumeDuty());
  delay(15);
  ledcWrite(SET_BUZZER_PIN, 0);
}

void set_playTestTone() {
  ledcWrite(SET_BUZZER_PIN, set_volumeDuty());
  delay(300);
  ledcWrite(SET_BUZZER_PIN, 0);
}

// ================= ICONS =================

void set_drawGear(int x, int y, uint16_t c) {
  tft.drawCircle(x, y, 10, c);
  tft.drawCircle(x, y, 5, c);

  for (int i = 0; i < 8; i++) {
    float a = i * 45 * 0.0174533;

    tft.drawLine(
      x + cos(a) * 10,
      y + sin(a) * 10,
      x + cos(a) * 14,
      y + sin(a) * 14,
      c
    );
  }
}

void set_drawDisplay(int x, int y, uint16_t c) {
  tft.drawRect(x - 12, y - 8, 24, 16, c);
  tft.drawLine(x - 5, y + 10, x + 5, y + 10, c);
}

void set_drawSpeaker(int x, int y, uint16_t c) {
  tft.fillTriangle(x - 10, y - 6, x - 10, y + 6, x, y, c);
  tft.fillRect(x, y - 5, 5, 10, c);
  tft.drawCircle(x + 10, y, 5, c);
}

void set_drawWifi(int x, int y, uint16_t c) {
  tft.drawCircle(x, y + 8, 2, c);
  tft.drawCircle(x, y, 10, c);
  tft.drawCircle(x, y, 16, c);
}

void set_drawBluetooth(int x, int y, uint16_t c) {
  tft.drawLine(x - 6, y - 10, x + 6, y, c);
  tft.drawLine(x + 6, y, x - 6, y + 10, c);
  tft.drawLine(x - 6, y - 10, x - 6, y + 10, c);
  tft.drawLine(x - 6, y - 10, x + 2, y - 4, c);
  tft.drawLine(x - 6, y + 10, x + 2, y + 4, c);
}

void set_drawClock(int x, int y, uint16_t c) {
  tft.drawCircle(x, y, 12, c);
  tft.drawLine(x, y, x, y - 7, c);
  tft.drawLine(x, y, x + 5, y + 2, c);
}

void set_drawBattery(int x, int y, uint16_t c) {
  tft.drawRect(x - 12, y - 7, 24, 14, c);
  tft.fillRect(x + 12, y - 3, 3, 6, c);
  tft.fillRect(x - 8, y - 4, 12, 8, c);
}

void set_drawLock(int x, int y, uint16_t c) {
  tft.drawRect(x - 8, y, 16, 14, c);
  tft.drawCircle(x, y - 4, 6, c);
}

void set_drawApps(int x, int y, uint16_t c) {
  for (int r = 0; r < 2; r++) {
    for (int col = 0; col < 2; col++) {
      tft.fillRect(x - 10 + col * 12, y - 10 + r * 12, 8, 8, c);
    }
  }
}

void set_drawInfo(int x, int y, uint16_t c) {
  tft.drawCircle(x, y, 10, c);
  tft.drawString("i", x - 3, y - 7, 2);
}

// ================= SHARED HEADER / BACK =================

void set_drawHeader() {
  tft.fillRect(0, 0, 480, 50, C_HEADER);

  tft.fillRoundRect(4, 8, 34, 34, 6, C_CARD);
  tft.setTextColor(C_TEXT, C_CARD);
  tft.setTextSize(1);
  tft.drawString("<", 16, 18, 2);

  tft.setTextColor(C_TEXT, C_HEADER);
  tft.setTextSize(2);
  tft.drawString("MiniOS Settings", 48, 15);

  set_drawGear(450, 25, C_GOLD);
}

// Sub-page header: title bar + a "Back" pill at the bottom that returns
// to the main menu (matches the original Display-page convention).
void set_drawSubHeader(const char* title, uint16_t accent) {
  tft.fillScreen(C_BG);
  tft.fillRect(0, 0, 480, 50, C_HEADER);
  tft.setTextColor(C_TEXT, C_HEADER);
  tft.setTextSize(2);
  tft.drawString(title, 15, 15);
  tft.fillRect(0, 47, 480, 3, accent);
}

void set_drawBackButton() {
  tft.fillRoundRect(10, 260, 120, 45, 10, C_CARD);
  tft.setTextColor(C_TEXT, C_CARD);
  tft.setTextSize(1);
  tft.drawString("Back", 40, 272, 2);
}

bool set_isBackTap(uint16_t x, uint16_t y) {
  return x > 10 && x < 130 && y > 260 && y < 305;
}

void set_goMenu() {
  set_currentPage = PAGE_MENU;
  set_drawMenu();
}

// ================= MENU ITEM =================

void set_drawItem(int idx, int y) {
  tft.fillRoundRect(10, y, 460, 50, 10, C_CARD);

  int iconX = 35;
  int iconY = y + 25;
  uint16_t c = set_menu[idx].color;

  switch (idx) {
    case 0: set_drawGear(iconX, iconY, c); break;
    case 1: set_drawDisplay(iconX, iconY, c); break;
    case 2: set_drawSpeaker(iconX, iconY, c); break;
    case 3: set_drawWifi(iconX, iconY, c); break;
    case 4: set_drawBluetooth(iconX, iconY, c); break;
    case 5: set_drawClock(iconX, iconY, c); break;
    case 6: set_drawBattery(iconX, iconY, c); break;
    case 7: set_drawApps(iconX, iconY, c); break;
    case 8: set_drawLock(iconX, iconY, c); break;
    case 9: set_drawInfo(iconX, iconY, c); break;
  }

  tft.setTextColor(C_TEXT, C_CARD);
  tft.drawString(set_menu[idx].name, 65, y + 15, 2);

  tft.setTextColor(c, C_CARD);
  tft.drawString(">", 440, y + 15, 2);
}

// ================= DRAW MENU =================

void set_drawMenu() {
  tft.fillScreen(C_BG);
  set_drawHeader();

  int itemH = 60;

  for (int i = 0; i < set_totalItems; i++) {
    int y = 60 + (i * itemH) - set_scrollOffset;

    if (y > -60 && y < 320)
      set_drawItem(i, y);
  }
}

// ================= DISPLAY PAGE =================

void set_drawDisplayPage() {
  set_drawSubHeader("Display", C_GOLD);

  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("Brightness", 40, 90, 2);

  tft.fillRoundRect(180, 75, 120, 40, 10, C_CARD);
  tft.setTextColor(C_GOLD, C_CARD);
  tft.drawCentreString(String(set_brightness) + "%", 240, 88, 2);

  tft.fillRoundRect(80, 170, 90, 60, 10, C_ROSE);
  tft.setTextColor(C_TEXT, C_ROSE);
  tft.drawCentreString("-", 125, 188, 4);

  tft.fillRoundRect(310, 170, 90, 60, 10, C_MINT);
  tft.setTextColor(C_TEXT, C_MINT);
  tft.drawCentreString("+", 355, 188, 4);

  set_drawBackButton();
}

void set_handleDisplayTouch(uint16_t x, uint16_t y) {
  if (x > 80 && x < 170 && y > 170 && y < 230) {
    if (set_brightness > 0) set_brightness -= 10;
    // NOTE: this is where you'd drive a real backlight PWM pin, e.g.
    // ledcWrite(BACKLIGHT_PIN, map(set_brightness, 0, 100, 0, 255));
    set_drawDisplayPage();
    return;
  }
  if (x > 310 && x < 400 && y > 170 && y < 230) {
    if (set_brightness < 100) set_brightness += 10;
    set_drawDisplayPage();
    return;
  }
  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

// ================= SYSTEM PAGE (mobile-style "About display") =================

void set_drawSystemPage() {
  set_drawSubHeader("System", C_TEAL);

  int y = 75;
  int lh = 30;

  tft.setTextColor(C_SUBTEXT, C_BG); tft.drawString("Screen resolution", 25, y, 2);
  tft.setTextColor(C_TEAL, C_BG);
  tft.drawString(String(tft.width()) + " x " + String(tft.height()) + " px", 300, y, 2);
  y += lh;

  tft.setTextColor(C_SUBTEXT, C_BG); tft.drawString("Color depth", 25, y, 2);
  tft.setTextColor(C_TEAL, C_BG); tft.drawString("16-bit (65K colors)", 300, y, 2);
  y += lh;

  tft.setTextColor(C_SUBTEXT, C_BG); tft.drawString("Touch panel", 25, y, 2);
  tft.setTextColor(C_MINT, C_BG); tft.drawString("Enabled", 300, y, 2);
  y += lh;

  tft.setTextColor(C_SUBTEXT, C_BG); tft.drawString("Rotation", 25, y, 2);
  tft.setTextColor(C_TEAL, C_BG); tft.drawString(String(tft.getRotation()), 300, y, 2);
  y += lh;

  tft.setTextColor(C_SUBTEXT, C_BG); tft.drawString("Free heap", 25, y, 2);
  tft.setTextColor(C_TEAL, C_BG); tft.drawString(String(ESP.getFreeHeap() / 1024) + " KB", 300, y, 2);
  y += lh;

  unsigned long s = millis() / 1000;
  tft.setTextColor(C_SUBTEXT, C_BG); tft.drawString("Uptime", 25, y, 2);
  tft.setTextColor(C_TEAL, C_BG);
  tft.drawString(String(s / 3600) + "h " + String((s % 3600) / 60) + "m " + String(s % 60) + "s", 300, y, 2);

  set_drawBackButton();
}

// ================= SOUND PAGE =================

void set_drawSoundPage() {
  set_drawSubHeader("Sound", C_MINT);

  tft.setTextColor(C_TEXT, C_BG);
  tft.drawString("Volume", 40, 90, 2);

  tft.fillRoundRect(180, 75, 120, 40, 10, C_CARD);
  tft.setTextColor(C_MINT, C_CARD);
  tft.drawCentreString(String(set_volume) + "%", 240, 88, 2);

  tft.fillRoundRect(60, 170, 80, 60, 10, C_ROSE);
  tft.setTextColor(C_TEXT, C_ROSE);
  tft.drawCentreString("-", 100, 188, 4);

  tft.fillRoundRect(330, 170, 80, 60, 10, C_MINT);
  tft.setTextColor(C_TEXT, C_MINT);
  tft.drawCentreString("+", 370, 188, 4);

  tft.fillRoundRect(160, 170, 150, 60, 10, C_TEAL);
  tft.setTextColor(C_TEXT, C_TEAL);
  tft.drawCentreString("Test Sound", 235, 190, 2);

  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawString("Buzzer pin " + String(SET_BUZZER_PIN) + " (PWM). Swap in a", 25, 245, 1);
  tft.drawString("real speaker/amp later - volume scales the same way.", 25, 258, 1);

  set_drawBackButton();
}

void set_handleSoundTouch(uint16_t x, uint16_t y) {
  if (x > 60 && x < 140 && y > 170 && y < 230) {
    if (set_volume > 0) set_volume -= 10;
    set_drawSoundPage();
    return;
  }
  if (x > 330 && x < 410 && y > 170 && y < 230) {
    if (set_volume < 100) set_volume += 10;
    set_drawSoundPage();
    return;
  }
  if (x > 160 && x < 310 && y > 170 && y < 230) {
    set_playTestTone();
    return;
  }
  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

// ================= WIFI PAGE =================

#define WIFI_MAX_LIST     8   // how many scan results we keep in memory
#define WIFI_VISIBLE_ROWS 4   // how many rows actually fit on screen without overlap
static bool   wifi_scanning     = false;
static int    wifi_netCount     = -1;   // -1 = never scanned
static String wifi_status       = "Tap Scan to search for networks";
static wl_status_t wifi_lastStatus = WL_IDLE_STATUS;

void set_startWifiScan() {
  WiFi.mode(WIFI_STA);
  wifi_scanning = true;
  wifi_status = "Scanning...";
  WiFi.scanNetworks(true); // async - does NOT disconnect an existing connection
}

void set_drawWifiPage() {
  set_drawSubHeader("Wi-Fi", C_SKY);

  String countTxt;
  if (wifi_scanning) countTxt = "Scanning...";
  else if (wifi_netCount < 0) countTxt = "Not scanned yet";
  else countTxt = String(wifi_netCount) + " network(s) found";

  tft.setTextColor(C_SKY, C_BG);
  tft.drawString(countTxt, 20, 58, 2);

  tft.fillRoundRect(370, 52, 95, 32, 8, C_TEAL);
  tft.setTextColor(C_TEXT, C_TEAL);
  tft.drawCentreString("Scan", 417, 60, 2);

  int rowH = 34;
  int top = 92;
  int shown = wifi_netCount > 0 ? min(wifi_netCount, min(WIFI_MAX_LIST, WIFI_VISIBLE_ROWS)) : 0;

  for (int i = 0; i < shown; i++) {
    int y = top + i * rowH;
    bool connected = (WiFi.status() == WL_CONNECTED && WiFi.SSID() == WiFi.SSID(i));
    tft.fillRoundRect(15, y, 450, rowH - 4, 6, connected ? C_CARD_TAP : C_CARD);

    wifi_auth_mode_t enc = WiFi.encryptionType(i);
    bool open = (enc == WIFI_AUTH_OPEN);

    int32_t rssi = WiFi.RSSI(i);
    uint16_t sigColor = rssi > -60 ? C_MINT : (rssi > -75 ? C_GOLD : C_ROSE);
    // simple signal bars
    for (int b = 0; b < 3; b++) {
      int barH = 6 + b * 5;
      int bx = 25 + b * 8;
      bool lit = (rssi > -80 + b * 15);
      tft.fillRect(bx, y + (rowH - 4) - barH - 4, 5, barH, lit ? sigColor : C_SUBTEXT);
    }

    tft.setTextColor(C_TEXT, connected ? C_CARD_TAP : C_CARD);
    tft.drawString(WiFi.SSID(i), 55, y + 8, 2);

    if (!open) {
      tft.setTextColor(C_GOLD, connected ? C_CARD_TAP : C_CARD);
      tft.drawString("locked", 380, y + 8, 1);
    }
    if (connected) {
      tft.setTextColor(C_MINT, C_CARD_TAP);
      tft.drawString("connected", 350, y + 8, 1);
    }
  }

  if (wifi_netCount > WIFI_VISIBLE_ROWS) {
    tft.setTextColor(C_SUBTEXT, C_BG);
    tft.drawString("+" + String(wifi_netCount - WIFI_VISIBLE_ROWS) + " more - rescan to refresh", 20, top + shown * rowH + 4, 1);
  }

  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawString(wifi_status, 20, 246, 1);

  set_drawBackButton();
}

void set_tryConnectWifi(int idx) {
  String ssid = WiFi.SSID(idx);
  wifi_auth_mode_t enc = WiFi.encryptionType(idx);

  if (enc == WIFI_AUTH_OPEN) {
    WiFi.begin(ssid.c_str());
    wifi_status = "Connecting to " + ssid + "...";
    return;
  }

  for (int i = 0; i < set_knownWifiCount; i++) {
    if (ssid == String(set_knownWifi[i].ssid)) {
      WiFi.begin(set_knownWifi[i].ssid, set_knownWifi[i].pass);
      wifi_status = "Connecting to " + ssid + "...";
      return;
    }
  }

  wifi_status = ssid + " needs a password - add it to set_knownWifi[] in code";
}

void set_handleWifiTouch(uint16_t x, uint16_t y) {
  if (x > 370 && x < 465 && y > 52 && y < 84) {
    set_startWifiScan();
    set_drawWifiPage();
    return;
  }

  int rowH = 34, top = 92;
  int shown = wifi_netCount > 0 ? min(wifi_netCount, min(WIFI_MAX_LIST, WIFI_VISIBLE_ROWS)) : 0;
  for (int i = 0; i < shown; i++) {
    int yy = top + i * rowH;
    if (x > 15 && x < 465 && y > yy && y < yy + rowH - 4) {
      set_tryConnectWifi(i);
      set_drawWifiPage();
      return;
    }
  }

  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

// Called continuously from set_loop() while the Wi-Fi page is open, so an
// async scan gets picked up as soon as it ends, AND so we notice when a
// begin()-triggered connection actually succeeds/fails in the background.
void set_pollWifi() {
  if (wifi_scanning) {
    int n = WiFi.scanComplete();
    if (n >= 0) {
      wifi_netCount = n;
      wifi_scanning = false;
      wifi_status = "Tap a network to connect";
      set_drawWifiPage();
    } else if (n == WIFI_SCAN_FAILED) {
      wifi_scanning = false;
      wifi_netCount = 0;
      wifi_status = "Scan failed - try again";
      set_drawWifiPage();
    }
  }

  wl_status_t st = WiFi.status();
  if (st != wifi_lastStatus) {
    wifi_lastStatus = st;
    if (st == WL_CONNECTED) {
      wifi_status = "Connected to " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")";
      set_drawWifiPage();
    } else if (st == WL_CONNECT_FAILED || st == WL_CONNECTION_LOST) {
      wifi_status = "Connection failed - wrong password?";
      set_drawWifiPage();
    } else if (st == WL_DISCONNECTED && wifi_status.startsWith("Connecting")) {
      // still mid-attempt, WiFi.begin() briefly reports DISCONNECTED first - ignore
    }
  }
}

// ================= BLUETOOTH (BLE) PAGE =================

#define BLE_MAX_LIST     8   // how many scan results we keep in memory
#define BLE_VISIBLE_ROWS 4   // how many rows actually fit on screen without overlap
static bool       ble_initialized = false;
static BLEScan*   ble_scanner     = nullptr;
static int        ble_deviceCount = -1;
static String     ble_names[BLE_MAX_LIST];
static BLEAddress ble_addrObjs[BLE_MAX_LIST]; // keeps the real address type (public/random) - do not round-trip through String
static int        ble_rssi[BLE_MAX_LIST];
static int        ble_connectedIdx = -1;
static String     ble_status = "Tap Scan to search for devices";

class SetBLECallback : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) override {
    if (ble_deviceCount >= BLE_MAX_LIST) return;
    String nm = advertisedDevice.getName().length()
                  ? String(advertisedDevice.getName().c_str())
                  : String("(unnamed device)");
    ble_names[ble_deviceCount]    = nm;
    ble_addrObjs[ble_deviceCount] = advertisedDevice.getAddress(); // keeps address type intact
    ble_rssi[ble_deviceCount]     = advertisedDevice.getRSSI();
    ble_deviceCount++;
  }
};

void set_bleInit() {
  if (ble_initialized) return;
  BLEDevice::init("MiniOS");
  ble_scanner = BLEDevice::getScan();
  ble_scanner->setAdvertisedDeviceCallbacks(new SetBLECallback(), false);
  ble_scanner->setActiveScan(true);
  ble_scanner->setInterval(100);
  ble_scanner->setWindow(99);
  ble_initialized = true;
}

// Blocking ~3s scan - simplest and most compatible across core versions.
void set_startBleScan() {
  set_bleInit();
  ble_deviceCount = 0;
  ble_connectedIdx = -1;
  ble_status = "Scanning...";
  set_drawBluetoothPage();
  ble_scanner->start(3, false);
  ble_scanner->stop();
  ble_scanner->clearResults();
  ble_status = ble_deviceCount > 0 ? "Tap a device to connect" : "No devices found";
}

void set_drawBluetoothPage() {
  set_drawSubHeader("Bluetooth", C_PURPLE);

  String countTxt = (ble_deviceCount < 0) ? "Not scanned yet" : (String(ble_deviceCount) + " device(s) found");
  tft.setTextColor(C_PURPLE, C_BG);
  tft.drawString(countTxt, 20, 58, 2);

  tft.fillRoundRect(370, 52, 95, 32, 8, C_TEAL);
  tft.setTextColor(C_TEXT, C_TEAL);
  tft.drawCentreString("Scan", 417, 60, 2);

  int rowH = 34, top = 92;
  int shown = ble_deviceCount > 0 ? min(ble_deviceCount, min(BLE_MAX_LIST, BLE_VISIBLE_ROWS)) : 0;

  for (int i = 0; i < shown; i++) {
    int y = top + i * rowH;
    bool connected = (i == ble_connectedIdx);
    tft.fillRoundRect(15, y, 450, rowH - 4, 6, connected ? C_CARD_TAP : C_CARD);

    set_drawBluetooth(30, y + 13, C_PURPLE);

    tft.setTextColor(C_TEXT, connected ? C_CARD_TAP : C_CARD);
    tft.drawString(ble_names[i], 55, y + 4, 2);
    tft.setTextColor(C_SUBTEXT, connected ? C_CARD_TAP : C_CARD);
    String addrStr = String(ble_addrObjs[i].toString().c_str());
    tft.drawString(addrStr + "  " + String(ble_rssi[i]) + "dBm", 55, y + 18, 1);

    if (connected) {
      tft.setTextColor(C_MINT, C_CARD_TAP);
      tft.drawString("connected", 375, y + 10, 1);
    }
  }

  if (ble_deviceCount > BLE_VISIBLE_ROWS) {
    tft.setTextColor(C_SUBTEXT, C_BG);
    tft.drawString("+" + String(ble_deviceCount - BLE_VISIBLE_ROWS) + " more - rescan to refresh", 20, top + shown * rowH + 4, 1);
  }

  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawString(ble_status, 20, 246, 1);

  set_drawBackButton();
}

void set_tryConnectBle(int idx) {
  BLEClient* client = BLEDevice::createClient();
  bool ok = client->connect(ble_addrObjs[idx]); // address type is preserved, unlike rebuilding from a string
  if (ok) {
    ble_connectedIdx = idx;
    ble_status = "Connected to " + ble_names[idx];
  } else {
    ble_status = "Could not connect to " + ble_names[idx] + " - device may be out of range or not connectable";
  }
}

void set_handleBluetoothTouch(uint16_t x, uint16_t y) {
  if (x > 370 && x < 465 && y > 52 && y < 84) {
    set_startBleScan();
    set_drawBluetoothPage();
    return;
  }

  int rowH = 34, top = 92;
  int shown = ble_deviceCount > 0 ? min(ble_deviceCount, min(BLE_MAX_LIST, BLE_VISIBLE_ROWS)) : 0;
  for (int i = 0; i < shown; i++) {
    int yy = top + i * rowH;
    if (x > 15 && x < 465 && y > yy && y < yy + rowH - 4) {
      ble_status = "Connecting...";
      set_drawBluetoothPage();
      set_tryConnectBle(i);
      set_drawBluetoothPage();
      return;
    }
  }

  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}



static long set_manualOffsetSec = 0; // used only when Wi-Fi isn't connected

void set_syncTime() {
  if (WiFi.status() == WL_CONNECTED) {
    configTime(set_gmtOffsetSec, set_daylightOffsetSec, set_ntpServer);
  }
}

void set_drawDateTimePage() {
  set_drawSubHeader("Date & Time", C_CORAL);

  struct tm timeinfo;
  bool haveTime = getLocalTime(&timeinfo, 200);

  char dateBuf[32];
  char timeBuf[32];

  if (haveTime) {
    strftime(dateBuf, sizeof(dateBuf), "%A, %d %B %Y", &timeinfo);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", &timeinfo);
  } else {
    time_t fallback = (millis() / 1000) + set_manualOffsetSec;
    struct tm* mt = gmtime(&fallback);
    strftime(dateBuf, sizeof(dateBuf), "%A, %d %B %Y", mt);
    strftime(timeBuf, sizeof(timeBuf), "%H:%M:%S", mt);
  }

  tft.setTextColor(C_CORAL, C_BG);
  tft.setTextSize(1);
  tft.drawCentreString(timeBuf, 240, 90, 7);

  tft.setTextColor(C_TEXT, C_BG);
  tft.drawCentreString(dateBuf, 240, 150, 2);

  if (WiFi.status() == WL_CONNECTED) {
    tft.setTextColor(C_MINT, C_BG);
    tft.drawCentreString(haveTime ? "Synced via NTP" : "Syncing...", 240, 180, 2);
  } else {
    tft.setTextColor(C_GOLD, C_BG);
    tft.drawCentreString("Connect Wi-Fi to auto-sync", 240, 180, 2);
  }

  tft.fillRoundRect(150, 210, 180, 40, 10, C_TEAL);
  tft.setTextColor(C_TEXT, C_TEAL);
  tft.drawCentreString("Sync Now", 240, 222, 2);

  set_drawBackButton();
}

void set_handleDateTimeTouch(uint16_t x, uint16_t y) {
  if (x > 150 && x < 330 && y > 210 && y < 250) {
    set_syncTime();
    set_drawDateTimePage();
    return;
  }
  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

// ================= BATTERY PAGE (stub - no fuel gauge wired yet) =================

void set_drawBatteryPage() {
  set_drawSubHeader("Battery", C_MINT);
  set_drawBattery(240, 110, C_MINT);
  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawCentreString("No battery monitor wired up yet.", 240, 160, 2);
  tft.drawCentreString("Connect a fuel-gauge IC (e.g. MAX17048)", 240, 185, 1);
  tft.drawCentreString("and read it here to show real %.", 240, 200, 1);
  set_drawBackButton();
}

void set_handleBatteryTouch(uint16_t x, uint16_t y) {
  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

// APPS PAGE

struct AppInfo { const char* name; const char* version; const char* size; const char* status; };
static AppInfo set_apps[] = {
  {"Settings",     "1.0.0", "128 KB", "Running"},
  {"File Manager", "0.9.2", "64 KB",  "Stopped"},
  {"Clock",        "1.1.0", "32 KB",  "Stopped"},
  {"Music Player", "0.5.0", "96 KB",  "Stopped"},
};
static const int set_appsCount = sizeof(set_apps) / sizeof(set_apps[0]);
static int set_appInfoOpen = -1; // index of app whose info popup is showing, -1 = none

void set_drawAppsPage() {
  set_drawSubHeader("Apps", C_SKY);

  int rowH = 44, top = 60;
  for (int i = 0; i < set_appsCount; i++) {
    int y = top + i * rowH;
    tft.fillRoundRect(15, y, 450, rowH - 6, 8, C_CARD);
    set_drawApps(35, y + 17, C_SKY);
    tft.setTextColor(C_TEXT, C_CARD);
    tft.drawString(set_apps[i].name, 60, y + 12, 2);
    tft.setTextColor(C_SUBTEXT, C_CARD);
    tft.drawString("i", 430, y + 12, 2);
    tft.drawCircle(438, y + 19, 12, C_SKY);
  }

  set_drawBackButton();
  if (set_appInfoOpen >= 0) set_drawAppInfoPopup(set_appInfoOpen);
}

void set_drawAppInfoPopup(int idx) {
  tft.fillRoundRect(90, 70, 300, 170, 14, C_CARD_TAP);
  tft.drawRoundRect(90, 70, 300, 170, 14, C_SKY);

  tft.setTextColor(C_TEXT, C_CARD_TAP);
  tft.setTextSize(2);
  tft.drawCentreString(set_apps[idx].name, 240, 85, 2);

  tft.setTextSize(1);
  tft.setTextColor(C_SUBTEXT, C_CARD_TAP);
  tft.drawString("Version:", 115, 120, 2);
  tft.setTextColor(C_TEXT, C_CARD_TAP);
  tft.drawString(set_apps[idx].version, 230, 120, 2);

  tft.setTextColor(C_SUBTEXT, C_CARD_TAP);
  tft.drawString("Size:", 115, 145, 2);
  tft.setTextColor(C_TEXT, C_CARD_TAP);
  tft.drawString(set_apps[idx].size, 230, 145, 2);

  tft.setTextColor(C_SUBTEXT, C_CARD_TAP);
  tft.drawString("Status:", 115, 170, 2);
  tft.setTextColor(String(set_apps[idx].status) == "Running" ? C_MINT : C_GOLD, C_CARD_TAP);
  tft.drawString(set_apps[idx].status, 230, 170, 2);

  tft.fillRoundRect(190, 205, 100, 26, 8, C_ROSE);
  tft.setTextColor(C_TEXT, C_ROSE);
  tft.drawCentreString("Close", 240, 211, 1);
}

void set_handleAppsTouch(uint16_t x, uint16_t y) {
  if (set_appInfoOpen >= 0) {
    if (x > 190 && x < 290 && y > 205 && y < 231) {
      set_appInfoOpen = -1;
      set_drawAppsPage();
    }
    return; // swallow other taps while popup is open
  }

  int rowH = 44, top = 60;
  for (int i = 0; i < set_appsCount; i++) {
    int yy = top + i * rowH;
    if (x > 15 && x < 465 && y > yy && y < yy + rowH - 6) {
      set_appInfoOpen = i;
      set_drawAppsPage();
      return;
    }
  }

  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

//  SECURITY PAGE (stub) 

void set_drawSecurityPage() {
  set_drawSubHeader("Security", C_ROSE);
  set_drawLock(240, 110, C_ROSE);
  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.drawCentreString("Screen lock / PIN not implemented yet.", 240, 160, 2);
  set_drawBackButton();
}

void set_handleSecurityTouch(uint16_t x, uint16_t y) {
  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

// ABOUT PAGE (real ESP32-S3 specs)

void set_drawAboutPage() {
  set_drawSubHeader("About", C_GOLD);

  int y = 58;
  int lh = 24;

  auto row = [&](const char* label, String value, uint16_t vc) {
    tft.setTextColor(C_SUBTEXT, C_BG);
    tft.drawString(label, 20, y, 1);
    tft.setTextColor(vc, C_BG);
    tft.drawString(value, 260, y, 1);
    y += lh;
  };

  row("Device",       "MiniOS on ESP32-S3", C_GOLD);
  row("Chip model",   String(ESP.getChipModel()), C_TEXT);
  row("Chip revision",String(ESP.getChipRevision()), C_TEXT);
  row("CPU cores",    String(ESP.getChipCores()), C_TEXT);
  row("CPU frequency",String(ESP.getCpuFreqMHz()) + " MHz", C_TEXT);
  row("Flash size",   String(ESP.getFlashChipSize() / (1024 * 1024)) + " MB", C_TEXT);

  bool hasPsram = psramFound();
  if (hasPsram) {
    row("PSRAM size",   String(ESP.getPsramSize() / (1024 * 1024)) + " MB", C_MINT);
    row("PSRAM free",   String(ESP.getFreePsram() / 1024) + " KB", C_MINT);
  } else {
    row("PSRAM", "Not detected - enable it in", C_ROSE);
    row("", "Tools > PSRAM in the Arduino IDE", C_ROSE);
  }

  row("Free heap",    String(ESP.getFreeHeap() / 1024) + " KB", C_TEXT);
  row("MiniOS version","1.0.0", C_TEXT);

  tft.setTextColor(C_SUBTEXT, C_BG);
  tft.setTextSize(1);
  tft.drawString("Like a phone, MiniOS keeps its UI buffers and", 20, y + 6, 1);
  tft.drawString("scan/connect data in PSRAM so the small internal", 20, y + 18, 1);
  tft.drawString("SRAM doesn't fill up while apps are running.", 20, y + 30, 1);

  set_drawBackButton();
}

void set_handleAboutTouch(uint16_t x, uint16_t y) {
  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

void set_handleSystemTouch(uint16_t x, uint16_t y) {
  if (set_isBackTap(x, y)) { set_goMenu(); return; }
}

// ================= MENU TOUCH =================

void set_handleMenuTouch(uint16_t x, uint16_t y) {
  int dy = y - set_lastY;

  if (abs(dy) > 2) {
    set_scrollOffset += dy;
    int maxScroll = set_totalItems * 60 - 260;
    if (maxScroll < 0) maxScroll = 0;
    if (set_scrollOffset < 0) set_scrollOffset = 0;
    if (set_scrollOffset > maxScroll) set_scrollOffset = maxScroll;
    set_drawMenu();
    set_lastY = y;
  }

  for (int i = 0; i < set_totalItems; i++) {
    int yy = 60 + (i * 60) - set_scrollOffset;
    if (x > 10 && x < 470 && y > yy && y < yy + 50) {
      set_beep();
      Serial.print("Selected: ");
      Serial.println(set_menu[i].name);

      switch (i) {
        case 0: set_currentPage = PAGE_SYSTEM;     set_drawSystemPage();     break;
        case 1: set_currentPage = PAGE_DISPLAY;    set_drawDisplayPage();    break;
        case 2: set_currentPage = PAGE_SOUND;      set_drawSoundPage();      break;
        case 3: set_currentPage = PAGE_WIFI;       set_startWifiScan(); set_drawWifiPage(); break;
        case 4: set_currentPage = PAGE_BLUETOOTH;  set_startBleScan(); set_drawBluetoothPage(); break;
        case 5: set_currentPage = PAGE_DATETIME;   set_syncTime(); set_drawDateTimePage(); break;
        case 6: set_currentPage = PAGE_BATTERY;    set_drawBatteryPage();    break;
        case 7: set_currentPage = PAGE_APPS;       set_appInfoOpen = -1; set_drawAppsPage(); break;
        case 8: set_currentPage = PAGE_SECURITY;   set_drawSecurityPage();   break;
        case 9: set_currentPage = PAGE_ABOUT;      set_drawAboutPage();      break;
      }
      delay(200);
      return;
    }
  }
}

//  TOUCH DISPATCH 

void set_handleTouch() {
  uint16_t x, y;

  if (tft.getTouch(&x, &y)) {
    if (!set_touching) {
      set_touching = true;
      set_lastY = y;
      set_beep();

      if (set_currentPage == PAGE_MENU) {
        // top-left back button exits the whole app
        if (x > 4 && x < 38 && y > 8 && y < 42) {
          set_close();
          return;
        }
      } else {
        switch (set_currentPage) {
          case PAGE_SYSTEM:    set_handleSystemTouch(x, y); break;
          case PAGE_DISPLAY:   set_handleDisplayTouch(x, y); break;
          case PAGE_SOUND:     set_handleSoundTouch(x, y); break;
          case PAGE_WIFI:      set_handleWifiTouch(x, y); break;
          case PAGE_BLUETOOTH: set_handleBluetoothTouch(x, y); break;
          case PAGE_DATETIME:  set_handleDateTimeTouch(x, y); break;
          case PAGE_BATTERY:   set_handleBatteryTouch(x, y); break;
          case PAGE_APPS:      set_handleAppsTouch(x, y); break;
          case PAGE_SECURITY:  set_handleSecurityTouch(x, y); break;
          case PAGE_ABOUT:     set_handleAboutTouch(x, y); break;
          default: break;
        }
        return;
      }
      return;
    }

    // Drag handling (menu scroll) only applies on the menu page
    if (set_currentPage == PAGE_MENU) {
      set_handleMenuTouch(x, y);
    }
  } else {
    set_touching = false;
  }
}

// Public API used by main.ino / actions.c 

void set_setup() {

}

void set_request_open() {
  set_open_requested = true;
}

int set_consume_open_request() {
  if (set_open_requested) {
    set_open_requested = false;
    return 1;
  }
  return 0;
}

void set_open() {
  set_active = true;
  set_currentPage = PAGE_MENU;
  set_scrollOffset = 0;
  set_drawMenu();
}

void set_close() {
  set_active = false;

  lv_obj_invalidate(objects.main);
}

void set_loop() {
  if (!set_active) return;

  if (set_currentPage == PAGE_WIFI) set_pollWifi();

  // Live-update the clock roughly once a second while that page is open.
  static unsigned long lastClockTick = 0;
  if (set_currentPage == PAGE_DATETIME && millis() - lastClockTick > 1000) {
    lastClockTick = millis();
    set_drawDateTimePage();
  }

  set_handleTouch();
}

int set_is_active() {
  return set_active ? 1 : 0;
}
