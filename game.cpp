

#include "game_module.h"
#include <TFT_eSPI.h>
#include <Preferences.h>

// Single tft instance lives in main.ino (same as touch_cal.cpp).
extern TFT_eSPI tft;


//  SHARED SCREEN CONSTANTS

#define SCR_W 480
#define SCR_H 320


//  SHARED TOUCH HELPERS (menu + both games use these)

struct Pt { int x, y; bool valid; };

static Pt gm_getTouch() {
  Pt p = {0, 0, false};
  uint16_t tx, ty;
  if (tft.getTouch(&tx, &ty, 600)) {
    p.x = (int)tx; p.y = (int)ty; p.valid = true;
  }
  return p;
}

static unsigned long s_lastTouchMs = 0;
static bool          s_wasTouched  = false;
#define TOUCH_DEBOUNCE 180

// A single shared "home" icon hit-box, top-left corner, used by both
// games and drawn identically in both so it always behaves the same.
#define HOME_BTN_X   6
#define HOME_BTN_Y   5
#define HOME_BTN_SZ  26
#define HOME_HIT_PAD 6

static bool touchOnHomeBtn(const Pt& p) {
  if (!p.valid) return false;
  return (p.x >= HOME_BTN_X - HOME_HIT_PAD &&
          p.x <= HOME_BTN_X + HOME_BTN_SZ + HOME_HIT_PAD &&
          p.y >= HOME_BTN_Y - HOME_HIT_PAD &&
          p.y <= HOME_BTN_Y + HOME_BTN_SZ + HOME_HIT_PAD);
}

// Draws the "◀" back icon directly onto a target (tft OR a sprite —

template <typename Canvas>
static void drawHomeIcon(Canvas& gfx, uint16_t bg, uint16_t fg, uint16_t accent) {
  gfx.fillRoundRect(HOME_BTN_X, HOME_BTN_Y, HOME_BTN_SZ, HOME_BTN_SZ, 4, bg);
  gfx.drawRoundRect(HOME_BTN_X, HOME_BTN_Y, HOME_BTN_SZ, HOME_BTN_SZ, 4, accent);
  // left-pointing chevron
  int cx = HOME_BTN_X + HOME_BTN_SZ / 2;
  int cy = HOME_BTN_Y + HOME_BTN_SZ / 2;
  gfx.fillTriangle(cx + 5, cy - 7, cx + 5, cy + 7, cx - 6, cy, fg);
}


//  MODULE STATE MACHINE

enum GmState { GM_CLOSED, GM_MENU, GM_SNAKE, GM_FLAPPY };
static GmState gmState = GM_CLOSED;
static bool    s_openRequested = false;

namespace SnakeGame  { void enter(); void loop(const Pt& p, bool fresh); }
namespace FlappyGame { void enter(); void loop(const Pt& p, bool fresh); }

static void gm_returnToMenu();   // fwd decl, used by both games' home icon


  //PICKER MENU

#define C_BG        0x0000
#define C_PANEL     0x0329
#define C_ACCENT    0x07FF
#define C_TEXT      0xFFFF
#define C_SUBTEXT   0x8410
#define C_SNAKE_C   0x07E0
#define C_FLAP_C    0xFFE0

// Card geometry (two side-by-side cards)
#define CARD_W   190
#define CARD_H   180
#define CARD_Y   90
#define CARD1_X  40
#define CARD2_X  (SCR_W - CARD1_X - CARD_W)

// EXIT tap target (top-right corner of the picker only)
#define EXIT_BTN_X   (SCR_W - 90)
#define EXIT_BTN_Y   8
#define EXIT_BTN_W   80
#define EXIT_BTN_H   28

static void drawMenu() {
  tft.fillScreen(C_BG);

  tft.setTextDatum(TL_DATUM);
  tft.setTextColor(C_ACCENT, C_BG);
  tft.setTextSize(3);
  tft.setCursor(30, 20);
  tft.print("GAMES");

  // Exit button
  tft.fillRoundRect(EXIT_BTN_X, EXIT_BTN_Y, EXIT_BTN_W, EXIT_BTN_H, 6, C_PANEL);
  tft.drawRoundRect(EXIT_BTN_X, EXIT_BTN_Y, EXIT_BTN_W, EXIT_BTN_H, 6, C_ACCENT);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(2);
  tft.setCursor(EXIT_BTN_X + 10, EXIT_BTN_Y + 6);
  tft.print("EXIT");

  // Snake card
  tft.fillRoundRect(CARD1_X, CARD_Y, CARD_W, CARD_H, 10, C_PANEL);
  tft.drawRoundRect(CARD1_X, CARD_Y, CARD_W, CARD_H, 10, C_ACCENT);
  // mini snake icon
  for (int i = 0; i < 5; i++) {
    uint16_t c = (i == 0) ? C_SNAKE_C : 0x03E0;
    tft.fillRoundRect(CARD1_X + 30 + i * 22, CARD_Y + 40, 18, 18, 3, c);
  }
  tft.fillCircle(CARD1_X + 30 + 5 * 22 + 9, CARD_Y + 49, 6, 0xF800);
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(2);
  tft.setCursor(CARD1_X + 45, CARD_Y + 100);
  tft.print("SNAKE");
  tft.setTextColor(C_SUBTEXT, C_PANEL);
  tft.setTextSize(1);
  tft.setCursor(CARD1_X + 22, CARD_Y + 130);
  tft.print("Tap to play");

  // Flappy card
  tft.fillRoundRect(CARD2_X, CARD_Y, CARD_W, CARD_H, 10, C_PANEL);
  tft.drawRoundRect(CARD2_X, CARD_Y, CARD_W, CARD_H, 10, C_ACCENT);
  // mini bird icon
  int bx = CARD2_X + CARD_W / 2 - 10, by = CARD_Y + 50;
  tft.fillCircle(bx, by, 13, C_FLAP_C);
  tft.fillCircle(bx + 5, by - 4, 3, C_BG);
  tft.fillTriangle(bx + 13, by, bx + 22, by - 5, bx + 22, by + 5, TFT_ORANGE);
  tft.fillRect(CARD2_X + 20, CARD_Y + 0, 30, 40, C_PANEL); // (kept minimal, no pipe deco to save draw time)
  tft.setTextColor(C_TEXT, C_PANEL);
  tft.setTextSize(2);
  tft.setCursor(CARD2_X + 15, CARD_Y + 100);
  tft.print("FLAPPY BIRD");
  tft.setTextColor(C_SUBTEXT, C_PANEL);
  tft.setTextSize(1);
  tft.setCursor(CARD2_X + 22, CARD_Y + 130);
  tft.print("Tap to play");
}

static void menuLoop(const Pt& p, bool fresh) {
  if (!fresh || !p.valid) return;

  if (p.x >= EXIT_BTN_X && p.x <= EXIT_BTN_X + EXIT_BTN_W &&
      p.y >= EXIT_BTN_Y && p.y <= EXIT_BTN_Y + EXIT_BTN_H) {
    gmState = GM_CLOSED;   // hands control back to main.ino's OS screen
    return;
  }

  if (p.x >= CARD1_X && p.x <= CARD1_X + CARD_W &&
      p.y >= CARD_Y  && p.y <= CARD_Y + CARD_H) {
    gmState = GM_SNAKE;
    SnakeGame::enter();
    return;
  }

  if (p.x >= CARD2_X && p.x <= CARD2_X + CARD_W &&
      p.y >= CARD_Y  && p.y <= CARD_Y + CARD_H) {
    gmState = GM_FLAPPY;
    FlappyGame::enter();
    return;
  }
}

static void gm_returnToMenu() {
  gmState = GM_MENU;
  drawMenu();
}


//  PUBLIC API

void gm_setup() {
  // Both games' one-time hardware/resource setup lives in their own
  // namespaces so this stays a thin dispatcher — see bottom of file.
  extern void SnakeGame_setup();
  extern void FlappyGame_setup();

  randomSeed(analogRead(34));  // shared RNG seed for both games' food/pipe spawns
  SnakeGame_setup();
  FlappyGame_setup();
}

void gm_open() {
  gmState = GM_MENU;
  drawMenu();
}

void gm_loop() {
  if (gmState == GM_CLOSED) return;

  Pt p = gm_getTouch();
  unsigned long now = millis();
  bool fresh = p.valid && !s_wasTouched && (now - s_lastTouchMs > TOUCH_DEBOUNCE);
  if (fresh) s_lastTouchMs = now;
  if (p.valid) s_wasTouched = true;
  else         s_wasTouched = false;

  switch (gmState) {
    case GM_MENU:   menuLoop(p, fresh); break;
    case GM_SNAKE:  SnakeGame::loop(p, fresh); break;
    case GM_FLAPPY: FlappyGame::loop(p, fresh); break;
    default: break;
  }
}

int gm_is_active() {
  return gmState != GM_CLOSED;
}

void gm_request_open() {
  s_openRequested = true;
}

int gm_consume_open_request() {
  if (!s_openRequested) return 0;
  s_openRequested = false;
  return 1;
}


//  SNAKE GAME  (adapted from the standalone sketch — same feel,
//  same full-width left/right touch turning, same pause icon.
//  Only change: wrapped into enter()/loop(), splash screen dropped
//  (the picker menu now serves that role), and a top-left "back to
//  menu" icon added next to the existing top-right pause icon.)


namespace SnakeGame {

#define S_C_BG          0x0000
#define S_C_GRID        0x0841
#define S_C_SNAKE_HEAD  0x07E0
#define S_C_SNAKE_BODY  0x03E0
#define S_C_SNAKE_OUT   0x0300
#define S_C_FOOD        0xF800
#define S_C_FOOD_SHINE  0xFD20
#define S_C_WALL        0x2945
#define S_C_WALL_HL     0x07FF
#define S_C_TEXT        0xFFFF
#define S_C_SCORE_BG    0x0329
#define S_C_ACCENT      0x07FF
#define S_C_GOLD        0xFFE0
#define S_C_PAUSE_BG    0x18C3
#define S_C_BTN_GREEN   0x03E0
#define S_C_BTN_RED     0x7800
#define S_C_SHADOW      0x2104
#define S_C_ACCENT_DIM  0x03EF
#define S_C_SUBTEXT     0x8410
#define S_C_DIVIDER     0x39C7

#define S_SCORE_BAR_H   36
#define S_WALL_T        4
#define S_GRID_X0       (S_WALL_T)
#define S_GRID_Y0       (S_SCORE_BAR_H + S_WALL_T)
#define S_GRID_X1       (SCR_W - S_WALL_T)
#define S_GRID_Y1       (SCR_H - S_WALL_T)
#define S_GRID_W        (S_GRID_X1 - S_GRID_X0)
#define S_GRID_H        (S_GRID_Y1 - S_GRID_Y0)
#define S_CELL          16
#define S_COLS          (S_GRID_W / S_CELL)
#define S_ROWS          (S_GRID_H / S_CELL)
#define S_GRID_OX       (S_GRID_X0 + (S_GRID_W - S_COLS * S_CELL) / 2)
#define S_GRID_OY       (S_GRID_Y0 + (S_GRID_H - S_ROWS * S_CELL) / 2)

#define S_ZONE_MID      (SCR_W / 2)

#define S_PAUSE_BTN_SZ   26
#define S_PAUSE_BTN_X    (SCR_W - S_PAUSE_BTN_SZ - 8)
#define S_PAUSE_BTN_Y    5
#define S_PAUSE_HIT_PAD  6

#define S_MAX_SNAKE     (S_COLS * S_ROWS)
#define S_DIR_UP    0
#define S_DIR_RIGHT 1
#define S_DIR_DOWN  2
#define S_DIR_LEFT  3

static const int S_SPEED_TABLE[] = { 220, 180, 140, 110, 80 };
#define S_MAX_LEVEL 4

enum SState { SS_PLAYING, SS_PAUSED, SS_OVER };
static SState sState = SS_PLAYING;

struct SCell { int8_t col, row; };
static SCell s_snake[S_MAX_SNAKE];
static int   s_snakeHead = 0;
static int   s_snakeLen  = 0;
static int   s_snakeDir  = S_DIR_RIGHT;
static int   s_nextDir   = S_DIR_RIGHT;
static SCell s_food;
static int   s_score = 0;
static int   s_highScore = 0;
static int   s_level = 0;
static unsigned long s_lastMoveMs = 0, s_lastFlashMs = 0;
static bool  s_foodVisible = true;
static Preferences s_prefs;

static int cellPx(int col) { return S_GRID_OX + col * S_CELL; }
static int cellPy(int row) { return S_GRID_OY + row * S_CELL; }
static SCell& snakeAt(int offset) {
  int idx = (s_snakeHead - offset + S_MAX_SNAKE) % S_MAX_SNAKE;
  return s_snake[idx];
}

static void drawPauseIcon() {
  tft.fillRoundRect(S_PAUSE_BTN_X, S_PAUSE_BTN_Y, S_PAUSE_BTN_SZ, S_PAUSE_BTN_SZ, 4, S_C_WALL);
  tft.drawRoundRect(S_PAUSE_BTN_X, S_PAUSE_BTN_Y, S_PAUSE_BTN_SZ, S_PAUSE_BTN_SZ, 4, S_C_ACCENT);
  tft.fillRect(S_PAUSE_BTN_X + 7,  S_PAUSE_BTN_Y + 6, 4, S_PAUSE_BTN_SZ - 12, S_C_TEXT);
  tft.fillRect(S_PAUSE_BTN_X + 15, S_PAUSE_BTN_Y + 6, 4, S_PAUSE_BTN_SZ - 12, S_C_TEXT);
}

static void drawBorder() {
  tft.fillRect(0, 0, SCR_W, S_SCORE_BAR_H, S_C_SCORE_BG);
  tft.fillRect(0, 0, 4, S_SCORE_BAR_H, S_C_ACCENT);
  tft.fillRect(0, S_SCORE_BAR_H, SCR_W, S_WALL_T, S_C_WALL);
  tft.fillRect(0, SCR_H - S_WALL_T, SCR_W, S_WALL_T, S_C_WALL);
  tft.fillRect(0, S_SCORE_BAR_H, S_WALL_T, SCR_H - S_SCORE_BAR_H, S_C_WALL);
  tft.fillRect(SCR_W - S_WALL_T, S_SCORE_BAR_H, S_WALL_T, SCR_H - S_SCORE_BAR_H, S_C_WALL);
  tft.drawFastHLine(S_WALL_T, S_SCORE_BAR_H + S_WALL_T, SCR_W - S_WALL_T * 2, S_C_WALL_HL);
  tft.drawFastHLine(S_WALL_T, S_SCORE_BAR_H + S_WALL_T + 1, SCR_W - S_WALL_T * 2, S_C_ACCENT_DIM);
}

static void drawScoreBar() {
  tft.fillRect(4, 0, SCR_W - 4, S_SCORE_BAR_H, S_C_SCORE_BG);
  tft.fillRect(0, 0, 4, S_SCORE_BAR_H, S_C_ACCENT);

  tft.setTextColor(S_C_ACCENT, S_C_SCORE_BG);
  tft.setTextSize(1);
  tft.setCursor(40, 6);   // shifted right +28 to clear the home icon
  tft.print("SCORE");
  tft.setTextColor(S_C_TEXT, S_C_SCORE_BG);
  tft.setTextSize(2);
  tft.setCursor(40, 18);
  char buf[8];
  sprintf(buf, "%05d", s_score);
  tft.print(buf);

  tft.setTextColor(S_C_GOLD, S_C_SCORE_BG);
  tft.setTextSize(1);
  tft.setCursor(140, 6);
  tft.print("BEST");
  tft.setTextColor(S_C_GOLD, S_C_SCORE_BG);
  tft.setTextSize(2);
  tft.setCursor(140, 18);
  sprintf(buf, "%05d", s_highScore);
  tft.print(buf);

  tft.setTextColor(S_C_ACCENT, S_C_SCORE_BG);
  tft.setTextSize(1);
  tft.setCursor(270, 6);
  tft.print("LEVEL");
  tft.setTextColor(S_C_TEXT, S_C_SCORE_BG);
  tft.setTextSize(2);
  tft.setCursor(270, 18);
  tft.print(s_level + 1);

  tft.setTextColor(S_C_SUBTEXT, S_C_SCORE_BG);
  tft.setTextSize(1);
  tft.setCursor(340, 6);
  tft.print("SPEED");
  for (int i = 0; i <= S_MAX_LEVEL; i++) {
    uint16_t c = (i <= s_level) ? S_C_ACCENT : S_C_DIVIDER;
    tft.fillRoundRect(340 + i * 16, 18, 12, 12, 2, c);
  }

  drawPauseIcon();
  drawHomeIcon(tft, S_C_WALL, S_C_TEXT, S_C_ACCENT);
}

static void drawGrid() {
  tft.fillRect(S_GRID_OX, S_GRID_OY, S_COLS * S_CELL, S_ROWS * S_CELL, S_C_BG);
  for (int c = 0; c <= S_COLS; c++)
    tft.drawFastVLine(S_GRID_OX + c * S_CELL, S_GRID_OY, S_ROWS * S_CELL, S_C_GRID);
  for (int r = 0; r <= S_ROWS; r++)
    tft.drawFastHLine(S_GRID_OX, S_GRID_OY + r * S_CELL, S_COLS * S_CELL, S_C_GRID);
}

static void drawSegment(int col, int row, bool isHead) {
  int px = cellPx(col), py = cellPy(row);
  int sz = S_CELL - 2;
  uint16_t bodyCol = isHead ? S_C_SNAKE_HEAD : S_C_SNAKE_BODY;

  tft.fillRoundRect(px + 2, py + 2, sz, sz, 3, S_C_SHADOW);
  tft.fillRoundRect(px + 1, py + 1, sz, sz, 3, bodyCol);
  tft.drawRoundRect(px + 1, py + 1, sz, sz, 3, S_C_SNAKE_OUT);

  if (isHead) {
    tft.fillRect(px + 3, py + 3, sz / 3, 2, 0xAFFF);
    int ex1, ey1, ex2, ey2;
    switch (s_snakeDir) {
      case S_DIR_RIGHT: ex1 = px + sz - 4; ey1 = py + 4; ex2 = px + sz - 4; ey2 = py + sz - 6; break;
      case S_DIR_LEFT:  ex1 = px + 3;      ey1 = py + 4; ex2 = px + 3;      ey2 = py + sz - 6; break;
      case S_DIR_UP:    ex1 = px + 4;      ey1 = py + 3; ex2 = px + sz - 6; ey2 = py + 3;      break;
      default:           ex1 = px + 4;      ey1 = py + sz - 4; ex2 = px + sz - 6; ey2 = py + sz - 4; break;
    }
    tft.fillCircle(ex1, ey1, 2, S_C_BG);
    tft.fillCircle(ex2, ey2, 2, S_C_BG);
    tft.fillCircle(ex1, ey1, 1, S_C_TEXT);
    tft.fillCircle(ex2, ey2, 1, S_C_TEXT);
  }
}

static void eraseSegment(int col, int row) {
  int px = cellPx(col), py = cellPy(row);
  tft.fillRect(px, py, S_CELL, S_CELL, S_C_BG);
  tft.drawFastVLine(px,          py, S_CELL, S_C_GRID);
  tft.drawFastVLine(px + S_CELL, py, S_CELL, S_C_GRID);
  tft.drawFastHLine(px, py,           S_CELL, S_C_GRID);
  tft.drawFastHLine(px, py + S_CELL,  S_CELL, S_C_GRID);
}

static void drawFood() {
  int px = cellPx(s_food.col) + S_CELL / 2;
  int py = cellPy(s_food.row) + S_CELL / 2;
  int r  = S_CELL / 2 - 2;
  tft.fillCircle(px, py, r, S_C_FOOD);
  tft.drawCircle(px, py, r, S_C_BTN_RED);
  tft.fillCircle(px - r / 3, py - r / 3, r / 4, S_C_FOOD_SHINE);
}
static void eraseFood() { eraseSegment(s_food.col, s_food.row); }

static bool cellOccupied(int col, int row) {
  for (int i = 0; i < s_snakeLen; i++) {
    SCell& c = snakeAt(i);
    if (c.col == col && c.row == row) return true;
  }
  return false;
}

static void spawnFood() {
  int tries = 0;
  do {
    s_food.col = random(0, S_COLS);
    s_food.row = random(0, S_ROWS);
    tries++;
  } while (cellOccupied(s_food.col, s_food.row) && tries < 200);
  s_foodVisible = true;
  drawFood();
}

static void initGame() {
  s_score = 0; s_level = 0; s_snakeLen = 4;
  s_snakeDir = S_DIR_RIGHT; s_nextDir = S_DIR_RIGHT;
  s_snakeHead = s_snakeLen - 1;

  int startCol = S_COLS / 2 - s_snakeLen / 2;
  int startRow = S_ROWS / 2;
  for (int i = 0; i < s_snakeLen; i++) {
    s_snake[i].col = startCol + i;
    s_snake[i].row = startRow;
  }

  drawGrid();
  for (int i = 0; i < s_snakeLen; i++)
    drawSegment(s_snake[i].col, s_snake[i].row, i == s_snakeHead);

  spawnFood();
  drawScoreBar();

  s_lastMoveMs = millis();
  s_lastFlashMs = millis();
  s_foodVisible = true;
  sState = SS_PLAYING;
}

static void drawGameOver() {
  tft.fillRect(S_GRID_OX + 40, S_GRID_OY + 30, S_COLS * S_CELL - 80, S_ROWS * S_CELL - 60, S_C_SCORE_BG);
  tft.drawRoundRect(S_GRID_OX + 40, S_GRID_OY + 30, S_COLS * S_CELL - 80, S_ROWS * S_CELL - 60, 10, S_C_ACCENT);

  int cx = SCR_W / 2, by = S_GRID_OY + 50;

  tft.setTextColor(S_C_SHADOW, S_C_SCORE_BG);
  tft.setTextSize(3);
  tft.setCursor(cx - 80, by + 2);
  tft.print("GAME OVER");
  tft.setTextColor(S_C_FOOD, S_C_SCORE_BG);
  tft.setCursor(cx - 82, by);
  tft.print("GAME OVER");

  tft.setTextColor(S_C_TEXT, S_C_SCORE_BG);
  tft.setTextSize(2);
  tft.setCursor(cx - 70, by + 40);
  tft.print("SCORE: ");
  tft.print(s_score);

  if (s_score >= s_highScore) {
    tft.setTextColor(S_C_GOLD, S_C_SCORE_BG);
    tft.setTextSize(1);
    tft.setCursor(cx - 48, by + 68);
    tft.print("** NEW HIGH SCORE! **");
  } else {
    tft.setTextColor(S_C_GOLD, S_C_SCORE_BG);
    tft.setTextSize(2);
    tft.setCursor(cx - 70, by + 64);
    tft.print("BEST:  ");
    tft.print(s_highScore);
  }

  tft.setTextColor(S_C_ACCENT, S_C_SCORE_BG);
  tft.setTextSize(1);
  tft.setCursor(cx - 40, by + 94);
  tft.print("Level reached: ");
  tft.print(s_level + 1);

  tft.fillRoundRect(cx - 80, by + 112, 160, 34, 7, S_C_BTN_GREEN);
  tft.drawRoundRect(cx - 80, by + 112, 160, 34, 7, S_C_ACCENT);
  tft.setTextColor(S_C_TEXT, S_C_BTN_GREEN);
  tft.setTextSize(2);
  tft.setCursor(cx - 66, by + 121);
  tft.print("PLAY AGAIN");
}

static void drawPause() {
  int cx = SCR_W / 2, cy = S_GRID_OY + S_ROWS * S_CELL / 2;
  tft.fillRoundRect(cx - 80, cy - 30, 160, 60, 8, S_C_PAUSE_BG);
  tft.drawRoundRect(cx - 80, cy - 30, 160, 60, 8, S_C_ACCENT);
  tft.setTextColor(S_C_ACCENT, S_C_PAUSE_BG);
  tft.setTextSize(2);
  tft.setCursor(cx - 46, cy - 18);
  tft.print("PAUSED");
  tft.setTextColor(S_C_SUBTEXT, S_C_PAUSE_BG);
  tft.setTextSize(1);
  tft.setCursor(cx - 70, cy + 8);
  tft.print("Tap pause icon to resume");
}

static void moveSnake() {
  s_snakeDir = s_nextDir;
  SCell oldHead = snakeAt(0);
  SCell newHead = oldHead;
  switch (s_snakeDir) {
    case S_DIR_UP:    newHead.row--; break;
    case S_DIR_DOWN:  newHead.row++; break;
    case S_DIR_LEFT:  newHead.col--; break;
    case S_DIR_RIGHT: newHead.col++; break;
  }

  if (newHead.col < 0 || newHead.col >= S_COLS || newHead.row < 0 || newHead.row >= S_ROWS) {
    if (s_score > s_highScore) { s_highScore = s_score; s_prefs.putInt("hi", s_highScore); }
    sState = SS_OVER;
    drawGameOver();
    return;
  }

  for (int i = 0; i < s_snakeLen - 1; i++) {
    SCell& c = snakeAt(i);
    if (c.col == newHead.col && c.row == newHead.row) {
      if (s_score > s_highScore) { s_highScore = s_score; s_prefs.putInt("hi", s_highScore); }
      sState = SS_OVER;
      drawGameOver();
      return;
    }
  }

  bool ate = (newHead.col == s_food.col && newHead.row == s_food.row);
  if (!ate) {
    SCell tail = snakeAt(s_snakeLen - 1);
    eraseSegment(tail.col, tail.row);
  }

  s_snakeHead = (s_snakeHead + 1) % S_MAX_SNAKE;
  s_snake[s_snakeHead] = newHead;

  if (ate) {
    s_snakeLen++;
    s_score += 10 * (s_level + 1);
    int newLevel = min((s_score / 50) / (s_level + 1), S_MAX_LEVEL);
    if (newLevel > s_level) s_level = newLevel;
    drawScoreBar();
    spawnFood();
  }

  drawSegment(oldHead.col, oldHead.row, false);
  drawSegment(newHead.col, newHead.row, true);
}

static void handleDirectionTouch(const Pt& p) {
  if (!p.valid) return;
  if (p.y < S_SCORE_BAR_H) return;

  if (p.x < S_ZONE_MID) {
    switch (s_snakeDir) {
      case S_DIR_UP:    s_nextDir = S_DIR_LEFT;  break;
      case S_DIR_LEFT:  s_nextDir = S_DIR_DOWN;  break;
      case S_DIR_DOWN:  s_nextDir = S_DIR_RIGHT; break;
      case S_DIR_RIGHT: s_nextDir = S_DIR_UP;    break;
    }
  } else {
    switch (s_snakeDir) {
      case S_DIR_UP:    s_nextDir = S_DIR_RIGHT; break;
      case S_DIR_RIGHT: s_nextDir = S_DIR_DOWN;  break;
      case S_DIR_DOWN:  s_nextDir = S_DIR_LEFT;  break;
      case S_DIR_LEFT:  s_nextDir = S_DIR_UP;    break;
    }
  }
}

static bool touchOnPauseBtn(const Pt& p) {
  if (!p.valid) return false;
  return (p.x >= S_PAUSE_BTN_X - S_PAUSE_HIT_PAD && p.x <= S_PAUSE_BTN_X + S_PAUSE_BTN_SZ + S_PAUSE_HIT_PAD &&
          p.y >= S_PAUSE_BTN_Y - S_PAUSE_HIT_PAD && p.y <= S_PAUSE_BTN_Y + S_PAUSE_BTN_SZ + S_PAUSE_HIT_PAD);
}

// ---- public (namespace-scope) entry points ----

void enter() {
  tft.fillScreen(S_C_BG);
  drawBorder();
  initGame();
}

void loop(const Pt& p, bool fresh) {
  unsigned long now = millis();

  if (sState == SS_OVER) {
    if (fresh) {
      if (touchOnHomeBtn(p)) { gm_returnToMenu(); return; }
      drawGrid();
      initGame();
    }
    return;
  }

  if (sState == SS_PAUSED) {
    if (fresh) {
      if (touchOnHomeBtn(p)) { gm_returnToMenu(); return; }
      if (touchOnPauseBtn(p)) {
        tft.fillRect(SCR_W / 2 - 82, S_GRID_OY + S_ROWS * S_CELL / 2 - 32, 164, 64, S_C_BG);
        drawBorder();
        drawGrid();
        for (int i = 0; i < s_snakeLen; i++)
          drawSegment(snakeAt(i).col, snakeAt(i).row, i == 0);
        drawFood();
        drawScoreBar();
        sState = SS_PLAYING;
        s_lastMoveMs = millis();
      }
    }
    return;
  }

  // SS_PLAYING
  if (fresh) {
    if (touchOnHomeBtn(p)) { gm_returnToMenu(); return; }
    if (touchOnPauseBtn(p)) {
      sState = SS_PAUSED;
      drawPause();
    } else {
      handleDirectionTouch(p);
    }
  }

  if (now - s_lastFlashMs > 400) {
    s_lastFlashMs = now;
    s_foodVisible = !s_foodVisible;
    if (s_foodVisible) drawFood(); else eraseFood();
  }

  int speed = S_SPEED_TABLE[min(s_level, S_MAX_LEVEL)];
  if (now - s_lastMoveMs >= (unsigned long)speed) {
    s_lastMoveMs = now;
    moveSnake();
  }
}

} // namespace SnakeGame

void SnakeGame_setup() {
  using namespace SnakeGame;
  s_prefs.begin("snake", false);
  s_highScore = s_prefs.getInt("hi", 0);
}


//  FLAPPY BIRD  (adapted from the standalone sketch — same
//  tuning, same sprite-based full-frame rendering. Only change:
//  wrapped into enter()/loop(), boot animation dropped (the
//  picker menu now serves that role), and a top-left "back to
//  menu" icon added, checked via touch alongside the existing
//  physical flap button.)

namespace FlappyGame {

#define BTN 40   // GPIO 40 — confirmed free (not TFT/touch/PSRAM pin)

static TFT_eSprite sprite(&tft);
static bool s_spriteReady = false;

static int   f_birdX = 60;
static float f_birdY = 160;
static float f_velocity = 0;

static float f_gravity   = 0.55f;
static float f_flapPower = -6.5f;
static int   f_pipeSpeed = 7;

#define F_NUM_PIPES 3
static int f_pipeSpacing = 220;
static int f_pipeWidth = 40;
static int f_gapSize = 90;

struct FPipe { int x; int gapY; bool scored; };
static FPipe f_pipes[F_NUM_PIPES];

static int  f_score = 0;
static bool f_gameOver = false;

static const unsigned long F_FRAME_INTERVAL_MS = 12;
static unsigned long f_lastFrameTime = 0;

static void drawBird() {
  sprite.fillCircle(f_birdX, (int)f_birdY, 13, TFT_YELLOW);
  sprite.fillCircle(f_birdX + 5, (int)f_birdY - 4, 3, TFT_BLACK);
  sprite.fillTriangle(f_birdX + 13, f_birdY, f_birdX + 22, f_birdY - 5, f_birdX + 22, f_birdY + 5, TFT_ORANGE);
}

static void drawPipe(const FPipe &p) {
  if (p.x + f_pipeWidth < 0 || p.x > SCR_W) return;
  sprite.fillRect(p.x, 0, f_pipeWidth, p.gapY, TFT_GREEN);
  sprite.fillRect(p.x - 3, p.gapY - 8, f_pipeWidth + 6, 8, TFT_DARKGREEN);
  sprite.fillRect(p.x, p.gapY + f_gapSize, f_pipeWidth, SCR_H - (p.gapY + f_gapSize), TFT_GREEN);
  sprite.fillRect(p.x - 3, p.gapY + f_gapSize, f_pipeWidth + 6, 8, TFT_DARKGREEN);
}

static int newGapY() { return random(60, 220); }

static void resetGame() {
  f_birdY = 160; f_velocity = 0; f_score = 0; f_gameOver = false;
  for (int i = 0; i < F_NUM_PIPES; i++) {
    f_pipes[i].x = SCR_W + i * f_pipeSpacing;
    f_pipes[i].gapY = newGapY();
    f_pipes[i].scored = false;
  }
}

static void drawGameOverOverlay() {
  sprite.setTextDatum(MC_DATUM);
  sprite.setTextColor(TFT_RED);
  sprite.setTextSize(3);
  sprite.drawString("GAME OVER", 240, 110);
  sprite.setTextColor(TFT_WHITE);
  sprite.setTextSize(2);
  sprite.drawString("SCORE : " + String(f_score), 240, 170);
  sprite.setTextColor(TFT_YELLOW);
  sprite.drawString("PRESS BUTTON", 240, 230);
  sprite.setTextColor(TFT_CYAN);
  sprite.setTextSize(1);
  sprite.drawString("Better Luck Next Time", 240, 300);
  sprite.setTextDatum(TL_DATUM);
}

static void drawFrame() {
  sprite.fillSprite(TFT_CYAN);

  sprite.fillCircle(60, 50, 12, TFT_WHITE);
  sprite.fillCircle(75, 45, 15, TFT_WHITE);
  sprite.fillCircle(90, 50, 12, TFT_WHITE);

  sprite.fillRect(0, 305, SCR_W, 15, TFT_BROWN);

  for (int i = 0; i < F_NUM_PIPES; i++) drawPipe(f_pipes[i]);
  drawBird();

  sprite.setTextColor(TFT_BLACK);
  sprite.setTextSize(2);
  sprite.setCursor(10, 10);
  sprite.print("Score: ");
  sprite.print(f_score);

  if (f_gameOver) drawGameOverOverlay();

  // top-left "back to menu" icon, drawn last so it stays on top
  drawHomeIcon(sprite, TFT_DARKGREY, TFT_WHITE, TFT_WHITE);

  sprite.pushSprite(0, 0);
}

// ---- public (namespace-scope) entry points ----

void enter() {
  if (!s_spriteReady) {
    // In case allocation somehow failed at boot, retry here rather
    // than silently drawing to a null sprite.
    sprite.setColorDepth(16);
    void* result = sprite.createSprite(SCR_W, SCR_H);
    s_spriteReady = (result != nullptr);
    if (!s_spriteReady) {
      tft.fillScreen(TFT_BLACK);
      tft.setTextColor(TFT_RED, TFT_BLACK);
      tft.setCursor(20, 140);
      tft.println("Flappy sprite alloc failed");
      tft.setCursor(20, 160);
      tft.println("(check PSRAM / OPI PSRAM in Tools menu)");
      return;
    }
  }
  resetGame();
  f_lastFrameTime = millis();
  drawFrame();
}

void loop(const Pt& p, bool fresh) {
  if (!s_spriteReady) return;  // enter() already reported the error on screen

  if (fresh && touchOnHomeBtn(p)) {
    gm_returnToMenu();
    return;
  }

  unsigned long now = millis();
  bool timeForFrame = (now - f_lastFrameTime) >= F_FRAME_INTERVAL_MS;

  if (f_gameOver) {
    if (digitalRead(BTN) == LOW) {
      delay(200);   // debounce
      resetGame();
    }
    if (timeForFrame) { f_lastFrameTime = now; drawFrame(); }
    return;
  }

  if (digitalRead(BTN) == LOW) f_velocity = f_flapPower;

  if (!timeForFrame) return;
  f_lastFrameTime = now;

  f_velocity += 2*(f_gravity);
  f_birdY += 2*(f_velocity);

  int rightmostX = -1;
  for (int i = 0; i < F_NUM_PIPES; i++) {
    f_pipes[i].x -= f_pipeSpeed;
    if (f_pipes[i].x > rightmostX) rightmostX = f_pipes[i].x;
  }
  for (int i = 0; i < F_NUM_PIPES; i++) {
    if (f_pipes[i].x < -f_pipeWidth) {
      f_pipes[i].x = rightmostX + f_pipeSpacing;
      f_pipes[i].gapY = newGapY();
      f_pipes[i].scored = false;
      rightmostX = f_pipes[i].x;
    }
  }

  if (f_birdY < 0 || f_birdY > 320) f_gameOver = true;

  for (int i = 0; i < F_NUM_PIPES; i++) {
    FPipe &p2 = f_pipes[i];
    if (f_birdX + 8 > p2.x && f_birdX - 8 < p2.x + f_pipeWidth) {
      if (f_birdY - 8 < p2.gapY || f_birdY + 8 > p2.gapY + f_gapSize) f_gameOver = true;
    }
    if (!p2.scored && p2.x + f_pipeWidth < f_birdX) { f_score++; p2.scored = true; }
  }

  drawFrame();
}

} // namespace FlappyGame

void FlappyGame_setup() {
  using namespace FlappyGame;
  pinMode(BTN, INPUT_PULLUP);

  sprite.setColorDepth(16);
  void* result = sprite.createSprite(SCR_W, SCR_H);
  s_spriteReady = (result != nullptr);
  if (!s_spriteReady) {
    Serial.println("Flappy sprite creation FAILED — check Tools > PSRAM is enabled (OPI PSRAM).");
  } else {
    Serial.println("Flappy sprite created successfully.");
  }
}