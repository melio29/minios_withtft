
#define ENABLE_SMTP
#define ENABLE_IMAP

#include <TFT_eSPI.h>
#include <SPI.h>
#include <WiFi.h>
#include <ESP_Mail_Client.h>

#include <esp_heap_caps.h>
#include <new>
#include "config.h"
#include "ui.h"      // for objects.main
#include <lvgl.h>    // for lv_obj_invalidate
#include "email.h"

extern TFT_eSPI tft;  // the single tft instance created in main.ino

// These are called from actions.c (plain C) and main.ino (C++).


#define EM_SCR_W 480
#define EM_SCR_H 320

// ── Mail sessions ─────────────────────────────────────────────
// Allocated on the heap (in em_setup) rather than as static objects —
// these library classes reserve internal buffers that were contributing
// to the DRAM overflow when combined with the other apps' static memory.
static SMTPSession* em_smtp = nullptr;
static IMAPSession* em_imap = nullptr;

// ── Screens ──────────────────────────────────────────────────
enum EmScreenState { EM_SCREEN_INBOX, EM_SCREEN_VIEW_EMAIL, EM_SCREEN_COMPOSE, EM_SCREEN_KEYBOARD, EM_SCREEN_LOADING, EM_SCREEN_DRAFTS };
static EmScreenState em_currentScreen = EM_SCREEN_LOADING;
static EmScreenState em_screenBeforeKeyboard = EM_SCREEN_COMPOSE;

// ── Inbox data ────────────────────────────────────────────────
#define EM_MAX_EMAILS 8
struct EmEmailHeader {
  String from;
  String subject;
  String date;
  int msgNo;
};
static EmEmailHeader* em_inbox = nullptr;   // allocated on the heap in em_setup()
static int em_emailCount = 0;

// ── Viewed email ──────────────────────────────────────────────
static String em_viewedFrom = "";
static String em_viewedSubject = "";
#define EM_MAX_BODY_LINES 200
static String* em_bodyLines = nullptr;   // allocated on the heap in em_setup()
static int em_bodyLineCount = 0;
static int em_bodyScroll = 0;
#define EM_VISIBLE_BODY_LINES 10

// ── Compose data ──────────────────────────────────────────────
static String em_composeTo = "";
static String em_composeSubject = "";
static String em_composeBody = "";
static int em_activeField = 0; // 0 = To, 1 = Subject, 2 = Body

// ── Drafts (in-memory, editable) ─────────────────────────────
#define EM_MAX_DRAFTS 4
struct EmDraft {
  String to;
  String subject;
  String body;
  bool used;
};
static EmDraft* em_drafts = nullptr;   // allocated on the heap in em_setup()
static int em_editingDraftIndex = -1; // -1 = new/unsaved compose, else index of draft being edited

// ── Keyboard ──────────────────────────────────────────────────
static String em_kbBuffer = "";
static bool em_kbShift = false;

// ── Status ────────────────────────────────────────────────────
static String em_statusMessage = "Starting...";

// ── App lifecycle state (added for LVGL integration) ───────────
static bool em_active = false;
static bool em_open_requested = false;

// -----------------------------------------------------------------------------
// PSRAM allocation helpers
// -----------------------------------------------------------------------------
// The Email app contains large arrays of String objects and mail-session
// objects. Put these objects themselves in external PSRAM when available.
// Placement-new is required because ps_malloc/heap_caps_malloc only reserves
// raw memory and does NOT run C++ constructors.
template <typename T>
static T* em_allocObjectPSRAM() {
  void *mem = heap_caps_malloc(sizeof(T), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (mem) {
    Serial.printf("[EMAIL] PSRAM object: %u bytes\n", (unsigned)sizeof(T));
    return new (mem) T();
  }

  Serial.printf("[EMAIL] PSRAM allocation failed for %u bytes, using heap\n",
                (unsigned)sizeof(T));
  return new T();
}

template <typename T>
static T* em_allocArrayPSRAM(size_t count) {
  if (count == 0) return nullptr;

  size_t bytes = sizeof(T) * count;

  void *mem = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

  if (mem) {
    T *array = static_cast<T *>(mem);

    // Construct every C++ object in the raw PSRAM block.
    for (size_t i = 0; i < count; ++i) {
      new (&array[i]) T();
    }

    Serial.printf("[EMAIL] PSRAM array: %u objects / %u bytes\n",
                  (unsigned)count, (unsigned)bytes);
    return array;
  }

  Serial.printf("[EMAIL] PSRAM array allocation failed (%u bytes), using heap\n",
                (unsigned)bytes);

  return new T[count]();
}

static void em_printMemory() {
  Serial.println("[EMAIL] -------- MEMORY --------");
  Serial.printf("[EMAIL] Free heap     : %u\n", ESP.getFreeHeap());
  Serial.printf("[EMAIL] Min free heap : %u\n", ESP.getMinFreeHeap());

  if (psramFound()) {
    Serial.printf("[EMAIL] Free PSRAM    : %u\n", ESP.getFreePsram());
    Serial.printf("[EMAIL] Total PSRAM   : %u\n", ESP.getPsramSize());
  } else {
    Serial.println("[EMAIL] PSRAM         : NOT FOUND");
  }

  Serial.println("[EMAIL] -------------------------");
}

// Forward declarations for screens referenced before their definition
void em_drawInboxScreen();
void em_drawViewEmailScreen();
void em_drawComposeScreen();
void em_drawDraftsScreen();
void em_openKeyboard(EmScreenState returnTo);
void em_drawKeyboardScreen();
void em_close();
int em_fetchInboxHeaders();

// ---------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------
bool em_inRect(int x, int y, int rx, int ry, int rw, int rh) {
  return (x >= rx && x <= rx + rw && y >= ry && y <= ry + rh);
}

void em_drawButton(int x, int y, int w, int h, const char *label, uint16_t bg = TFT_BLUE, uint16_t fg = TFT_WHITE) {
  tft.fillRoundRect(x, y, w, h, 6, bg);
  tft.drawRoundRect(x, y, w, h, 6, TFT_BLACK);
  tft.setTextColor(fg, bg);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(label, x + w / 2, y + h / 2, 2);
  tft.setTextDatum(TL_DATUM);
}

void em_showLoading(const char *msg) {
  em_currentScreen = EM_SCREEN_LOADING;
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextDatum(MC_DATUM);
  tft.drawString(msg, EM_SCR_W / 2, EM_SCR_H / 2, 4);
  tft.setTextDatum(TL_DATUM);
}

void em_wrapTextInto(String text, String *outLines, int &outCount, int maxLines, int maxCharsPerLine) {
  outCount = 0;
  int start = 0;
  int len = text.length();
  while (start < len && outCount < maxLines) {
    int remaining = len - start;
    int take = min(remaining, maxCharsPerLine);
    int seg_end = start + take;

    if (take == maxCharsPerLine && seg_end < len) {
      int lastSpace = text.lastIndexOf(' ', seg_end);
      if (lastSpace > start) seg_end = lastSpace;
    }
    String line = text.substring(start, seg_end);
    line.replace("\r", "");
    int nl = line.indexOf('\n');
    if (nl >= 0) {
      outLines[outCount++] = line.substring(0, nl);
      start = start + nl + 1;
      continue;
    }
    outLines[outCount++] = line;
    start = seg_end;
    while (start < len && text.charAt(start) == ' ') start++;
  }
}

// ---------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------
void em_connectWiFi() {
  if (WiFi.status() == WL_CONNECTED) {
    em_statusMessage = "WiFi OK: " + WiFi.localIP().toString();
    return;   // already connected (e.g. from ChatBot app earlier) — don't reconnect
  }
  em_showLoading("Connecting WiFi...");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries < 40) {
    delay(300);
    tries++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    em_statusMessage = "WiFi OK: " + WiFi.localIP().toString();
  } else {
    em_statusMessage = "WiFi FAILED - check config.h";
  }
}

// ---------------------------------------------------------------
// Mail: SMTP send
// ---------------------------------------------------------------
void em_smtpCallback(SMTP_Status status) {
  Serial.println(status.info());
}

bool em_sendEmailNow(String to, String subject, String body) {
  em_showLoading("Sending email...");

  if (!em_smtp) {
    em_smtp = new SMTPSession();
    if (!em_smtp) {
      em_statusMessage = "SMTP memory allocation failed";
      return false;
    }
  }

  ESP_Mail_Session session;
  session.server.host_name = SMTP_HOST;
  session.server.port = SMTP_PORT;
  session.login.email = GMAIL_EMAIL;
  session.login.password = GMAIL_APP_PASSWORD;
  session.login.user_domain = "";

  SMTP_Message message;
  message.sender.name = "ESP32 Mail";
  message.sender.email = GMAIL_EMAIL;
  message.subject = subject.c_str();
  message.addRecipient("Recipient", to.c_str());
  message.text.content = body.c_str();
  message.text.charSet = "utf-8";

  em_smtp->callback(em_smtpCallback);

  if (!em_smtp->connect(&session)) {
    em_statusMessage = "SMTP connect failed";
    return false;
  }
  if (!MailClient.sendMail(em_smtp, &message)) {
    em_statusMessage = "Send failed: " + String(em_smtp->errorReason().c_str());
    em_smtp->closeSession();
    return false;
  }
  em_smtp->closeSession();
  em_statusMessage = "Email sent!";
  return true;
}

// ---------------------------------------------------------------
// Mail: IMAP fetch
// ---------------------------------------------------------------
void em_imapCallback(IMAP_Status status) {
  Serial.println(status.info());
}

bool em_imapOpenSession(ESP_Mail_Session &session, IMAP_Config &config) {

  Serial.println();
  Serial.println("#### Starting IMAP connection...");

  if (!em_imap) {
    Serial.println("#### IMAP object is NULL!");

    em_imap = new IMAPSession();

    if (!em_imap) {
      Serial.println("#### IMAP allocation failed!");
      return false;
    }
  }

  // Gmail IMAP configuration
  session.server.host_name = IMAP_HOST;
  session.server.port = IMAP_PORT;

  session.login.email = GMAIL_EMAIL;
  session.login.password = GMAIL_APP_PASSWORD;

  // IMAP callback
  em_imap->callback(em_imapCallback);

  Serial.print("IMAP HOST: ");
  Serial.println(IMAP_HOST);

  Serial.print("IMAP PORT: ");
  Serial.println(IMAP_PORT);

  Serial.println("#### Connecting...");

  if (!em_imap->connect(&session, &config)) {

    Serial.println("#### IMAP CONNECTION FAILED");

    Serial.print("Reason: ");
    Serial.println(em_imap->errorReason().c_str());

    return false;
  }

  Serial.println("#### IMAP CONNECTED!");

  Serial.println("#### Opening INBOX...");

  if (!em_imap->selectFolder("INBOX")) {

    Serial.println("#### INBOX OPEN FAILED");

    Serial.print("Reason: ");
    Serial.println(em_imap->errorReason().c_str());

    em_imap->closeSession();

    return false;
  }

  Serial.println("#### INBOX OPENED!");

  return true;
}
int em_fetchInboxHeaders() {
  em_showLoading("Fetching inbox...");

  ESP_Mail_Session session;
  IMAP_Config config;
  config.enable.html = false;
  config.enable.text = false;
  config.limit.msg_size = 512;

  if (!em_imapOpenSession(session, config)) {
    return 0;
  }

  int total = em_imap->selectedFolder().msgCount();
  em_emailCount = 0;

  for (int n = total; n > 0 && em_emailCount < EM_MAX_EMAILS; n--) {
    config.fetch.number = n;
    config.fetch.set_seen = false;
    if (MailClient.readMail(em_imap)) {
      IMAP_MSG_List msgList = em_imap->data();
      if (msgList.msgItems.size() > 0) {
        IMAP_MSG_Item m = msgList.msgItems[0];
        em_inbox[em_emailCount].from = String(m.from);
        em_inbox[em_emailCount].subject = String(m.subject);
        em_inbox[em_emailCount].date = String(m.date);
        em_inbox[em_emailCount].msgNo = n;
        em_emailCount++;
      }
    }
  }

  em_imap->closeSession();
  em_statusMessage = String(em_emailCount) + " messages loaded";
  return em_emailCount;
}

bool em_fetchEmailBody(int msgNo) {
  em_showLoading("Opening email...");

  ESP_Mail_Session session;
  IMAP_Config config;
  config.enable.html = false;
  config.enable.text = true;
  config.limit.msg_size = 4096;
  config.fetch.number = msgNo;

  if (!em_imapOpenSession(session, config)) {
    return false;
  }

  bool ok = false;
  if (MailClient.readMail(em_imap)) {
    IMAP_MSG_List msgList = em_imap->data();
    if (msgList.msgItems.size() > 0) {
      IMAP_MSG_Item m = msgList.msgItems[0];
      em_viewedFrom = String(m.from);
      em_viewedSubject = String(m.subject);
      String bodyText = String(m.text.content);
      if (bodyText.length() == 0) bodyText = "(no plain-text body found)";
      if (em_bodyLines) {
        em_wrapTextInto(bodyText, em_bodyLines, em_bodyLineCount, EM_MAX_BODY_LINES, 46);
      }
      em_bodyScroll = 0;
      ok = true;
    }
  }
  em_imap->closeSession();
  return ok;
}

// ---------------------------------------------------------------
// Screen: INBOX
// ---------------------------------------------------------------
// showBack=true draws an EXIT arrow (app-level, new) that leaves the whole
// Email app back to the LVGL main menu. Other screens keep their own
// explicit "Back" button (already present) for internal navigation.
void em_drawTopBar(const char *title, bool showBack = false) {
  tft.fillRect(0, 0, EM_SCR_W, 34, TFT_RED);

  int titleX = 10;
  if (showBack) {
    tft.fillRoundRect(4, 4, 32, 26, 5, TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextDatum(MC_DATUM);
    tft.drawString("<", 20, 17, 2);
    tft.setTextDatum(TL_DATUM);
    titleX = 44;
  }

  tft.setTextColor(TFT_WHITE, TFT_RED);
  tft.setTextDatum(ML_DATUM);
  tft.drawString(title, titleX, 17, 4);
  tft.setTextDatum(TL_DATUM);
}

void em_drawInboxScreen() {
  em_currentScreen = EM_SCREEN_INBOX;
  tft.fillScreen(TFT_WHITE);
  em_drawTopBar("Inbox", true);   // true = show app-exit button (new)

  em_drawButton(220, 4, 75, 26, "Refresh", TFT_DARKGREY);
  em_drawButton(300, 4, 75, 26, "Drafts", TFT_ORANGE, TFT_BLACK);
  em_drawButton(390, 4, 80, 26, "Compose", TFT_BLUE);

  int rowY = 40;
  int rowH = 34;
  if (em_emailCount == 0) {
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("No messages. Tap Refresh.", 10, 50, 2);
  }
  for (int i = 0; i < em_emailCount; i++) {
    int y = rowY + i * rowH;
    tft.drawFastHLine(0, y, EM_SCR_W, TFT_LIGHTGREY);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    String fromShort = em_inbox[i].from;
    if (fromShort.length() > 22) fromShort = fromShort.substring(0, 22) + "..";
    String subjShort = em_inbox[i].subject;
    if (subjShort.length() > 34) subjShort = subjShort.substring(0, 34) + "..";
    tft.drawString(fromShort, 8, y + 4, 2);
    tft.drawString(subjShort, 180, y + 4, 2);
  }
  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString(em_statusMessage, 8, EM_SCR_H - 16, 1);
}

void em_handleInboxTouch(int x, int y) {
  // App-exit button (new) — checked first
  if (em_inRect(x, y, 4, 4, 32, 26)) {
    em_close();
    return;
  }
  if (em_inRect(x, y, 220, 4, 75, 26)) {
    em_fetchInboxHeaders();
    em_drawInboxScreen();
    return;
  }
  if (em_inRect(x, y, 300, 4, 75, 26)) {
    em_drawDraftsScreen();
    return;
  }
  if (em_inRect(x, y, 390, 4, 80, 26)) {
    em_composeTo = "";
    em_composeSubject = "";
    em_composeBody = "";
    em_editingDraftIndex = -1;
    em_drawComposeScreen();
    return;
  }
  int rowY = 40, rowH = 34;
  for (int i = 0; i < em_emailCount; i++) {
    int ry = rowY + i * rowH;
    if (em_inRect(x, y, 0, ry, EM_SCR_W, rowH)) {
      if (em_fetchEmailBody(em_inbox[i].msgNo)) {
        em_drawViewEmailScreen();
      } else {
        em_drawInboxScreen();
      }
      return;
    }
  }
}

// ---------------------------------------------------------------
// Screen: VIEW EMAIL
// ---------------------------------------------------------------
void em_drawViewEmailScreen() {
  em_currentScreen = EM_SCREEN_VIEW_EMAIL;
  tft.fillScreen(TFT_WHITE);
  em_drawTopBar("Message");
  em_drawButton(10, 4, 70, 26, "Back", TFT_DARKGREY);

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  String fromLine = "From: " + em_viewedFrom;
  if (fromLine.length() > 55) fromLine = fromLine.substring(0, 55) + "..";
  tft.drawString(fromLine, 8, 40, 2);
  String subjLine = "Subj: " + em_viewedSubject;
  if (subjLine.length() > 55) subjLine = subjLine.substring(0, 55) + "..";
  tft.drawString(subjLine, 8, 58, 2);
  tft.drawFastHLine(0, 78, EM_SCR_W, TFT_BLACK);

  int y = 86;
  for (int i = 0; i < EM_VISIBLE_BODY_LINES; i++) {
    int idx = em_bodyScroll + i;
    if (idx >= em_bodyLineCount) break;
    tft.drawString(em_bodyLines[idx], 8, y, 2);
    y += 18;
  }

  em_drawButton(390, 40, 80, 30, "Up", TFT_DARKGREY);
  em_drawButton(390, 74, 80, 30, "Down", TFT_DARKGREY);
}

void em_handleViewTouch(int x, int y) {
  if (em_inRect(x, y, 10, 4, 70, 26)) {
    em_drawInboxScreen();
    return;
  }
  if (em_inRect(x, y, 390, 40, 80, 30)) {
    if (em_bodyScroll > 0) em_bodyScroll -= EM_VISIBLE_BODY_LINES;
    if (em_bodyScroll < 0) em_bodyScroll = 0;
    em_drawViewEmailScreen();
    return;
  }
  if (em_inRect(x, y, 390, 74, 80, 30)) {
    if (em_bodyScroll + EM_VISIBLE_BODY_LINES < em_bodyLineCount) em_bodyScroll += EM_VISIBLE_BODY_LINES;
    em_drawViewEmailScreen();
    return;
  }
}

// ---------------------------------------------------------------
// Screen: COMPOSE
// ---------------------------------------------------------------
void em_drawComposeScreen() {
  em_currentScreen = EM_SCREEN_COMPOSE;
  tft.fillScreen(TFT_WHITE);
  em_drawTopBar(em_editingDraftIndex >= 0 ? "Edit Draft" : "Compose");
  em_drawButton(10, 4, 70, 26, "Back", TFT_DARKGREY);
  em_drawButton(300, 4, 80, 26, "Save", TFT_ORANGE, TFT_BLACK);
  em_drawButton(390, 4, 80, 26, "Send", TFT_GREEN, TFT_BLACK);

  tft.setTextColor(TFT_BLACK, TFT_WHITE);
  tft.drawString("To:", 8, 46, 2);
  tft.drawRoundRect(60, 40, 410, 26, 4, TFT_BLACK);
  tft.drawString(em_composeTo, 66, 46, 2);

  tft.drawString("Subj:", 8, 80, 2);
  tft.drawRoundRect(60, 74, 410, 26, 4, TFT_BLACK);
  tft.drawString(em_composeSubject, 66, 80, 2);

  tft.drawString("Body:", 8, 114, 2);
  tft.drawRoundRect(60, 108, 410, 90, 4, TFT_BLACK);
  String tmp[6];
  int tmpCount = 0;
  em_wrapTextInto(em_composeBody, tmp, tmpCount, 6, 55);
  int by = 114;
  for (int i = 0; i < tmpCount; i++) {
    tft.drawString(tmp[i], 66, by, 2);
    by += 16;
  }

  tft.setTextColor(TFT_DARKGREY, TFT_WHITE);
  tft.drawString("Tap a field to type on it", 8, 205, 2);
  tft.drawString(em_statusMessage, 8, EM_SCR_H - 16, 1);
}

void em_handleComposeTouch(int x, int y) {
  if (em_inRect(x, y, 10, 4, 70, 26)) {
    em_drawInboxScreen();
    return;
  }
  if (em_inRect(x, y, 300, 4, 80, 26)) {
    int slot = (em_editingDraftIndex >= 0) ? em_editingDraftIndex : -1;
    if (slot < 0) {
      for (int i = 0; i < EM_MAX_DRAFTS; i++) {
        if (!em_drafts[i].used) { slot = i; break; }
      }
    }
    if (slot < 0) {
      em_statusMessage = "Drafts full - delete one first";
    } else {
      em_drafts[slot].to = em_composeTo;
      em_drafts[slot].subject = em_composeSubject;
      em_drafts[slot].body = em_composeBody;
      em_drafts[slot].used = true;
      em_editingDraftIndex = slot;
      em_statusMessage = "Draft saved";
    }
    em_drawComposeScreen();
    return;
  }
  if (em_inRect(x, y, 390, 4, 80, 26)) {
    if (em_composeTo.length() == 0) {
      em_statusMessage = "Enter a To address first";
      em_drawComposeScreen();
      return;
    }
    bool ok = em_sendEmailNow(em_composeTo, em_composeSubject, em_composeBody);
    if (ok && em_editingDraftIndex >= 0) {
      em_drafts[em_editingDraftIndex].used = false;
      em_editingDraftIndex = -1;
    }
    em_drawComposeScreen();
    return;
  }
  if (em_inRect(x, y, 60, 40, 410, 26)) {
    em_activeField = 0;
    em_kbBuffer = em_composeTo;
    em_openKeyboard(EM_SCREEN_COMPOSE);
    return;
  }
  if (em_inRect(x, y, 60, 74, 410, 26)) {
    em_activeField = 1;
    em_kbBuffer = em_composeSubject;
    em_openKeyboard(EM_SCREEN_COMPOSE);
    return;
  }
  if (em_inRect(x, y, 60, 108, 410, 90)) {
    em_activeField = 2;
    em_kbBuffer = em_composeBody;
    em_openKeyboard(EM_SCREEN_COMPOSE);
    return;
  }
}

// ---------------------------------------------------------------
// Screen: DRAFTS
// ---------------------------------------------------------------
void em_drawDraftsScreen() {
  em_currentScreen = EM_SCREEN_DRAFTS;
  tft.fillScreen(TFT_WHITE);
  em_drawTopBar("Drafts");
  em_drawButton(10, 4, 70, 26, "Back", TFT_DARKGREY);

  int rowY = 40, rowH = 40;
  bool any = false;
  for (int i = 0; i < EM_MAX_DRAFTS; i++) {
    if (!em_drafts[i].used) continue;
    any = true;
    int y = rowY + i * rowH;
    tft.drawFastHLine(0, y, EM_SCR_W, TFT_LIGHTGREY);
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    String toShort = "To: " + em_drafts[i].to;
    String subjShort = "Subj: " + em_drafts[i].subject;
    if (toShort.length() > 40) toShort = toShort.substring(0, 40) + "..";
    if (subjShort.length() > 40) subjShort = subjShort.substring(0, 40) + "..";
    tft.drawString(toShort, 8, y + 3, 2);
    tft.drawString(subjShort, 8, y + 20, 1);
    em_drawButton(400, y + 4, 70, 30, "Delete", TFT_MAROON);
  }
  if (!any) {
    tft.setTextColor(TFT_BLACK, TFT_WHITE);
    tft.drawString("No saved drafts. Compose one and tap Save.", 10, 50, 2);
  }
}

void em_handleDraftsTouch(int x, int y) {
  if (em_inRect(x, y, 10, 4, 70, 26)) {
    em_drawInboxScreen();
    return;
  }
  int rowY = 40, rowH = 40;
  for (int i = 0; i < EM_MAX_DRAFTS; i++) {
    if (!em_drafts[i].used) continue;
    int y0 = rowY + i * rowH;
    if (em_inRect(x, y, 400, y0 + 4, 70, 30)) {
      em_drafts[i].used = false;
      em_drawDraftsScreen();
      return;
    }
    if (em_inRect(x, y, 0, y0, 400, rowH)) {
      em_composeTo = em_drafts[i].to;
      em_composeSubject = em_drafts[i].subject;
      em_composeBody = em_drafts[i].body;
      em_editingDraftIndex = i;
      em_drawComposeScreen();
      return;
    }
  }
}

// ---------------------------------------------------------------
// Screen: ON-SCREEN KEYBOARD
// ---------------------------------------------------------------
static const char *em_kbRowLower[4] = { "1234567890", "qwertyuiop", "asdfghjkl", "zxcvbnm" };
static const char *em_kbRowUpper[4] = { "1234567890", "QWERTYUIOP", "ASDFGHJKL", "ZXCVBNM" };

void em_openKeyboard(EmScreenState returnTo) {
  em_screenBeforeKeyboard = returnTo;
  em_drawKeyboardScreen();
}

void em_drawKeyboardScreen() {
  em_currentScreen = EM_SCREEN_KEYBOARD;
  tft.fillScreen(TFT_BLACK);

  tft.fillRect(0, 0, EM_SCR_W, 40, TFT_DARKGREY);
  tft.setTextColor(TFT_WHITE, TFT_DARKGREY);
  String preview = em_kbBuffer;
  if (preview.length() > 60) preview = preview.substring(preview.length() - 60);
  tft.drawString(preview, 6, 12, 2);

  const char **rows = em_kbShift ? em_kbRowUpper : em_kbRowLower;
  int keyW = 42, keyH = 40, gap = 4;
  int startY = 48;

  for (int r = 0; r < 4; r++) {
    int len = strlen(rows[r]);
    int rowWidth = len * (keyW + gap);
    int startX = (EM_SCR_W - rowWidth) / 2;
    for (int c = 0; c < len; c++) {
      int kx = startX + c * (keyW + gap);
      int ky = startY + r * (keyH + gap);
      char label[2] = { rows[r][c], '\0' };
      tft.fillRoundRect(kx, ky, keyW, keyH, 4, TFT_NAVY);
      tft.drawRoundRect(kx, ky, keyW, keyH, 4, TFT_WHITE);
      tft.setTextColor(TFT_WHITE, TFT_NAVY);
      tft.setTextDatum(MC_DATUM);
      tft.drawString(label, kx + keyW / 2, ky + keyH / 2, 2);
      tft.setTextDatum(TL_DATUM);
    }
  }

  int fy = startY + 4 * (keyH + gap);
  em_drawButton(4, fy, 60, keyH, "SHIFT", TFT_PURPLE);
  em_drawButton(70, fy, 200, keyH, "SPACE", TFT_NAVY);
  em_drawButton(276, fy, 40, keyH, "@", TFT_NAVY);
  em_drawButton(322, fy, 40, keyH, ".", TFT_NAVY);
  em_drawButton(368, fy, 50, keyH, "DEL", TFT_MAROON);
  em_drawButton(424, fy, 52, keyH, "DONE", TFT_DARKGREEN);
}

void em_handleKeyboardTouch(int x, int y) {
  const char **rows = em_kbShift ? em_kbRowUpper : em_kbRowLower;
  int keyW = 42, keyH = 40, gap = 4;
  int startY = 48;

  for (int r = 0; r < 4; r++) {
    int len = strlen(rows[r]);
    int rowWidth = len * (keyW + gap);
    int startX = (EM_SCR_W - rowWidth) / 2;
    for (int c = 0; c < len; c++) {
      int kx = startX + c * (keyW + gap);
      int ky = startY + r * (keyH + gap);
      if (em_inRect(x, y, kx, ky, keyW, keyH)) {
        em_kbBuffer += rows[r][c];
        em_drawKeyboardScreen();
        return;
      }
    }
  }

  int fy = startY + 4 * (keyH + gap);
  if (em_inRect(x, y, 4, fy, 60, keyH)) {
    em_kbShift = !em_kbShift;
    em_drawKeyboardScreen();
    return;
  }
  if (em_inRect(x, y, 70, fy, 200, keyH)) {
    em_kbBuffer += " ";
    em_drawKeyboardScreen();
    return;
  }
  if (em_inRect(x, y, 276, fy, 40, keyH)) {
    em_kbBuffer += "@";
    em_drawKeyboardScreen();
    return;
  }
  if (em_inRect(x, y, 322, fy, 40, keyH)) {
    em_kbBuffer += ".";
    em_drawKeyboardScreen();
    return;
  }
  if (em_inRect(x, y, 368, fy, 50, keyH)) {
    if (em_kbBuffer.length() > 0) em_kbBuffer.remove(em_kbBuffer.length() - 1);
    em_drawKeyboardScreen();
    return;
  }
  if (em_inRect(x, y, 424, fy, 52, keyH)) {
    if (em_activeField == 0) em_composeTo = em_kbBuffer;
    else if (em_activeField == 1) em_composeSubject = em_kbBuffer;
    else em_composeBody = em_kbBuffer;
    if (em_screenBeforeKeyboard == EM_SCREEN_COMPOSE) em_drawComposeScreen();
    else em_drawInboxScreen();
    return;
  }
}

// ---------------------------------------------------------------
// Touch dispatcher
// ---------------------------------------------------------------
void em_handleTouch(int x, int y) {
  switch (em_currentScreen) {
    case EM_SCREEN_INBOX:       em_handleInboxTouch(x, y); break;
    case EM_SCREEN_VIEW_EMAIL:  em_handleViewTouch(x, y); break;
    case EM_SCREEN_COMPOSE:     em_handleComposeTouch(x, y); break;
    case EM_SCREEN_KEYBOARD:    em_handleKeyboardTouch(x, y); break;
    case EM_SCREEN_DRAFTS:      em_handleDraftsTouch(x, y); break;
    default: break;
  }
}

// ---------------------------------------------------------------
// Public API used by main.ino / actions.c
// ---------------------------------------------------------------

void em_setup() {
  // Touch calibration is already set once, globally, in main.ino.
  // The CALIBRATE_MODE / runCalibration() / calData logic from the
  // original standalone sketch has been intentionally removed —
  // it would override the shared calibration and break touch in
  // the other apps (same issue we fixed earlier for notes.ino).
  // config.h's CALIBRATE_MODE and calData values are simply unused now.

  // Allocate the larger objects/arrays on the heap instead of as static
  // memory — this is what fixed the DRAM overflow (same technique used
  // for the chatbot's message log).
  if (!em_smtp) {
    em_smtp = em_allocObjectPSRAM<SMTPSession>();
  }

  if (!em_imap) {
    em_imap = em_allocObjectPSRAM<IMAPSession>();
  }

  if (!em_bodyLines) {
    em_bodyLines = em_allocArrayPSRAM<String>(EM_MAX_BODY_LINES);
  }

  if (!em_inbox) {
    em_inbox = em_allocArrayPSRAM<EmEmailHeader>(EM_MAX_EMAILS);
  }

  if (!em_drafts) {
    em_drafts = em_allocArrayPSRAM<EmDraft>(EM_MAX_DRAFTS);

    for (int i = 0; i < EM_MAX_DRAFTS; i++) {
      em_drafts[i].used = false;
    }
  }

  em_printMemory();
}

void em_request_open() {

  em_open_requested = true;
  // if (!em_smtp)      em_smtp = new SMTPSession();
  // if (!em_imap)      em_imap = new IMAPSession();
  // if (!em_bodyLines) em_bodyLines = new String[EM_MAX_BODY_LINES];
  // if (!em_inbox)     em_inbox = new EmEmailHeader[EM_MAX_EMAILS];
  // if (!em_drafts) {
  //   em_drafts = new EmDraft[EM_MAX_DRAFTS];
  //   for (int i = 0; i < EM_MAX_DRAFTS; i++) em_drafts[i].used = false;
  // }

  // em_active = true;
  // em_connectWiFi();
  // if (WiFi.status() == WL_CONNECTED) {
  //   em_fetchInboxHeaders();
  // }
  // em_drawInboxScreen();

}

int em_consume_open_request() {
  if (em_open_requested) {
    em_open_requested = false;
    return 1;
  }
  return 0;
}

void em_open() {
  em_active = true;

  // -------------------------------------------------
  // Allocate Email app resources FIRST.
  // This must happen before any IMAP/SMTP operation.
  // -------------------------------------------------
  if (!em_smtp) {
    em_smtp = em_allocObjectPSRAM<SMTPSession>();
  }

  if (!em_imap) {
    em_imap = em_allocObjectPSRAM<IMAPSession>();
  }

  if (!em_bodyLines) {
    em_bodyLines = em_allocArrayPSRAM<String>(EM_MAX_BODY_LINES);
  }

  if (!em_inbox) {
    em_inbox = em_allocArrayPSRAM<EmEmailHeader>(EM_MAX_EMAILS);
  }

  if (!em_drafts) {
    em_drafts = em_allocArrayPSRAM<EmDraft>(EM_MAX_DRAFTS);

    for (int i = 0; i < EM_MAX_DRAFTS; i++) {
      em_drafts[i].used = false;
    }
  }

  em_printMemory();

  // -------------------------------------------------
  // Network operations.
  // Do NOT keep tft.startWrite() active here.
  // WiFi/IMAP can take several seconds.
  // -------------------------------------------------
  em_connectWiFi();

  if (WiFi.status() == WL_CONNECTED) {
    em_fetchInboxHeaders();
  }

  // -------------------------------------------------
  // Only drawing uses the TFT transaction.
  // -------------------------------------------------
  tft.startWrite();
  em_drawInboxScreen();
  tft.endWrite();
}

void em_close() {
  em_active = false;
  em_printMemory();
  // objects.main was already LVGL's active screen the whole time Email
  // was open (we only drew over it with raw tft calls). Force a full
  // repaint so LVGL redraws over it on the next lv_timer_handler().
  lv_obj_invalidate(objects.main);
}

void em_loop() {
  if (!em_active) return;

  uint16_t tx, ty;
  bool touched = tft.getTouch(&tx, &ty);
  if (touched) {
    tft.startWrite();
    em_handleTouch(tx, ty);
    tft.endWrite();
    delay(180);   // simple debounce so one tap = one action
  }
}

int em_is_active() {
  return em_active ? 1 : 0;
}

