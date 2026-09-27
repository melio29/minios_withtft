
#include <TFT_eSPI.h>
#include "FS.h"
#include <LittleFS.h>
#include "ui.h"      // for objects.main
#include <lvgl.h>    // for lv_obj_invalidate

using namespace fs;   // fixes 'File' scope error on ESP32 Arduino core 3.x

extern TFT_eSPI tft;  // the single tft instance created in main.ino

// These are called from actions.c (plain C) and main.ino (C++).
extern "C" {
    void notes_setup();
    void notes_open();
    void notes_loop();
    int  notes_is_active();
    void notes_request_open();
    int  notes_consume_open_request();
}

// ── Colour palette ──────────────────────────────────────────
#define NT_C_BG        0x1082   // near-black   #101010
#define NT_C_SURFACE   0x2104   // dark card    #212121
#define NT_C_ACCENT    0x07FF   // cyan         #00FFFF  (signature colour)
#define NT_C_ACCENT2   0x03EF   // teal         #007B7B
#define NT_C_TEXT      0xFFFF   // white
#define NT_C_SUBTEXT   0x8410   // mid-grey
#define NT_C_KEY       0x2945   // key face     #294A52
#define NT_C_KEY_HL    0x07BF   // key pressed
#define NT_C_BTN_ADD   0x0438   // green-teal   #008070
#define NT_C_BTN_DEL   0x8000   // red
#define NT_C_DIVIDER   0x39C7   // subtle line

// ── Screen / layout constants ───────────────────────────────
#define NT_SCR_W  480
#define NT_SCR_H  320

// ── Keyboard layout ─────────────────────────────────────────
const char* NT_KB_ROW0 = "QWERTYUIOP";
const char* NT_KB_ROW1 = "ASDFGHJKL";
const char* NT_KB_ROW2 = "ZXCVBNM";

#define NT_KEY_W   42
#define NT_KEY_H   36
#define NT_KEY_GAP  3
#define NT_KB_Y    170   // top of keyboard area

// ── File system ─────────────────────────────────────────────
#define NT_NOTES_DIR  "/notes"
#define NT_MAX_NOTES  20
#define NT_MAX_NOTE_LEN 512

// ── UI state machine ─────────────────────────────────────────
enum NtScreen { NT_SCR_LIST, NT_SCR_EDIT, NT_SCR_VIEW };
NtScreen nt_currentScreen = NT_SCR_LIST;

// ── Note storage ─────────────────────────────────────────────
String nt_noteFiles[NT_MAX_NOTES];
int    nt_noteCount  = 0;
int    nt_listScroll = 0;   // first visible note index

String nt_editBuffer = "";   // current note being typed
int    nt_editIndex  = -1;   // -1 = new note
bool   nt_capsLock   = false;
bool   nt_numPanelOpen = false;   // numeric strip toggle (edit screen)

// ── App state (added for LVGL integration) ────────────────────
static bool notes_active = false;
static bool notes_open_requested = false;

// ── Touch state (plain globals) ───────────────────────────────
int  nt_touchX = 0;
int  nt_touchY = 0;
bool nt_touchValid = false;

void nt_updateTouch() {
  uint16_t tx, ty;
  nt_touchValid = tft.getTouch(&tx, &ty, 600);
  if (nt_touchValid) {
    nt_touchX = (int)tx;
    nt_touchY = (int)ty;
  }
}

bool nt_touchIn(int x, int y, int w, int h) {
  return nt_touchValid && nt_touchX >= x && nt_touchX < x + w && nt_touchY >= y && nt_touchY < y + h;
}

// ── LittleFS helpers ────────────────────────────────────────
void nt_loadNoteList() {
  nt_noteCount = 0;
  File root = LittleFS.open(NT_NOTES_DIR);
  if (!root || !root.isDirectory()) return;
  File f = root.openNextFile();
  while (f && nt_noteCount < NT_MAX_NOTES) {
    if (!f.isDirectory()) {
      String filename = String(f.name());
      int slash = filename.lastIndexOf('/');
      if (slash >= 0) filename = filename.substring(slash + 1);
      nt_noteFiles[nt_noteCount++] = filename;
    }
    f = root.openNextFile();
  }
}

String nt_readNote(const String& filename) {
  String path = filename;
  if (!path.startsWith("/")) path = String(NT_NOTES_DIR) + "/" + path;
  File f = LittleFS.open(path, "r");
  if (!f) return "";
  String s = f.readString();
  f.close();
  return s;
}

void nt_saveNote(const String& filename, const String& content) {
  String path = filename;
  if (!path.startsWith("/")) path = String(NT_NOTES_DIR) + "/" + path;
  File f = LittleFS.open(path, "w");
  if (!f) return;
  f.print(content);
  f.close();
}

void nt_deleteNote(const String& filename) {
  String path = filename;
  if (!path.startsWith("/")) path = String(NT_NOTES_DIR) + "/" + path;
  LittleFS.remove(path);
}

String nt_newFilename() {
  return "note_" + String(millis()) + ".txt";
}

// ── Draw helpers ─────────────────────────────────────────────
void nt_drawRoundRect(int x, int y, int w, int h, int r, uint16_t col, bool filled = false) {
  if (filled) tft.fillRoundRect(x, y, w, h, r, col);
  else        tft.drawRoundRect(x, y, w, h, r, col);
}

void nt_drawHeader(const String& title, bool showBack = false) {
  tft.fillRect(0, 0, NT_SCR_W, 40, NT_C_ACCENT2);
  tft.setTextColor(NT_C_TEXT, NT_C_ACCENT2);
  tft.setTextSize(2);
  tft.setCursor(showBack ? 50 : 10, 12);
  tft.print(title);
  if (showBack) {
    tft.fillRoundRect(4, 6, 38, 28, 5, NT_C_BG);
    tft.setTextColor(NT_C_ACCENT, NT_C_BG);
    tft.setCursor(8, 12);
    tft.print("< ");
  }
  tft.drawFastHLine(0, 40, NT_SCR_W, NT_C_ACCENT);
}

// ── LIST SCREEN ──────────────────────────────────────────────
void nt_drawListScreen() {
  tft.fillScreen(NT_C_BG);
  // showBack = true here: this is the app's top-level screen, so the
  // back arrow exits the whole Notes app back to the LVGL main menu.
  nt_drawHeader("My Notes", true);

  // Add Note button (top-right)
  tft.fillRoundRect(NT_SCR_W - 110, 6, 102, 28, 6, NT_C_BTN_ADD);
  tft.setTextColor(NT_C_TEXT, NT_C_BTN_ADD);
  tft.setTextSize(1);
  tft.setCursor(NT_SCR_W - 106, 14);
  tft.print("+ Add Note");

  if (nt_noteCount == 0) {
    tft.setTextColor(NT_C_SUBTEXT, NT_C_BG);
    tft.setTextSize(1);
    tft.setCursor(160, 150);
    tft.print("No notes yet.");
    tft.setCursor(130, 165);
    tft.print("Tap '+ Add Note' to start.");
    return;
  }

  int visibleMax = min(nt_noteCount - nt_listScroll, 5);
  for (int i = 0; i < visibleMax; i++) {
    int ni   = nt_listScroll + i;
    int cardY = 48 + i * 50;

    nt_drawRoundRect(6, cardY, NT_SCR_W - 70, 44, 6, NT_C_SURFACE, true);
    nt_drawRoundRect(6, cardY, NT_SCR_W - 70, 44, 6, NT_C_DIVIDER);

    String content = nt_readNote(nt_noteFiles[ni]);
    String preview = content.length() > 0 ? content.substring(0, min((int)content.length(), 50)) : "(empty)";
    preview.replace('\n', ' ');

    tft.setTextColor(NT_C_TEXT, NT_C_SURFACE);
    tft.setTextSize(1);
    tft.setCursor(14, cardY + 8);
    tft.print(nt_noteFiles[ni]);

    tft.setTextColor(NT_C_SUBTEXT, NT_C_SURFACE);
    tft.setCursor(14, cardY + 24);
    tft.print(preview);

    tft.fillRoundRect(NT_SCR_W - 60, cardY + 8, 50, 28, 5, NT_C_BTN_DEL);
    tft.setTextColor(NT_C_TEXT, NT_C_BTN_DEL);
    tft.setCursor(NT_SCR_W - 52, cardY + 16);
    tft.print("DEL");
  }

  if (nt_listScroll > 0) {
    tft.setTextColor(NT_C_ACCENT, NT_C_BG);
    tft.setCursor(NT_SCR_W / 2 - 10, 42);
    tft.print("^");
  }
  if (nt_listScroll + 5 < nt_noteCount) {
    tft.setTextColor(NT_C_ACCENT, NT_C_BG);
    tft.setCursor(NT_SCR_W / 2 - 10, NT_SCR_H - 12);
    tft.print("v");
  }
}

// Forward declarations
void nt_drawEditScreen();
void notes_close();

void nt_handleListTouch() {
  // App-exit BACK button (top-left header arrow) — leaves the Notes app
  // entirely and returns to the LVGL main menu.
  if (nt_touchIn(4, 6, 38, 28)) {
    notes_close();
    return;
  }
  // Add Note button
  if (nt_touchIn(NT_SCR_W - 110, 6, 102, 28)) {
    nt_editBuffer = "";
    nt_editIndex  = -1;
    nt_currentScreen = NT_SCR_EDIT;
    nt_drawEditScreen();
    return;
  }
  // Scroll up arrow
  if (nt_touchIn(NT_SCR_W / 2 - 20, 42, 40, 14) && nt_listScroll > 0) {
    nt_listScroll--;
    nt_drawListScreen();
    return;
  }
  // Scroll down arrow
  if (nt_touchIn(NT_SCR_W / 2 - 20, NT_SCR_H - 20, 40, 14) && nt_listScroll + 5 < nt_noteCount) {
    nt_listScroll++;
    nt_drawListScreen();
    return;
  }
  // Note cards
  int visibleMax = min(nt_noteCount - nt_listScroll, 5);
  for (int i = 0; i < visibleMax; i++) {
    int ni    = nt_listScroll + i;
    int cardY = 48 + i * 50;
    if (nt_touchIn(NT_SCR_W - 60, cardY + 8, 50, 28)) {
      nt_deleteNote(nt_noteFiles[ni]);
      nt_loadNoteList();
      if (nt_listScroll >= nt_noteCount) nt_listScroll = max(0, nt_noteCount - 1);
      nt_drawListScreen();
      return;
    }
    if (nt_touchIn(6, cardY, NT_SCR_W - 70, 44)) {
      nt_editBuffer = nt_readNote(nt_noteFiles[ni]);
      nt_editIndex  = ni;
      nt_currentScreen = NT_SCR_EDIT;
      nt_drawEditScreen();
      return;
    }
  }
}

// ── EDIT SCREEN ───────────────────────────────────────────────
void nt_drawTextArea() {
  tft.fillRect(0, 42, NT_SCR_W, NT_KB_Y - 44, NT_C_BG);
  nt_drawRoundRect(4, 44, NT_SCR_W - 8, NT_KB_Y - 48, 5, NT_C_SURFACE, true);
  nt_drawRoundRect(4, 44, NT_SCR_W - 8, NT_KB_Y - 48, 5, NT_C_DIVIDER);

  tft.setTextColor(NT_C_TEXT, NT_C_SURFACE);
  tft.setTextSize(1);

  String buf   = nt_editBuffer + "_";
  int    lineH = 14;
  int    maxW  = NT_SCR_W - 20;
  int    cx    = 10;
  int    cy    = 52;
  String word  = "";

  for (int i = 0; i < (int)buf.length(); i++) {
    char c = buf[i];
    if (c == '\n') {
      tft.setCursor(cx, cy);
      tft.print(word);
      word = "";
      cx = 10;
      cy += lineH;
      if (cy > NT_KB_Y - 20) break;
    } else {
      word += c;
      if ((int)word.length() * 6 > maxW - cx) {
        tft.setCursor(cx, cy);
        tft.print(word.substring(0, word.length() - 1));
        word = String(c);
        cx = 10;
        cy += lineH;
        if (cy > NT_KB_Y - 20) break;
      }
    }
  }
  if (cy <= NT_KB_Y - 20 && word.length() > 0) {
    tft.setCursor(cx, cy);
    tft.print(word);
  }
}

void nt_drawKey(int x, int y, int w, int h, const String& label, uint16_t bg = NT_C_KEY) {
  tft.fillRoundRect(x, y, w, h, 4, bg);
  tft.drawRoundRect(x, y, w, h, 4, NT_C_ACCENT2);
  tft.setTextColor(NT_C_TEXT, bg);
  tft.setTextSize(1);
  int tx = x + (w - label.length() * 6) / 2;
  int ty = y + (h - 8) / 2;
  tft.setCursor(tx, ty);
  tft.print(label);
}

void nt_drawKeyboard() {
  tft.fillRect(0, NT_KB_Y, NT_SCR_W, NT_SCR_H - NT_KB_Y, NT_C_BG);

  int r0x = (NT_SCR_W - (10 * (NT_KEY_W + NT_KEY_GAP) - NT_KEY_GAP)) / 2;
  for (int i = 0; i < 10; i++) {
    char c = nt_capsLock ? NT_KB_ROW0[i] : (NT_KB_ROW0[i] + 32);
    nt_drawKey(r0x + i * (NT_KEY_W + NT_KEY_GAP), NT_KB_Y + 2, NT_KEY_W, NT_KEY_H, String(c));
  }

  int r1x = (NT_SCR_W - (9 * (NT_KEY_W + NT_KEY_GAP) - NT_KEY_GAP)) / 2;
  for (int i = 0; i < 9; i++) {
    char c = nt_capsLock ? NT_KB_ROW1[i] : (NT_KB_ROW1[i] + 32);
    nt_drawKey(r1x + i * (NT_KEY_W + NT_KEY_GAP), NT_KB_Y + NT_KEY_H + NT_KEY_GAP + 2, NT_KEY_W, NT_KEY_H, String(c));
  }

  int r2y = NT_KB_Y + (NT_KEY_H + NT_KEY_GAP) * 2 + 2;
  uint16_t capsBg = nt_capsLock ? NT_C_ACCENT : NT_C_KEY;
  nt_drawKey(4, r2y, 44, NT_KEY_H, "CAP", capsBg);
  int r2x = 52;
  for (int i = 0; i < 7; i++) {
    char c = nt_capsLock ? NT_KB_ROW2[i] : (NT_KB_ROW2[i] + 32);
    nt_drawKey(r2x + i * (NT_KEY_W + NT_KEY_GAP), r2y, NT_KEY_W, NT_KEY_H, String(c));
  }
  int bkspX = r2x + 7 * (NT_KEY_W + NT_KEY_GAP);
  nt_drawKey(bkspX, r2y, NT_SCR_W - bkspX - 4, NT_KEY_H, "<X", NT_C_BTN_DEL);

  int r3y = r2y + NT_KEY_H + NT_KEY_GAP;
  nt_drawKey(4,   r3y, 44, NT_KEY_H, ".,!?", NT_C_KEY);
  nt_drawKey(52,  r3y, 270, NT_KEY_H, "SPACE", NT_C_KEY);
  nt_drawKey(326, r3y, 60, NT_KEY_H, "ENT", NT_C_ACCENT2);
  nt_drawKey(390, r3y, 86, NT_KEY_H, "SAVE", NT_C_BTN_ADD);
}

// ── Numeric strip (toggled from edit-screen header, replaces keyboard) ──
void nt_drawNumToggleBtn() {
  const char* label = nt_numPanelOpen ? "ABC" : "123";
  tft.fillRoundRect(NT_SCR_W - 50, 6, 42, 28, 5, NT_C_ACCENT2);
  tft.drawRoundRect(NT_SCR_W - 50, 6, 42, 28, 5, NT_C_ACCENT);
  tft.setTextColor(NT_C_TEXT, NT_C_ACCENT2);
  tft.setTextSize(1);
  tft.setCursor(NT_SCR_W - 44, 15);
  tft.print(label);
}

void nt_drawNumPanel() {
  tft.fillRect(0, NT_KB_Y, NT_SCR_W, NT_SCR_H - NT_KB_Y, NT_C_BG);

  // Row of digits 0-9 (same geometry as letter row0, keypad order 1..9,0)
  int r0x = (NT_SCR_W - (10 * (NT_KEY_W + NT_KEY_GAP) - NT_KEY_GAP)) / 2;
  for (int i = 0; i < 10; i++) {
    char c = '0' + ((i + 1) % 10);
    nt_drawKey(r0x + i * (NT_KEY_W + NT_KEY_GAP), NT_KB_Y + 2, NT_KEY_W, NT_KEY_H, String(c));
  }

  // Row: backspace + close
  int r1y = NT_KB_Y + NT_KEY_H + NT_KEY_GAP + 2;
  nt_drawKey(4, r1y, 200, NT_KEY_H, "<X BACKSPACE", NT_C_BTN_DEL);
  nt_drawKey(NT_SCR_W - 200, r1y, 196, NT_KEY_H, "CLOSE PANEL", NT_C_BTN_ADD);
}

char nt_handleNumPanelTouch() {
  int r0x = (NT_SCR_W - (10 * (NT_KEY_W + NT_KEY_GAP) - NT_KEY_GAP)) / 2;
  if (nt_touchY >= NT_KB_Y + 2 && nt_touchY < NT_KB_Y + 2 + NT_KEY_H) {
    for (int i = 0; i < 10; i++) {
      int kx = r0x + i * (NT_KEY_W + NT_KEY_GAP);
      if (nt_touchX >= kx && nt_touchX < kx + NT_KEY_W) {
        return '0' + ((i + 1) % 10);
      }
    }
  }
  int r1y = NT_KB_Y + NT_KEY_H + NT_KEY_GAP + 2;
  if (nt_touchY >= r1y && nt_touchY < r1y + NT_KEY_H) {
    if (nt_touchX >= 4 && nt_touchX < 204) return 8;    // backspace
    if (nt_touchX >= NT_SCR_W - 200) return 99;         // close panel
  }
  return 0;
}

void nt_drawEditScreen() {
  tft.fillScreen(NT_C_BG);
  bool isNew = (nt_editIndex == -1);
  // showBack = true here means "back to notes List" (internal nav),
  // NOT exit-the-app — handled distinctly in nt_handleEditTouch below.
  nt_drawHeader(isNew ? "New Note" : "Edit Note", true);
  nt_drawNumToggleBtn();
  nt_drawTextArea();
  if (nt_numPanelOpen) nt_drawNumPanel();
  else                 nt_drawKeyboard();
}

char nt_handleKeyboardTouch() {
  int r0x = (NT_SCR_W - (10 * (NT_KEY_W + NT_KEY_GAP) - NT_KEY_GAP)) / 2;
  if (nt_touchY >= NT_KB_Y + 2 && nt_touchY < NT_KB_Y + 2 + NT_KEY_H) {
    for (int i = 0; i < 10; i++) {
      int kx = r0x + i * (NT_KEY_W + NT_KEY_GAP);
      if (nt_touchX >= kx && nt_touchX < kx + NT_KEY_W) {
        return nt_capsLock ? NT_KB_ROW0[i] : (NT_KB_ROW0[i] + 32);
      }
    }
  }
  int r1x = (NT_SCR_W - (9 * (NT_KEY_W + NT_KEY_GAP) - NT_KEY_GAP)) / 2;
  int r1y = NT_KB_Y + NT_KEY_H + NT_KEY_GAP + 2;
  if (nt_touchY >= r1y && nt_touchY < r1y + NT_KEY_H) {
    for (int i = 0; i < 9; i++) {
      int kx = r1x + i * (NT_KEY_W + NT_KEY_GAP);
      if (nt_touchX >= kx && nt_touchX < kx + NT_KEY_W) {
        return nt_capsLock ? NT_KB_ROW1[i] : (NT_KB_ROW1[i] + 32);
      }
    }
  }
  int r2y = NT_KB_Y + (NT_KEY_H + NT_KEY_GAP) * 2 + 2;
  int r2x = 52;
  if (nt_touchY >= r2y && nt_touchY < r2y + NT_KEY_H) {
    if (nt_touchX >= 4 && nt_touchX < 48) return 1;
    for (int i = 0; i < 7; i++) {
      int kx = r2x + i * (NT_KEY_W + NT_KEY_GAP);
      if (nt_touchX >= kx && nt_touchX < kx + NT_KEY_W) {
        return nt_capsLock ? NT_KB_ROW2[i] : (NT_KB_ROW2[i] + 32);
      }
    }
    int bkspX = r2x + 7 * (NT_KEY_W + NT_KEY_GAP);
    if (nt_touchX >= bkspX) return 8;
  }
  int r3y = r2y + NT_KEY_H + NT_KEY_GAP;
  if (nt_touchY >= r3y && nt_touchY < r3y + NT_KEY_H) {
    if (nt_touchX >= 4   && nt_touchX < 48)  return 4;
    if (nt_touchX >= 52  && nt_touchX < 322) return ' ';
    if (nt_touchX >= 326 && nt_touchX < 386) return 10;
    if (nt_touchX >= 390)                   return 2;
  }
  return 0;
}

const String NT_PUNCTS = ".,!?\"'/:;@#-+()";
void nt_showPunctPicker() {
  int px = 4, py = NT_KB_Y - 40, pw = 300, ph = 36;
  tft.fillRoundRect(px, py, pw, ph, 6, NT_C_SURFACE);
  tft.drawRoundRect(px, py, pw, ph, 6, NT_C_ACCENT);
  int kw = 18;
  for (int i = 0; i < (int)NT_PUNCTS.length(); i++) {
    int kx = px + 4 + i * (kw + 2);
    tft.fillRoundRect(kx, py + 4, kw, 28, 3, NT_C_KEY);
    tft.setTextColor(NT_C_TEXT, NT_C_KEY);
    tft.setCursor(kx + 4, py + 12);
    tft.print(NT_PUNCTS[i]);
  }
  unsigned long t0 = millis();
  while (millis() - t0 < 4000) {
    nt_updateTouch();
    if (nt_touchValid && nt_touchY >= py && nt_touchY < py + ph && nt_touchX >= px && nt_touchX < px + pw) {
      int i = (nt_touchX - px - 4) / (kw + 2);
      if (i >= 0 && i < (int)NT_PUNCTS.length()) {
        if ((int)nt_editBuffer.length() < NT_MAX_NOTE_LEN) nt_editBuffer += NT_PUNCTS[i];
      }
      delay(150);
      break;
    }
    delay(10);
  }
  nt_drawTextArea();
}

void nt_handleEditTouch() {
  // Back button (top left) — internal nav: Edit -> List (does NOT exit app)
  if (nt_touchIn(4, 6, 38, 28)) {
    nt_loadNoteList();
    nt_listScroll = 0;
    nt_currentScreen = NT_SCR_LIST;
    nt_drawListScreen();
    return;
  }

  // 123 / ABC toggle button (top right of header)
  if (nt_touchIn(NT_SCR_W - 50, 6, 42, 28)) {
    nt_numPanelOpen = !nt_numPanelOpen;
    nt_drawNumToggleBtn();
    if (nt_numPanelOpen) nt_drawNumPanel();
    else                 nt_drawKeyboard();
    return;
  }

  if (nt_touchY >= NT_KB_Y) {
    if (nt_numPanelOpen) {
      char k = nt_handleNumPanelTouch();
      if (k == 99) {                       // CLOSE PANEL -> back to letters
        nt_numPanelOpen = false;
        nt_drawNumToggleBtn();
        nt_drawKeyboard();
      } else if (k == 8) {                 // backspace
        if (nt_editBuffer.length() > 0) nt_editBuffer.remove(nt_editBuffer.length() - 1);
        nt_drawTextArea();
      } else if (k != 0) {                 // digit
        if ((int)nt_editBuffer.length() < NT_MAX_NOTE_LEN) nt_editBuffer += k;
        nt_drawTextArea();
      }
      return;
    }

    char k = nt_handleKeyboardTouch();
    if (k == 2) {
      String fname = (nt_editIndex == -1) ? nt_newFilename() : nt_noteFiles[nt_editIndex];
      nt_saveNote(fname, nt_editBuffer);
      nt_loadNoteList();
      nt_listScroll = 0;
      nt_currentScreen = NT_SCR_LIST;
      nt_drawListScreen();
    } else if (k == 1) {
      nt_capsLock = !nt_capsLock;
      nt_drawKeyboard();
    } else if (k == 8) {
      if (nt_editBuffer.length() > 0) nt_editBuffer.remove(nt_editBuffer.length() - 1);
      nt_drawTextArea();
    } else if (k == 10) {
      if ((int)nt_editBuffer.length() < NT_MAX_NOTE_LEN) nt_editBuffer += '\n';
      nt_drawTextArea();
    } else if (k == 4) {
      nt_showPunctPicker();
    } else if (k != 0) {
      if ((int)nt_editBuffer.length() < NT_MAX_NOTE_LEN) nt_editBuffer += k;
      nt_drawTextArea();
    }
  }
}

// ── Public API used by main.ino / actions.c ────────────────────

void notes_setup() {
  // Touch calibration is already set once, globally, in main.ino —
  // no need to call tft.setTouch() here.
  if (!LittleFS.begin(true)) {
    Serial.println("LittleFS mount failed!");
    return;
  }
  if (!LittleFS.exists(NT_NOTES_DIR)) {
    LittleFS.mkdir(NT_NOTES_DIR);
  }
}

void notes_request_open() {
  notes_open_requested = true;
}

int notes_consume_open_request() {
  if (notes_open_requested) {
    notes_open_requested = false;
    return 1;
  }
  return 0;
}

void notes_open() {
  notes_active = true;

  // Splash (kept from original app, shown each time Notes opens)
  tft.fillScreen(NT_C_BG);
  tft.setTextColor(NT_C_ACCENT, NT_C_BG);
  tft.setTextSize(3);
  tft.setCursor(140, 120);
  tft.print("NoteApp");
  tft.setTextColor(NT_C_SUBTEXT, NT_C_BG);
  tft.setTextSize(1);
  tft.setCursor(168, 158);
  tft.print("ESP32 + ILI9488");
  delay(800);

  nt_currentScreen = NT_SCR_LIST;
  nt_loadNoteList();
  nt_listScroll = 0;
  nt_drawListScreen();
}

void notes_close() {
  notes_active = false;
  nt_numPanelOpen = false;
  // objects.main was already LVGL's active screen the whole time Notes was
  // open (we only drew over it with raw tft calls). Force a full repaint
  // so LVGL redraws over our raw drawing on the next lv_timer_handler().
  lv_obj_invalidate(objects.main);
}

void notes_loop() {
  if (!notes_active) return;

  static bool wasTouched = false;
  static unsigned long lastTouch = 0;

  nt_updateTouch();

  if (nt_touchValid && !wasTouched && (millis() - lastTouch > 200)) {
    wasTouched = true;
    lastTouch  = millis();

    switch (nt_currentScreen) {
      case NT_SCR_LIST: nt_handleListTouch(); break;
      case NT_SCR_EDIT: nt_handleEditTouch(); break;
      default: break;
    }
  }

  if (!nt_touchValid) wasTouched = false;
}

int notes_is_active() {
  return notes_active ? 1 : 0;
}
