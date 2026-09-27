
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "ui.h"      // for objects.main
#include <lvgl.h>    // for lv_obj_invalidate

extern TFT_eSPI tft;  // the single tft instance created in main.ino

// These are called from actions.c (plain C) and main.ino (C++).
extern "C" {
    void chatbot_setup();
    void chatbot_open();
    void chatbot_loop();
    int  chatbot_is_active();
    void chatbot_request_open();
    int  chatbot_consume_open_request();
}

// ── WiFi / Gemini credentials ───────────────────────────────
const char* CB_WIFI_SSID     = "wifi_name";
const char* CB_WIFI_PASSWORD = "wifi_password";
const char* CB_GEMINI_API_KEY  = "your_API";
const char* CB_GEMINI_URL_BASE =
  "https://generativelanguage.googleapis.com/v1beta/models/"
  "gemini-2.5-flash:generateContent?key=";

// ── Colour palette ───────────────────────────────────────────
#define CB_C_BG          0x0841
#define CB_C_SURFACE     0x18C3
#define CB_C_HEADER_BG   0x0329
#define CB_C_ACCENT      0x07FF
#define CB_C_ACCENT_DIM  0x03EF
#define CB_C_TEXT        0xFFFF
#define CB_C_SUBTEXT     0x8C71
#define CB_C_DIVIDER     0x2965
#define CB_C_USER_BUB    0x034B
#define CB_C_AI_BUB      0x1228
#define CB_C_AI_BORDER   0x4A6F
#define CB_C_USER_BORDER 0x05BF
#define CB_C_KEY         0x2124
#define CB_C_KEY_PRESS   0x05DF
#define CB_C_BKSP        0x7000
#define CB_C_ENTER       0x0248
#define CB_C_CAPS_ON     0x07E0
#define CB_C_WIFI_OK     0x07E0
#define CB_C_WIFI_FAIL   0xF800
#define CB_C_THINKING    0xFFE0
#define CB_C_INPUT_BOR   0x07BF

// ── Layout (480 x 320 landscape) ─────────────────────────────
#define CB_SCR_W       480
#define CB_SCR_H       320
#define CB_HEADER_H     36
#define CB_BACK_W       32   // back button width (new)
#define CB_BACK_H       28   // back button height (new)

#define CB_KEY_W        42
#define CB_KEY_H        34
#define CB_KEY_GAP       3

#define CB_KB_TOTAL_H  ((CB_KEY_H + CB_KEY_GAP) * 4 + CB_KEY_GAP)

#define CB_INPUT_H      30
#define CB_INPUT_Y     (CB_SCR_H - CB_KB_TOTAL_H - CB_INPUT_H - 2)

#define CB_CHAT_Y      (CB_HEADER_H + 1)
#define CB_CHAT_H      (CB_INPUT_Y - CB_CHAT_Y - 1)

#define CB_KB_Y        (CB_INPUT_Y + CB_INPUT_H + 2)

// ── Keyboard rows ─────────────────────────────────────────────
static const char* CB_KB_ROW0 = "QWERTYUIOP";
static const char* CB_KB_ROW1 = "ASDFGHJKL";
static const char* CB_KB_ROW2 = "ZXCVBNM";
static const char* CB_PUNCTS  = ".,!?\"'/:;@#-+()";

#define CB_KEY_BACKSPACE  8
#define CB_KEY_ENTER     10
#define CB_KEY_CAPS       1
#define CB_KEY_PUNCT      4
#define CB_KEY_NONE       0


#define CB_MSG_MAX_LINES  12
#define CB_LINE_CHARS     50
#define CB_MAX_MESSAGES   10
#define CB_MAX_INPUT_LEN  250

struct CbChatMsg {
  char  lines[CB_MSG_MAX_LINES][CB_LINE_CHARS + 1];
  int   lineCount;
  bool  isUser;
};

static CbChatMsg* cb_chatLog = nullptr;   // allocated on the heap in chatbot_setup()
static int        cb_chatCount  = 0;
static int        cb_chatScroll = 0;

// ── App state ──────────────────────────────────────────────────
enum CbAppState { CB_STATE_CHAT, CB_STATE_THINKING };
static CbAppState cb_appState = CB_STATE_CHAT;

static bool   cb_capsLock  = false;
static String cb_inputBuf  = "";

#define CB_DEBOUNCE_MS   155
#define CB_WIFI_CHECK_MS 5000


static bool chatbot_active = false;
static bool chatbot_open_requested = false;


int  cb_touchX = 0;
int  cb_touchY = 0;
bool cb_touchValid = false;

void cb_updateTouch() {
  uint16_t tx, ty;
  cb_touchValid = tft.getTouch(&tx, &ty, 600);
  if (cb_touchValid) {
    cb_touchX = (int)tx;
    cb_touchY = (int)ty;
  }
}

bool cb_inRect(int x, int y, int w, int h) {
  return cb_touchValid && cb_touchX >= x && cb_touchX < x + w && cb_touchY >= y && cb_touchY < y + h;
}

// ── Text wrap utility ────────────────────────────────────────
int cb_wrapIntoLines(const String& text, int maxChars,
                     char dst[][CB_LINE_CHARS + 1], int maxLines) {
  int lineIdx = 0;
  int len     = text.length();
  int i       = 0;

  while (i < len && lineIdx < maxLines) {
    if (lineIdx > 0 && i < len && text[i] == ' ') i++;

    int remaining = len - i;
    if (remaining <= 0) break;

    if (remaining <= maxChars) {
      String chunk = text.substring(i, i + remaining);
      chunk.toCharArray(dst[lineIdx], CB_LINE_CHARS + 1);
      lineIdx++;
      break;
    }

    int end = i + maxChars;
    int breakAt = end;
    while (breakAt > i && text[breakAt] != ' ') breakAt--;
    if (breakAt == i) breakAt = end;

    String chunk = text.substring(i, breakAt);
    chunk.toCharArray(dst[lineIdx], CB_LINE_CHARS + 1);
    lineIdx++;
    i = breakAt;
  }

  return lineIdx;
}

// ── Message store functions ───────────────────────────────────
void cb_addMessage(const String& text, bool isUser) {
  if (!cb_chatLog) return;   // safety: heap allocation failed or setup() not yet called

  if (cb_chatCount == CB_MAX_MESSAGES) {
    for (int i = 0; i < CB_MAX_MESSAGES - 1; i++) cb_chatLog[i] = cb_chatLog[i + 1];
    cb_chatCount = CB_MAX_MESSAGES - 1;
    if (cb_chatScroll > 0) cb_chatScroll--;
  }

  CbChatMsg& msg = cb_chatLog[cb_chatCount];
  msg.isUser    = isUser;
  msg.lineCount = cb_wrapIntoLines(text, CB_LINE_CHARS, msg.lines, CB_MSG_MAX_LINES);
  if (msg.lineCount < 1) msg.lineCount = 1;

  cb_chatCount++;

  const int LINE_H = 13;
  const int PAD_V  = 8;
  const int GAP_V  = 6;

  int usedH    = 0;
  int firstVis = cb_chatCount - 1;
  for (int i = cb_chatCount - 1; i >= 0; i--) {
    int bubH = cb_chatLog[i].lineCount * LINE_H + PAD_V * 2 + GAP_V;
    if (i + 1 < cb_chatCount && cb_chatLog[i].isUser != cb_chatLog[i+1].isUser)
      bubH += 4;
    usedH += bubH;
    if (usedH <= CB_CHAT_H) firstVis = i;
    else break;
  }
  cb_chatScroll = firstVis;
}

// ── Draw header (with new BACK button) ───────────────────────
void cb_drawHeader(bool wifiOk, bool thinking) {
  tft.fillRect(0, 0, CB_SCR_W, CB_HEADER_H, CB_C_HEADER_BG);

  // BACK button (new) — exits the whole ChatBot app back to LVGL main menu
  tft.fillRoundRect(4, 4, CB_BACK_W, CB_BACK_H, 5, CB_C_BG);
  tft.setTextColor(CB_C_ACCENT, CB_C_BG);
  tft.setTextSize(1);
  tft.setCursor(9, 14);
  tft.print("<");

  tft.setTextColor(CB_C_TEXT, CB_C_HEADER_BG);
  tft.setTextSize(2);
  tft.setCursor(4 + CB_BACK_W + 8, 10);
  tft.print("AI Assistant");

  tft.fillCircle(CB_SCR_W - 52, CB_HEADER_H / 2, 5, wifiOk ? CB_C_WIFI_OK : CB_C_WIFI_FAIL);

  tft.setTextSize(1);
  if (thinking) {
    tft.setTextColor(CB_C_THINKING, CB_C_HEADER_BG);
    tft.setCursor(CB_SCR_W - 102, 14);
    tft.print("Thinking...");
  } else {
    tft.setTextColor(wifiOk ? CB_C_WIFI_OK : CB_C_WIFI_FAIL, CB_C_HEADER_BG);
    tft.setCursor(CB_SCR_W - 44, 14);
    tft.print(wifiOk ? "WiFi" : "No WiFi");
  }
  tft.drawFastHLine(0, CB_HEADER_H, CB_SCR_W, CB_C_ACCENT);
}

// ── Draw input box ────────────────────────────────────────────
void cb_drawInputBox() {
  tft.fillRect(0, CB_INPUT_Y, CB_SCR_W, CB_INPUT_H + 2, CB_C_BG);
  tft.fillRoundRect(4, CB_INPUT_Y + 1, CB_SCR_W - 8, CB_INPUT_H, 5, CB_C_SURFACE);
  tft.drawRoundRect(4, CB_INPUT_Y + 1, CB_SCR_W - 8, CB_INPUT_H, 5, CB_C_INPUT_BOR);

  tft.setTextColor(CB_C_TEXT, CB_C_SURFACE);
  tft.setTextSize(1);

  int maxVisible = (CB_SCR_W - 20) / 6;
  String display = cb_inputBuf;
  if ((int)display.length() > maxVisible)
    display = display.substring(display.length() - maxVisible);

  tft.setCursor(10, CB_INPUT_Y + 10);
  tft.print(display);

  int cx = 10 + display.length() * 6;
  if (cx < CB_SCR_W - 8)
    tft.drawFastVLine(cx, CB_INPUT_Y + 5, CB_INPUT_H - 8, CB_C_ACCENT);
}

// ── Draw keyboard ──────────────────────────────────────────────
void cb_drawKey(int x, int y, int w, int h, const String& lbl, uint16_t bg = CB_C_KEY) {
  tft.fillRoundRect(x, y, w, h, 4, bg);
  tft.drawRoundRect(x, y, w, h, 4, CB_C_ACCENT_DIM);
  tft.setTextColor(CB_C_TEXT, bg);
  tft.setTextSize(1);
  int tx = x + (w - (int)lbl.length() * 6) / 2;
  int ty = y + (h - 8) / 2;
  tft.setCursor(tx, ty);
  tft.print(lbl);
}

void cb_drawKeyboard() {
  tft.fillRect(0, CB_KB_Y, CB_SCR_W, CB_SCR_H - CB_KB_Y, CB_C_BG);

  int r0w = 10 * (CB_KEY_W + CB_KEY_GAP) - CB_KEY_GAP;
  int r0x = (CB_SCR_W - r0w) / 2;
  int r0y = CB_KB_Y + CB_KEY_GAP;
  for (int i = 0; i < 10; i++) {
    char c = cb_capsLock ? CB_KB_ROW0[i] : (char)(CB_KB_ROW0[i] + 32);
    cb_drawKey(r0x + i * (CB_KEY_W + CB_KEY_GAP), r0y, CB_KEY_W, CB_KEY_H, String(c));
  }

  int r1w = 9 * (CB_KEY_W + CB_KEY_GAP) - CB_KEY_GAP;
  int r1x = (CB_SCR_W - r1w) / 2;
  int r1y = r0y + CB_KEY_H + CB_KEY_GAP;
  for (int i = 0; i < 9; i++) {
    char c = cb_capsLock ? CB_KB_ROW1[i] : (char)(CB_KB_ROW1[i] + 32);
    cb_drawKey(r1x + i * (CB_KEY_W + CB_KEY_GAP), r1y, CB_KEY_W, CB_KEY_H, String(c));
  }

  int r2y = r1y + CB_KEY_H + CB_KEY_GAP;
  int r2x = 52;
  cb_drawKey(4, r2y, 44, CB_KEY_H, "CAP", cb_capsLock ? CB_C_CAPS_ON : CB_C_KEY);
  for (int i = 0; i < 7; i++) {
    char c = cb_capsLock ? CB_KB_ROW2[i] : (char)(CB_KB_ROW2[i] + 32);
    cb_drawKey(r2x + i * (CB_KEY_W + CB_KEY_GAP), r2y, CB_KEY_W, CB_KEY_H, String(c));
  }
  int bkX = r2x + 7 * (CB_KEY_W + CB_KEY_GAP);
  cb_drawKey(bkX, r2y, CB_SCR_W - bkX - 4, CB_KEY_H, "<X", CB_C_BKSP);

  int r3y = r2y + CB_KEY_H + CB_KEY_GAP;
  cb_drawKey(4,   r3y, 44,  CB_KEY_H, ".,!?", CB_C_KEY);
  cb_drawKey(52,  r3y, 256, CB_KEY_H, "SPACE", CB_C_KEY);
  cb_drawKey(312, r3y, CB_SCR_W - 316, CB_KEY_H, "ENTER", CB_C_ENTER);
}

// ── Keyboard touch detection ───────────────────────────────────
char cb_detectKey() {
  int r0w = 10 * (CB_KEY_W + CB_KEY_GAP) - CB_KEY_GAP;
  int r0x = (CB_SCR_W - r0w) / 2;
  int r0y = CB_KB_Y + CB_KEY_GAP;
  if (cb_touchY >= r0y && cb_touchY < r0y + CB_KEY_H) {
    for (int i = 0; i < 10; i++) {
      int kx = r0x + i * (CB_KEY_W + CB_KEY_GAP);
      if (cb_touchX >= kx && cb_touchX < kx + CB_KEY_W)
        return cb_capsLock ? CB_KB_ROW0[i] : (char)(CB_KB_ROW0[i] + 32);
    }
  }

  int r1w = 9 * (CB_KEY_W + CB_KEY_GAP) - CB_KEY_GAP;
  int r1x = (CB_SCR_W - r1w) / 2;
  int r1y = r0y + CB_KEY_H + CB_KEY_GAP;
  if (cb_touchY >= r1y && cb_touchY < r1y + CB_KEY_H) {
    for (int i = 0; i < 9; i++) {
      int kx = r1x + i * (CB_KEY_W + CB_KEY_GAP);
      if (cb_touchX >= kx && cb_touchX < kx + CB_KEY_W)
        return cb_capsLock ? CB_KB_ROW1[i] : (char)(CB_KB_ROW1[i] + 32);
    }
  }

  int r2y = r1y + CB_KEY_H + CB_KEY_GAP;
  int r2x = 52;
  if (cb_touchY >= r2y && cb_touchY < r2y + CB_KEY_H) {
    if (cb_touchX >= 4 && cb_touchX < 48) return CB_KEY_CAPS;
    for (int i = 0; i < 7; i++) {
      int kx = r2x + i * (CB_KEY_W + CB_KEY_GAP);
      if (cb_touchX >= kx && cb_touchX < kx + CB_KEY_W)
        return cb_capsLock ? CB_KB_ROW2[i] : (char)(CB_KB_ROW2[i] + 32);
    }
    int bkX = r2x + 7 * (CB_KEY_W + CB_KEY_GAP);
    if (cb_touchX >= bkX) return CB_KEY_BACKSPACE;
  }

  int r3y = r2y + CB_KEY_H + CB_KEY_GAP;
  if (cb_touchY >= r3y && cb_touchY < r3y + CB_KEY_H) {
    if (cb_touchX >= 4   && cb_touchX < 48)  return CB_KEY_PUNCT;
    if (cb_touchX >= 52  && cb_touchX < 312) return ' ';
    if (cb_touchX >= 312)                    return CB_KEY_ENTER;
  }

  return CB_KEY_NONE;
}

// ── Punctuation picker ─────────────────────────────────────────
void cb_showPunctPicker() {
  int n   = strlen(CB_PUNCTS);
  int kw  = 18;
  int pw  = n * (kw + 2) + 6;
  int ph  = 36;
  int px  = (CB_SCR_W - pw) / 2;
  int py  = CB_INPUT_Y - ph - 2;

  tft.fillRoundRect(px, py, pw, ph, 6, CB_C_SURFACE);
  tft.drawRoundRect(px, py, pw, ph, 6, CB_C_ACCENT);
  for (int i = 0; i < n; i++) {
    int kx = px + 3 + i * (kw + 2);
    tft.fillRoundRect(kx, py + 4, kw, 28, 3, CB_C_KEY);
    tft.setTextColor(CB_C_TEXT, CB_C_KEY);
    tft.setTextSize(1);
    tft.setCursor(kx + 4, py + 12);
    tft.print(CB_PUNCTS[i]);
  }

  unsigned long t0 = millis();
  while (millis() - t0 < 4000) {
    cb_updateTouch();
    if (cb_touchValid && cb_touchY >= py && cb_touchY < py + ph &&
        cb_touchX >= px && cb_touchX < px + pw) {
      int idx = (cb_touchX - px - 3) / (kw + 2);
      if (idx >= 0 && idx < n && (int)cb_inputBuf.length() < CB_MAX_INPUT_LEN)
        cb_inputBuf += CB_PUNCTS[idx];
      delay(120);
      break;
    }
    delay(8);
  }

  tft.fillRect(px, py, pw, ph + 2, CB_C_BG);
  cb_drawInputBox();
}

// ── Draw chat area ─────────────────────────────────────────────
void cb_drawChatArea() {
  tft.fillRect(0, CB_CHAT_Y, CB_SCR_W, CB_CHAT_H, CB_C_BG);

  if (cb_chatCount == 0) {
    tft.setTextColor(CB_C_SUBTEXT, CB_C_BG);
    tft.setTextSize(1);
    tft.setCursor(130, CB_CHAT_Y + CB_CHAT_H / 2 - 8);
    tft.print("Type a message and press ENTER");
    tft.setCursor(154, CB_CHAT_Y + CB_CHAT_H / 2 + 6);
    tft.print("to chat with Gemini AI.");
    return;
  }

  const int LINE_H = 13;
  const int PAD_H  = 7;
  const int PAD_V  = 6;
  const int GAP_V  = 5;
  const int MARGIN = 5;
  const int MAX_W  = CB_SCR_W - MARGIN * 2 - 4;

  int yPos       = CB_CHAT_Y + 3;
  int clipBottom = CB_CHAT_Y + CB_CHAT_H - 3;

  for (int i = cb_chatScroll; i < cb_chatCount; i++) {
    if (yPos >= clipBottom) break;

    CbChatMsg& msg = cb_chatLog[i];
    int nLines     = msg.lineCount;

    int maxLen = 0;
    for (int l = 0; l < nLines; l++) {
      int ll = strlen(msg.lines[l]);
      if (ll > maxLen) maxLen = ll;
    }

    int bubW = maxLen * 6 + PAD_H * 2;
    if (bubW > MAX_W) bubW = MAX_W;
    if (bubW < 24)    bubW = 24;

    int bubH = nLines * LINE_H + PAD_V * 2;
    if (yPos + bubH > clipBottom) bubH = clipBottom - yPos;
    if (bubH < 1) break;

    int bubX;
    uint16_t bubBg, bubBorder;

    if (msg.isUser) {
      bubX      = CB_SCR_W - MARGIN - bubW;
      bubBg     = CB_C_USER_BUB;
      bubBorder = CB_C_USER_BORDER;
    } else {
      bubX      = MARGIN;
      bubBg     = CB_C_AI_BUB;
      bubBorder = CB_C_AI_BORDER;

      if (yPos + bubH < clipBottom) {
        tft.setTextColor(CB_C_ACCENT_DIM, CB_C_BG);
        tft.setTextSize(1);
        tft.setCursor(bubX + 2, yPos - 1);
        if (yPos > CB_CHAT_Y + 8) tft.print("AI");
      }
    }

    tft.fillRoundRect(bubX, yPos, bubW, bubH, 6, bubBg);
    tft.drawRoundRect(bubX, yPos, bubW, bubH, 6, bubBorder);

    tft.setTextColor(CB_C_TEXT, bubBg);
    tft.setTextSize(1);
    for (int l = 0; l < nLines; l++) {
      int lineY = yPos + PAD_V + l * LINE_H;
      if (lineY + 8 > clipBottom) break;
      tft.setCursor(bubX + PAD_H, lineY);
      tft.print(msg.lines[l]);
    }

    yPos += bubH + GAP_V;

    if (i + 1 < cb_chatCount && cb_chatLog[i].isUser != cb_chatLog[i + 1].isUser)
      yPos += 3;
  }

  if (cb_chatScroll > 0) {
    tft.setTextColor(CB_C_ACCENT, CB_C_BG);
    tft.setTextSize(1);
    tft.setCursor(CB_SCR_W / 2 - 3, CB_CHAT_Y + 1);
    tft.print("^");
  }

  if (yPos > clipBottom && cb_chatScroll + 1 < cb_chatCount) {
    tft.setTextColor(CB_C_ACCENT, CB_C_BG);
    tft.setTextSize(1);
    tft.setCursor(CB_SCR_W / 2 - 3, clipBottom - 9);
    tft.print("v");
  }
}

// ── Full screen redraw ─────────────────────────────────────────
void cb_drawFullScreen() {
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  cb_drawHeader(wifiOk, false);
  cb_drawChatArea();
  cb_drawInputBox();
  cb_drawKeyboard();
}

// ── Gemini API call (original logic, untouched) ─────────────────
String cb_askGemini(const String& prompt) {
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient https;
  String url = String(CB_GEMINI_URL_BASE) + String(CB_GEMINI_API_KEY);

  if (!https.begin(client, url)) {
    return "HTTPS connection failed.";
  }

  https.addHeader("Content-Type", "application/json");
  https.setTimeout(20000);

  String safe = prompt;
  safe.replace("\\", "\\\\");
  safe.replace("\"", "\\\"");
  safe.replace("\n", "\\n");
  safe.replace("\r", "");
  safe.replace("\t", " ");

  String payload =
    "{\"contents\":[{\"parts\":[{\"text\":\""
    + safe +
    "\"}]}]}";

  Serial.println("[Gemini] POST: " + prompt.substring(0, 80));

  int httpCode = https.POST(payload);

  if (httpCode <= 0) {
    Serial.println("[Gemini] HTTP err: " + String(httpCode));
    https.end();
    return "Network error (" + String(httpCode) + "). Check WiFi.";
  }

  Serial.println("[Gemini] HTTP " + String(httpCode));

  String response = https.getString();
  https.end();
  Serial.println("[Gemini] Raw response length: " + String(response.length()));
  Serial.println("[Gemini] Raw response: " + response.substring(0, 300));

  DynamicJsonDocument doc(32768);
  DeserializationError err = deserializeJson(doc, response);

  if (err) {
    Serial.println("[Gemini] JSON err: " + String(err.c_str()));
    return "JSON parse error: " + String(err.c_str());
  }

  if (doc.containsKey("error")) {
    const char* msg = doc["error"]["message"] | "Unknown API error";
    Serial.println("[Gemini] API error: " + String(msg));
    return String("API error: ") + msg;
  }

  JsonVariant textNode =
    doc["candidates"][0]["content"]["parts"][0]["text"];

  if (textNode.isNull()) {
    Serial.println("[Gemini] No text node found. Raw: " + response.substring(0, 200));
    return "No answer returned. Try again.";
  }

  String answer = textNode.as<String>();

  answer.replace("**", "");
  answer.replace("##", "");
  answer.replace("###", "");
  answer.replace("* ", "- ");
  answer.replace("`", "'");

  while (answer.indexOf("\n\n\n") >= 0) answer.replace("\n\n\n", "\n\n");
  answer.trim();

  Serial.println("[Gemini] Answer (" + String(answer.length()) + " chars): "
                 + answer.substring(0, 120));

  return answer;
}

// ── Send message (user presses ENTER) ───────────────────────────
void cb_sendMessage() {
  if (cb_inputBuf.length() == 0) return;

  String question = cb_inputBuf;
  cb_inputBuf = "";

  cb_addMessage(question, true);
  bool wifiOk = (WiFi.status() == WL_CONNECTED);
  cb_drawHeader(wifiOk, true);
  cb_drawChatArea();
  cb_drawInputBox();

  cb_appState = CB_STATE_THINKING;
  Serial.println("[Chat] User: " + question);

  String reply = cb_askGemini(question);

  cb_addMessage(reply, false);
  cb_appState = CB_STATE_CHAT;

  cb_drawFullScreen();

  Serial.println("[Chat] AI reply stored. Lines: "
                 + String(cb_chatLog[cb_chatCount - 1].lineCount));
}

// ── Keyboard touch handler ──────────────────────────────────────
void cb_handleKeyboardTouch() {
  char k = cb_detectKey();
  if (k == CB_KEY_NONE) return;

  switch (k) {
    case CB_KEY_ENTER:
      cb_sendMessage();
      break;

    case CB_KEY_BACKSPACE:
      if (cb_inputBuf.length() > 0) {
        cb_inputBuf.remove(cb_inputBuf.length() - 1);
        cb_drawInputBox();
      }
      break;

    case CB_KEY_CAPS:
      cb_capsLock = !cb_capsLock;
      cb_drawKeyboard();
      break;

    case CB_KEY_PUNCT:
      cb_showPunctPicker();
      break;

    case ' ':
      if ((int)cb_inputBuf.length() < CB_MAX_INPUT_LEN) {
        cb_inputBuf += ' ';
        cb_drawInputBox();
      }
      break;

    default:
      if (k >= 32 && (int)cb_inputBuf.length() < CB_MAX_INPUT_LEN) {
        cb_inputBuf += k;
        cb_drawInputBox();
      }
      break;
  }
}

// ── Chat area scroll touch ──────────────────────────────────────
void cb_handleChatTouch() {
  int mid = CB_CHAT_Y + CB_CHAT_H / 2;
  if (cb_touchY < mid && cb_chatScroll > 0) {
    cb_chatScroll--;
    cb_drawChatArea();
  } else if (cb_touchY >= mid && cb_chatScroll + 1 < cb_chatCount) {
    cb_chatScroll++;
    cb_drawChatArea();
  }
}

// ── WiFi connect ────────────────────────────────────────────────
void cb_connectWiFi() {
  Serial.print("[WiFi] Connecting to: ");
  Serial.println(CB_WIFI_SSID);

  WiFi.begin(CB_WIFI_SSID, CB_WIFI_PASSWORD);
  WiFi.setAutoReconnect(true);

  tft.setTextColor(CB_C_SUBTEXT, CB_C_BG);
  tft.setTextSize(1);
  tft.setCursor(100, CB_SCR_H / 2 + 18);
  tft.print("Connecting to WiFi...");

  int bx = 80, by = CB_SCR_H / 2 + 34, bw = 320, bh = 10;
  tft.drawRoundRect(bx, by, bw, bh, 3, CB_C_DIVIDER);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    attempts++;
    tft.fillRoundRect(bx + 1, by + 1, (attempts * (bw - 2)) / 30, bh - 2, 2, CB_C_ACCENT);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    tft.fillRoundRect(bx + 1, by + 1, bw - 2, bh - 2, 2, CB_C_WIFI_OK);
    tft.setTextColor(CB_C_WIFI_OK, CB_C_BG);
    tft.setCursor(100, CB_SCR_H / 2 + 50);
    tft.print("Connected!  IP: " + WiFi.localIP().toString());
    Serial.println("[WiFi] Connected. IP: " + WiFi.localIP().toString());
    delay(600);
  } else {
    tft.setTextColor(CB_C_WIFI_FAIL, CB_C_BG);
    tft.setCursor(100, CB_SCR_H / 2 + 50);
    tft.print("Failed. Will retry...");
    Serial.println("[WiFi] Failed.");
    delay(800);
  }
}

// ── Splash screen ────────────────────────────────────────────────
void cb_showSplash() {
  tft.fillScreen(CB_C_BG);

  tft.fillRect(0, 0, CB_SCR_W, 60, CB_C_HEADER_BG);
  tft.fillRect(0, 0, 5, 60, CB_C_ACCENT);

  tft.setTextColor(CB_C_TEXT, CB_C_HEADER_BG);
  tft.setTextSize(3);
  tft.setCursor(16, 12);
  tft.print("AI Assistant");

  tft.setTextColor(CB_C_ACCENT_DIM, CB_C_HEADER_BG);
  tft.setTextSize(1);
  tft.setCursor(16, 46);
  tft.print("ESP32  +  Gemini 2.5 Flash  +  ILI9488  480x320");

  tft.drawFastHLine(0, 62, CB_SCR_W, CB_C_ACCENT_DIM);

  tft.setTextColor(CB_C_SUBTEXT, CB_C_BG);
  tft.setTextSize(1);
  int ty = 78;
  tft.setCursor(40, ty);      tft.print("Display  :  ILI9488  480x320  SPI");
  tft.setCursor(40, ty + 16); tft.print("Touch    :  XPT2046  (TFT_eSPI built-in)");
  tft.setCursor(40, ty + 32); tft.print("Network  :  WiFi 802.11 b/g/n");
  tft.setCursor(40, ty + 48); tft.print("AI Model :  Google Gemini 2.5 Flash");
  tft.setCursor(40, ty + 64); tft.print("Answer   :  Displayed as chat bubbles on screen");
}

// ── Public API used by main.ino / actions.c ──────────────────────

void chatbot_setup() {
  // Touch calibration is already set once, globally, in main.ino —
  // no need to call tft.setTouch() here.

  // Allocate the chat log on the heap (not as a static array) — see the
  // note above cb_chatLog's declaration for why.
  if (!cb_chatLog) {
    cb_chatLog = new CbChatMsg[CB_MAX_MESSAGES];
  }
}

void chatbot_request_open() {
  chatbot_open_requested = true;
}

int chatbot_consume_open_request() {
  if (chatbot_open_requested) {
    chatbot_open_requested = false;
    return 1;
  }
  return 0;
}

void chatbot_open() {
  chatbot_active = true;

  cb_showSplash();
  delay(1200);

  cb_connectWiFi();
  WiFi.setSleep(false);   // disables modem sleep -> no periodic radio wake spikes
  cb_drawFullScreen();
}

void chatbot_close() {
  chatbot_active = false;
  // objects.main was already LVGL's active screen the whole time the
  // ChatBot was open (we only drew over it with raw tft calls). Force
  // a full repaint so LVGL redraws over our raw drawing on the next
  // lv_timer_handler().
  lv_obj_invalidate(objects.main);
}

void chatbot_loop() {
  if (!chatbot_active) return;

  static bool          wasTouched  = false;
  static unsigned long lastTouchMs = 0;
  static unsigned long lastWifiMs  = 0;

  if (millis() - lastWifiMs > CB_WIFI_CHECK_MS) {
    lastWifiMs = millis();
    if (cb_appState == CB_STATE_CHAT)
      cb_drawHeader(WiFi.status() == WL_CONNECTED, false);
  }

  if (cb_appState == CB_STATE_THINKING) {
    return;   // main.ino's loop() already applies a small delay each pass
  }

  cb_updateTouch();

  if (cb_touchValid && !wasTouched && millis() - lastTouchMs > CB_DEBOUNCE_MS) {
    wasTouched  = true;
    lastTouchMs = millis();

    // BACK button (new) — checked first, exits the whole app
    if (cb_inRect(4, 4, CB_BACK_W, CB_BACK_H)) {
      chatbot_close();
      return;
    }

    if (cb_touchY >= CB_KB_Y)
      cb_handleKeyboardTouch();
    else if (cb_touchY >= CB_CHAT_Y && cb_touchY < CB_INPUT_Y)
      cb_handleChatTouch();
  }

  if (!cb_touchValid) wasTouched = false;
}

int chatbot_is_active() {
  return chatbot_active ? 1 : 0;
}

