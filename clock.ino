/*
 * Time Tools app: one icon opens a small menu (Clock / Stopwatch).
 * Each sub-screen has its own back button (returns to this menu);
 * the menu's back button exits the whole app back to the LVGL main menu.
 *
 * Uses the ONE shared touch calibration set up once in main.ino via
 * touch_calibration_init() - no per-app calibration data here.
 */

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include <math.h>
#include "ui.h"      // for objects.main
#include <lvgl.h>    // for lv_obj_invalidate

extern TFT_eSPI tft;  // shared instance from main.ino - do NOT re-init here

extern "C" {
    void time_setup();
    void time_open();
    void time_loop();
    int  time_is_active();
    void time_request_open();
    int  time_consume_open_request();
}

#define TT_DEBUG 1
#if TT_DEBUG
  #define TT_LOG(...) Serial.printf(__VA_ARGS__)
#else
  #define TT_LOG(...)
#endif

// ── Screen constants ─────────────────────────────────────────
#define TT_SCR_W 480
#define TT_SCR_H 320

#define TT_C_BG      TFT_BLACK
#define TT_C_HEADER  0x03EF   // teal, matches notes.ino's header colour
#define TT_C_ACCENT  0x07FF   // cyan
#define TT_C_TEXT    TFT_WHITE

// ── App / navigation state ─────────────────────────────────────
enum TtScreen { TT_SCR_MENU, TT_SCR_CLOCK, TT_SCR_STOPWATCH };
static TtScreen tt_currentScreen = TT_SCR_MENU;
static bool tt_active         = false;
static bool tt_open_requested = false;

// ── Shared touch helpers (same debounce pattern as notes.ino) ──
static int  tt_touchX = 0, tt_touchY = 0;
static bool tt_touchValid = false;

static void tt_updateTouch() {
  uint16_t x, y;
  tt_touchValid = tft.getTouch(&x, &y, 600);
  if (tt_touchValid) { tt_touchX = x; tt_touchY = y; }
}

static bool tt_touchIn(int x, int y, int w, int h) {
  return tt_touchValid && tt_touchX >= x && tt_touchX < x + w &&
         tt_touchY >= y && tt_touchY < y + h;
}

// ── Shared header (same visual language as notes.ino) ──────────
static void tt_drawHeader(const String& title) {
  tft.fillRect(0, 0, TT_SCR_W, 40, TT_C_HEADER);
  tft.setTextColor(TT_C_TEXT, TT_C_HEADER);
  tft.setTextSize(2);
  tft.setCursor(50, 12);
  tft.print(title);
  tft.fillRoundRect(4, 6, 38, 28, 5, TT_C_BG);
  tft.setTextColor(TT_C_ACCENT, TT_C_BG);
  tft.setCursor(8, 12);
  tft.print("< ");
  tft.drawFastHLine(0, 40, TT_SCR_W, TT_C_ACCENT);
}

static bool tt_backTapped() {
  return tt_touchIn(4, 6, 38, 28);
}

// ════════════════════════════════════════════════════════════════
// MENU SCREEN
// ════════════════════════════════════════════════════════════════
#define TT_MENU_BTN_W 220
#define TT_MENU_BTN_H 80
#define TT_MENU_BTN_X ((TT_SCR_W - TT_MENU_BTN_W) / 2)
#define TT_CLOCK_BTN_Y 90
#define TT_STOPWATCH_BTN_Y 190

void time_close();  // fwd decl

static void tt_drawMenu() {
  tft.fillScreen(TT_C_BG);
  tt_drawHeader("Time Tools");

  tft.fillRoundRect(TT_MENU_BTN_X, TT_CLOCK_BTN_Y, TT_MENU_BTN_W, TT_MENU_BTN_H, 10, TFT_DARKCYAN);
  tft.setTextColor(TT_C_TEXT, TFT_DARKCYAN);
  tft.drawCentreString("Clock", TT_SCR_W / 2, TT_CLOCK_BTN_Y + 30, 4);

  tft.fillRoundRect(TT_MENU_BTN_X, TT_STOPWATCH_BTN_Y, TT_MENU_BTN_W, TT_MENU_BTN_H, 10, TFT_NAVY);
  tft.setTextColor(TT_C_TEXT, TFT_NAVY);
  tft.drawCentreString("Stopwatch", TT_SCR_W / 2, TT_STOPWATCH_BTN_Y + 30, 4);
}

static void tt_drawClockScreen();
static void tt_drawStopwatchScreen();

static void tt_handleMenuTouch() {
  if (tt_backTapped()) {
    time_close();   // this is the app's top-level screen - back = exit app
    return;
  }
  if (tt_touchIn(TT_MENU_BTN_X, TT_CLOCK_BTN_Y, TT_MENU_BTN_W, TT_MENU_BTN_H)) {
    tt_currentScreen = TT_SCR_CLOCK;
    tt_drawClockScreen();
    return;
  }
  if (tt_touchIn(TT_MENU_BTN_X, TT_STOPWATCH_BTN_Y, TT_MENU_BTN_W, TT_MENU_BTN_H)) {
    tt_currentScreen = TT_SCR_STOPWATCH;
    tt_drawStopwatchScreen();
    return;
  }
}

// ════════════════════════════════════════════════════════════════
// CLOCK SCREEN
// ════════════════════════════════════════════════════════════════

// ---- Fill in your real WiFi credentials here ----
static const char* CLK_WIFI_SSID = "your_wifi_name";
static const char* CLK_WIFI_PASS = "your_wifi_password";

static const char* CLK_NTP_SERVER = "pool.ntp.org";
static const long  CLK_GMT_OFFSET_SEC = 19800;  // India (UTC+5:30)
static const int   CLK_DST_OFFSET_SEC = 0;

#define CLK_WIFI_TIMEOUT_MS 8000

static const int clk_cx = TT_SCR_W / 2;   // 240
static const int clk_cy = 150;
static const int clk_radius = 80;

enum ClkState { CLK_ST_CONNECTING, CLK_ST_RUNNING, CLK_ST_ERROR };
static ClkState clk_state = CLK_ST_CONNECTING;
static unsigned long clk_wifiStartedAt = 0;
static unsigned long clk_lastTick = 0;
static bool clk_timeConfigured = false;

static void clk_drawFace() {
  tft.fillRect(0, 41, TT_SCR_W, TT_SCR_H - 41, TT_C_BG);
  tft.drawCircle(clk_cx, clk_cy, clk_radius, TFT_WHITE);
  for (int i = 0; i < 60; i++) {
    float angle = (i * 6 - 90) * DEG_TO_RAD;
    int x1 = clk_cx + cos(angle) * (clk_radius - 5);
    int y1 = clk_cy + sin(angle) * (clk_radius - 5);
    int len = (i % 5 == 0) ? 14 : 6;
    int x2 = clk_cx + cos(angle) * (clk_radius - len);
    int y2 = clk_cy + sin(angle) * (clk_radius - len);
    tft.drawLine(x1, y1, x2, y2, TFT_WHITE);
  }
  for (int n = 1; n <= 12; n++) {
    float angle = (n * 30 - 90) * DEG_TO_RAD;
    int x = clk_cx + cos(angle) * (clk_radius - 24);
    int y = clk_cy + sin(angle) * (clk_radius - 24);
    tft.setTextColor(TFT_WHITE, TT_C_BG);
    tft.drawCentreString(String(n), x, y - 8, 2);
  }
}

static void clk_drawHands(int hh, int mm, int ss) {
  float secAngle  = (ss * 6 - 90) * DEG_TO_RAD;
  float minAngle  = ((mm + ss / 60.0) * 6 - 90) * DEG_TO_RAD;
  float hourAngle = ((hh % 12 + mm / 60.0) * 30 - 90) * DEG_TO_RAD;
  int sx = clk_cx + cos(secAngle)  * (clk_radius - 18);
  int sy = clk_cy + sin(secAngle)  * (clk_radius - 18);
  int mx = clk_cx + cos(minAngle)  * (clk_radius - 30);
  int my = clk_cy + sin(minAngle)  * (clk_radius - 30);
  int hx = clk_cx + cos(hourAngle) * (clk_radius - 45);
  int hy = clk_cy + sin(hourAngle) * (clk_radius - 45);
  tft.drawLine(clk_cx, clk_cy, hx, hy, TFT_WHITE);
  tft.drawLine(clk_cx, clk_cy, mx, my, TFT_GREEN);
  tft.drawLine(clk_cx, clk_cy, sx, sy, TFT_RED);
  tft.fillCircle(clk_cx, clk_cy, 4, TFT_WHITE);
}

static void clk_drawStatus(const String& msg) {
  tft.fillRect(0, 41, TT_SCR_W, TT_SCR_H - 41, TT_C_BG);
  tft.setTextColor(TT_C_ACCENT, TT_C_BG);
  tft.setTextSize(2);
  tft.setCursor(20, 140);
  tft.print(msg);
}

static void tt_drawClockScreen() {
  tft.fillScreen(TT_C_BG);
  tt_drawHeader("Clock");

  clk_state = CLK_ST_CONNECTING;
  clk_timeConfigured = false;
  clk_lastTick = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(CLK_WIFI_SSID, CLK_WIFI_PASS);
  clk_wifiStartedAt = millis();
  clk_drawStatus("Connecting to WiFi...");
  TT_LOG("[clock] connecting to %s...\n", CLK_WIFI_SSID);
}

// Called every tt_loop() tick while on the Clock screen.
static void clk_tick() {
  if (clk_state == CLK_ST_CONNECTING) {
    if (WiFi.status() == WL_CONNECTED) {
      TT_LOG("[clock] WiFi connected\n");
      configTime(CLK_GMT_OFFSET_SEC, CLK_DST_OFFSET_SEC, CLK_NTP_SERVER);
      clk_timeConfigured = true;
      clk_state = CLK_ST_RUNNING;
      clk_drawFace();
      clk_lastTick = 0;   // force immediate first draw
    } else if (millis() - clk_wifiStartedAt > CLK_WIFI_TIMEOUT_MS) {
      TT_LOG("[clock] WiFi connect TIMED OUT\n");
      clk_state = CLK_ST_ERROR;
      clk_drawStatus("WiFi connect failed.\nCheck credentials, tap < to exit.");
    }
    return;
  }

  if (clk_state != CLK_ST_RUNNING) return;

  // Update once per second, non-blocking (no delay()).
  if (millis() - clk_lastTick < 1000) return;
  clk_lastTick = millis();

  struct tm timeinfo;
  if (!getLocalTime(&timeinfo, 100)) {
    TT_LOG("[clock] getLocalTime failed\n");
    return;
  }

  clk_drawFace();
  clk_drawHands(timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);

  char timeStr[20], dayStr[20], dateStr[30];
  strftime(timeStr, sizeof(timeStr), "%I:%M:%S %p", &timeinfo);
  strftime(dayStr,  sizeof(dayStr),  "%A", &timeinfo);
  strftime(dateStr, sizeof(dateStr), "%d %B %Y", &timeinfo);

  tft.setTextColor(TFT_CYAN, TT_C_BG);
  tft.drawCentreString(timeStr, TT_SCR_W / 2, 250, 4);
  tft.setTextColor(TFT_YELLOW, TT_C_BG);
  tft.drawCentreString(dayStr, TT_SCR_W / 2, 282, 2);
  tft.setTextColor(TFT_WHITE, TT_C_BG);
  tft.drawCentreString(dateStr, TT_SCR_W / 2, 300, 2);
}

static void clk_leave() {
  // Independent WiFi per app (per your setup) - disconnect cleanly when
  // leaving the Clock screen so it doesn't sit connected in the background.
  WiFi.disconnect(true, true);
}

// ════════════════════════════════════════════════════════════════
// STOPWATCH SCREEN
// ════════════════════════════════════════════════════════════════
#define SW_BTN_W 100
#define SW_BTN_H 50
#define SW_BTN_GAP 15
#define SW_BTN_Y 190
#define SW_BTN_TOTAL_W (4 * SW_BTN_W + 3 * SW_BTN_GAP)
#define SW_BTN_START_X ((TT_SCR_W - SW_BTN_TOTAL_W) / 2)

#define SW_START_X (SW_BTN_START_X)
#define SW_STOP_X  (SW_BTN_START_X + (SW_BTN_W + SW_BTN_GAP))
#define SW_LAP_X   (SW_BTN_START_X + 2 * (SW_BTN_W + SW_BTN_GAP))
#define SW_RESET_X (SW_BTN_START_X + 3 * (SW_BTN_W + SW_BTN_GAP))

#define SW_MAX_LAPS 5
#define SW_LAPS_Y 250
#define SW_LAP_LINE_H 14

static bool sw_running = false;
static unsigned long sw_startMillis = 0;
static unsigned long sw_elapsedMillis = 0;
static String sw_lapTimes[SW_MAX_LAPS];
static int sw_lapCount = 0;
static String sw_lastTimeStr = "";

static String sw_formatTime(unsigned long ms) {
  unsigned long totalSeconds = ms / 1000;
  int hours = totalSeconds / 3600;
  int minutes = (totalSeconds % 3600) / 60;
  int seconds = totalSeconds % 60;
  char buf[20];
  sprintf(buf, "%02d:%02d:%02d", hours, minutes, seconds);
  return String(buf);
}

static unsigned long sw_currentElapsed() {
  return sw_running ? (sw_elapsedMillis + (millis() - sw_startMillis)) : sw_elapsedMillis;
}

static void sw_drawLaps() {
  tft.fillRect(0, SW_LAPS_Y, TT_SCR_W, TT_SCR_H - SW_LAPS_Y, TT_C_BG);
  int y = SW_LAPS_Y;
  for (int i = sw_lapCount - 1; i >= 0; i--) {
    tft.setTextColor(TT_C_TEXT, TT_C_BG);
    tft.drawString("Lap " + String(i + 1) + ": " + sw_lapTimes[i], 20, y, 2);
    y += SW_LAP_LINE_H;
  }
}

static void sw_addLap() {
  String lap = sw_formatTime(sw_currentElapsed());
  if (sw_lapCount < SW_MAX_LAPS) {
    sw_lapTimes[sw_lapCount++] = lap;
  } else {
    for (int i = 0; i < SW_MAX_LAPS - 1; i++) sw_lapTimes[i] = sw_lapTimes[i + 1];
    sw_lapTimes[SW_MAX_LAPS - 1] = lap;
  }
  sw_drawLaps();
}

static void sw_drawButtons() {
  tft.fillRoundRect(SW_START_X, SW_BTN_Y, SW_BTN_W, SW_BTN_H, 8, TFT_GREEN);
  tft.setTextColor(TFT_BLACK, TFT_GREEN);
  tft.drawCentreString("START", SW_START_X + SW_BTN_W / 2, SW_BTN_Y + 15, 2);

  tft.fillRoundRect(SW_STOP_X, SW_BTN_Y, SW_BTN_W, SW_BTN_H, 8, TFT_RED);
  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.drawCentreString("STOP", SW_STOP_X + SW_BTN_W / 2, SW_BTN_Y + 15, 2);

  tft.fillRoundRect(SW_LAP_X, SW_BTN_Y, SW_BTN_W, SW_BTN_H, 8, TFT_ORANGE);
  tft.setTextColor(TFT_BLACK, TFT_ORANGE);
  tft.drawCentreString("LAP", SW_LAP_X + SW_BTN_W / 2, SW_BTN_Y + 15, 2);

  tft.fillRoundRect(SW_RESET_X, SW_BTN_Y, SW_BTN_W, SW_BTN_H, 8, TFT_BLUE);
  tft.setTextColor(TFT_WHITE, TFT_BLUE);
  tft.drawCentreString("RESET", SW_RESET_X + SW_BTN_W / 2, SW_BTN_Y + 15, 2);
}

static void sw_drawTimeDisplay(bool force = false) {
  String timeStr = sw_formatTime(sw_currentElapsed());
  if (!force && timeStr == sw_lastTimeStr) return;
  tft.fillRect(120, 54, 240, 60, TT_C_BG);
  tft.setTextColor(TFT_CYAN, TT_C_BG);
  tft.drawCentreString(timeStr, TT_SCR_W / 2, 69, 4);
  sw_lastTimeStr = timeStr;
}

static void tt_drawStopwatchScreen() {
  tft.fillScreen(TT_C_BG);
  tt_drawHeader("Stopwatch");
  tft.drawRoundRect(120, 54, 240, 60, 6, TFT_WHITE);
  sw_lastTimeStr = "";
  sw_drawTimeDisplay(true);
  sw_drawButtons();
  sw_drawLaps();
}

static void tt_handleStopwatchTouch() {
  if (tt_backTapped()) {
    tt_currentScreen = TT_SCR_MENU;
    tt_drawMenu();
    return;
  }
  if (tt_touchIn(SW_START_X, SW_BTN_Y, SW_BTN_W, SW_BTN_H)) {
    if (!sw_running) { sw_startMillis = millis(); sw_running = true; }
    return;
  }
  if (tt_touchIn(SW_STOP_X, SW_BTN_Y, SW_BTN_W, SW_BTN_H)) {
    if (sw_running) { sw_elapsedMillis += millis() - sw_startMillis; sw_running = false; }
    return;
  }
  if (tt_touchIn(SW_LAP_X, SW_BTN_Y, SW_BTN_W, SW_BTN_H)) {
    if (sw_running) sw_addLap();
    return;
  }
  if (tt_touchIn(SW_RESET_X, SW_BTN_Y, SW_BTN_W, SW_BTN_H)) {
    sw_running = false;
    sw_startMillis = 0;
    sw_elapsedMillis = 0;
    sw_lapCount = 0;
    for (int i = 0; i < SW_MAX_LAPS; i++) sw_lapTimes[i] = "";
    sw_drawTimeDisplay(true);
    sw_drawLaps();
    return;
  }
}

// ════════════════════════════════════════════════════════════════
// Public API used by main.ino / actions.c
// ════════════════════════════════════════════════════════════════

void time_setup() {
  // Nothing to allocate up front - both sub-apps init their own state
  // when opened. Kept for symmetry with the other apps' setup pattern.
  TT_LOG("[time] setup: OK\n");
}

void time_request_open() {
  tt_open_requested = true;
}

int time_consume_open_request() {
  if (tt_open_requested) {
    tt_open_requested = false;
    return 1;
  }
  return 0;
}

void time_open() {
  tt_active = true;
  tt_currentScreen = TT_SCR_MENU;
  tt_drawMenu();
}

void time_close() {
  tt_active = false;
  if (tt_currentScreen == TT_SCR_CLOCK) clk_leave();
  lv_obj_invalidate(objects.main);
}

void time_loop() {
  if (!tt_active) return;

  tt_updateTouch();
  static bool wasTouched = false;
  static unsigned long lastTouch = 0;
  bool tapped = false;

  if (tt_touchValid && !wasTouched && (millis() - lastTouch > 200)) {
    wasTouched = true;
    lastTouch  = millis();
    tapped = true;
  }
  if (!tt_touchValid) wasTouched = false;

  switch (tt_currentScreen) {
    case TT_SCR_MENU:
      if (tapped) tt_handleMenuTouch();
      break;

    case TT_SCR_CLOCK:
      if (tapped && tt_backTapped()) {
        clk_leave();
        tt_currentScreen = TT_SCR_MENU;
        tt_drawMenu();
        break;
      }
      clk_tick();   // non-blocking, ticks itself every ~1s
      break;

    case TT_SCR_STOPWATCH:
      if (tapped) tt_handleStopwatchTouch();
      // Keep the running display live even without a touch this tick.
      if (tt_currentScreen == TT_SCR_STOPWATCH) sw_drawTimeDisplay();
      break;
  }
}

int time_is_active() {
  return tt_active ? 1 : 0;
}
