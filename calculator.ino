// #include <TFT_eSPI.h>
// #include <math.h>
// #include "ui.h"      // for objects.main
// #include <lvgl.h>    // for lv_scr_load_anim

// extern TFT_eSPI tft;  // the single tft instance created in main.ino

// // These four functions are called from actions.c (plain C) and main.ino (C++).
// // extern "C" gives them C linkage so actions.c's compiled call matches
// // this file's compiled definition.
// extern "C" {
//     void calculator_setup();
//     void calculator_open();
//     void calculator_loop();
//     int  calculator_is_active();
//     void calculator_request_open();
//     int  calculator_consume_open_request();
// }

// #define BUTTON_W 120
// #define BUTTON_H 43
// #define BUTTON_ROWS 6
// #define BUTTON_COLS 4

// const char *calc_labels[BUTTON_ROWS][BUTTON_COLS] = {
//   { "sin", "cos", "tan", "sqrt" },
//   { "log", "exp", "C", "<" },
//   { "7", "8", "9", "/" },
//   { "4", "5", "6", "*" },
//   { "1", "2", "3", "-" },
//   { "0", ".", "=", "+" }
// };

// TFT_eSPI_Button calc_buttons[BUTTON_ROWS][BUTTON_COLS];
// TFT_eSPI_Button calc_backButton;
// String calc_input = "";

// uint16_t calc_calData[5];
// bool calc_calibrated = false;
// bool calc_active = false;
// static bool calc_open_requested = false;

// void calc_drawButtons() {
//   int x_offset = 0;
//   int y_offset = 60;

//   for (int r = 0; r < BUTTON_ROWS; r++) {
//     for (int c = 0; c < BUTTON_COLS; c++) {
//       int x = c * BUTTON_W + x_offset;
//       int y = r * BUTTON_H + y_offset;
//       calc_buttons[r][c].initButton(&tft, x + BUTTON_W / 2, y + BUTTON_H / 2,
//                                BUTTON_W - 4, BUTTON_H - 4,
//                                TFT_WHITE, TFT_DARKGREY, TFT_GREEN,
//                                (char *)calc_labels[r][c], 1);
//       calc_buttons[r][c].drawButton();
//     }
//   }
// }

// void calc_drawBackButton() {
//   // Sits in the header strip (0..60px tall), left side, clear of the display text
//   calc_backButton.initButton(&tft, 35, 30, 60, 50,
//                          TFT_WHITE, TFT_RED, TFT_WHITE,
//                          (char *)"BACK", 1);
//   calc_backButton.drawButton();
// }

// void calc_updateDisplay() {
//   // Only clear the display area to the right of the Back button (x >= 70)
//   tft.fillRect(70, 0, 410, 60, TFT_BLACK);
//   tft.setTextDatum(TR_DATUM);
//   tft.setTextColor(TFT_GREEN, TFT_BLACK);
//   tft.setFreeFont(&FreeSansBold9pt7b);
//   tft.drawString(calc_input, 475, 35);
// }

// float calc_evalExpression(String expr) {
//   expr.trim();

//   if (expr.startsWith("sin")) return sin(radians(expr.substring(3).toFloat()));
//   if (expr.startsWith("cos")) return cos(radians(expr.substring(3).toFloat()));
//   if (expr.startsWith("tan")) return tan(radians(expr.substring(3).toFloat()));
//   if (expr.startsWith("sqrt")) return sqrt(expr.substring(4).toFloat());
//   if (expr.startsWith("log")) return log10(expr.substring(3).toFloat());
//   if (expr.startsWith("exp")) return exp(expr.substring(3).toFloat());

//   char op = 0;
//   int idx = -1;
//   for (unsigned int i = 0; i < expr.length(); i++) {
//     if (strchr("+-*/", expr[i])) {
//       op = expr[i];
//       idx = i;
//       break;
//     }
//   }

//   if (idx != -1 && idx > 0) {
//     float a = expr.substring(0, idx).toFloat();
//     float b = expr.substring(idx + 1).toFloat();
//     switch (op) {
//       case '+': return a + b;
//       case '-': return a - b;
//       case '*': return a * b;
//       case '/': return b != 0 ? a / b : 0;
//     }
//   }
//   return expr.toFloat();
// }

// void calc_touchCalibration() {
//   uint16_t calDataTemp[5];
//   tft.fillScreen(TFT_BLACK);
//   tft.setCursor(10, 140);
//   tft.setTextColor(TFT_WHITE);
//   tft.println("Touch Corner to calibrate");
//   tft.calibrateTouch(calDataTemp, TFT_GREEN, TFT_BLACK, 15);
//   for (int i = 0; i < 5; i++) calc_calData[i] = calDataTemp[i];
//   tft.setTouch(calc_calData);
// }

// void calc_logo() {
//   tft.fillScreen(TFT_RED);

//   int centerX = 240;
//   int centerY = 160;
//   int radius = 70;
//   int thickness = 7;

//   for (int r = radius - thickness / 2; r <= radius + thickness / 2; r++) {
//     tft.drawCircle(centerX, centerY, r, TFT_BLACK);
//   }

//   tft.setTextColor(TFT_BLACK, TFT_RED);
//   tft.setTextDatum(MC_DATUM);

//   tft.setFreeFont(&FreeSansBold24pt7b);
//   tft.drawString("DSN", centerX, centerY);

//   tft.setFreeFont(&FreeSansBold9pt7b);
//   tft.drawString("Engineering Calculator", centerX, 280);
// }

// /* ---------- Functions called from main.ino / actions.ino ---------- */

// void calculator_setup() {
//   if (!calc_calibrated) {
//     // One-time touch calibration, same as your original sketch's setup()
//     calc_touchCalibration();
//     calc_calibrated = true;
//   }
// }

// void calculator_open() {
//   calc_active = true;
//   calc_input = "";

//   calc_logo();
//   delay(1200);

//   tft.setTouch(calc_calData);
//   tft.setFreeFont(&FreeSansBold9pt7b);
//   tft.fillScreen(TFT_BLACK);

//   calc_drawBackButton();
//   calc_updateDisplay();
//   calc_drawButtons();
// }

// void calculator_close() {
//   calc_active = false;
//   // objects.main was already LVGL's active screen the whole time the
//   // calculator was open (we only drew over it with raw tft calls, we
//   // never told LVGL to switch away). So lv_scr_load_anim(objects.main, ...)
//   // would be a no-op here — LVGL thinks nothing changed. Force a full
//   // repaint instead so LVGL actually redraws over our raw drawing.
//   lv_obj_invalidate(objects.main);
// }

// void calculator_loop() {
//   if (!calc_active) return;

//   uint16_t x, y;
//   if (tft.getTouch(&x, &y)) {

//     if (calc_backButton.contains(x, y)) {
//       calculator_close();
//       return;
//     }

//     for (int r = 0; r < BUTTON_ROWS; r++) {
//       for (int c = 0; c < BUTTON_COLS; c++) {
//         if (calc_buttons[r][c].contains(x, y)) {
//           String label = calc_labels[r][c];
//           if (label == "=") {
//             float result = calc_evalExpression(calc_input);
//             calc_input = String(result);
//           } else if (label == "C") {
//             calc_input = "";
//           } else if (label == "<") {
//             if (calc_input.length() > 0) calc_input.remove(calc_input.length() - 1);
//           } else {
//             calc_input += label;
//           }
//           calc_updateDisplay();
//           delay(200);
//         }
//       }
//     }
//   }
// }

// int calculator_is_active() {
//   return calc_active ? 1 : 0;
// }

// // Called from action_open_calculator (fires mid lv_timer_handler()). We only
// // set a flag here — actually drawing now would get overwritten by LVGL's
// // own flush that happens right after the click event finishes processing.
// void calculator_request_open() {
//   calc_open_requested = true;
// }

// // Called from main.ino's loop(), AFTER lv_timer_handler() has fully
// // finished its current pass. Safe to draw over the screen now.
// int calculator_consume_open_request() {
//   if (calc_open_requested) {
//     calc_open_requested = false;
//     return 1;
//   }
//   return 0;
// }

#include <TFT_eSPI.h>
#include <math.h>
#include "ui.h"
#include <lvgl.h>
#include "touch_cal.h"

extern TFT_eSPI tft;

extern "C" {
    void calculator_open();
    void calculator_loop();
    int  calculator_is_active();
    void calculator_request_open();
    int  calculator_consume_open_request();
}

#define BUTTON_W 120
#define BUTTON_H 43
#define BUTTON_ROWS 6
#define BUTTON_COLS 4

const char *calc_labels[BUTTON_ROWS][BUTTON_COLS] = {
  { "sin", "cos", "tan", "sqrt" },
  { "log", "exp", "C", "<" },
  { "7", "8", "9", "/" },
  { "4", "5", "6", "*" },
  { "1", "2", "3", "-" },
  { "0", ".", "=", "+" }
};

TFT_eSPI_Button calc_buttons[BUTTON_ROWS][BUTTON_COLS];
TFT_eSPI_Button calc_backButton;
String calc_input = "";

bool calc_active = false;
static bool calc_open_requested = false;

void calc_drawButtons() {
  int x_offset = 0;
  int y_offset = 60;

  for (int r = 0; r < BUTTON_ROWS; r++) {
    for (int c = 0; c < BUTTON_COLS; c++) {
      int x = c * BUTTON_W + x_offset;
      int y = r * BUTTON_H + y_offset;
      calc_buttons[r][c].initButton(&tft, x + BUTTON_W / 2, y + BUTTON_H / 2,
                               BUTTON_W - 4, BUTTON_H - 4,
                               TFT_WHITE, TFT_DARKGREY, TFT_GREEN,
                               (char *)calc_labels[r][c], 1);
      calc_buttons[r][c].drawButton();
    }
  }
}

void calc_drawBackButton() {
  calc_backButton.initButton(&tft, 35, 30, 60, 50,
                         TFT_WHITE, TFT_RED, TFT_WHITE,
                         (char *)"BACK", 1);
  calc_backButton.drawButton();
}

void calc_updateDisplay() {
  tft.fillRect(70, 0, 410, 60, TFT_BLACK);
  tft.setTextDatum(TR_DATUM);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.drawString(calc_input, 475, 35);
}

float calc_evalExpression(String expr) {
  expr.trim();

  if (expr.startsWith("sin")) return sin(radians(expr.substring(3).toFloat()));
  if (expr.startsWith("cos")) return cos(radians(expr.substring(3).toFloat()));
  if (expr.startsWith("tan")) return tan(radians(expr.substring(3).toFloat()));
  if (expr.startsWith("sqrt")) return sqrt(expr.substring(4).toFloat());
  if (expr.startsWith("log")) return log10(expr.substring(3).toFloat());
  if (expr.startsWith("exp")) return exp(expr.substring(3).toFloat());

  char op = 0;
  int idx = -1;
  for (unsigned int i = 0; i < expr.length(); i++) {
    if (strchr("+-*/", expr[i])) {
      op = expr[i];
      idx = i;
      break;
    }
  }

  if (idx != -1 && idx > 0) {
    float a = expr.substring(0, idx).toFloat();
    float b = expr.substring(idx + 1).toFloat();
    switch (op) {
      case '+': return a + b;
      case '-': return a - b;
      case '*': return a * b;
      case '/': return b != 0 ? a / b : 0;
    }
  }
  return expr.toFloat();
}

void calc_logo() {
  tft.fillScreen(TFT_RED);

  int centerX = 240;
  int centerY = 160;
  int radius = 70;
  int thickness = 7;

  for (int r = radius - thickness / 2; r <= radius + thickness / 2; r++) {
    tft.drawCircle(centerX, centerY, r, TFT_BLACK);
  }

  tft.setTextColor(TFT_BLACK, TFT_RED);
  tft.setTextDatum(MC_DATUM);
  tft.setFreeFont(&FreeSansBold24pt7b);
  tft.drawString("DSN", centerX, centerY);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.drawString("Engineering Calculator", centerX, 280);
}

/* ---------- Functions called from main.ino / actions.c ---------- */

void calculator_open() {
  // Touch calibration is already done once, globally, in main.ino's
  // setup(). Nothing to calibrate here anymore - just bail out safely
  // if it somehow wasn't ready yet.
  if (!touch_calibration_is_ready()) return;

  calc_active = true;
  calc_input = "";

  calc_logo();
  delay(1200);

  tft.setFreeFont(&FreeSansBold9pt7b);
  tft.fillScreen(TFT_BLACK);

  calc_drawBackButton();
  calc_updateDisplay();
  calc_drawButtons();
}

void calculator_close() {
  calc_active = false;
  lv_obj_invalidate(objects.main);
}

void calculator_loop() {
  if (!calc_active) return;

  uint16_t x, y;
  if (tft.getTouch(&x, &y)) {

    if (calc_backButton.contains(x, y)) {
      calculator_close();
      return;
    }

    for (int r = 0; r < BUTTON_ROWS; r++) {
      for (int c = 0; c < BUTTON_COLS; c++) {
        if (calc_buttons[r][c].contains(x, y)) {
          String label = calc_labels[r][c];
          if (label == "=") {
            float result = calc_evalExpression(calc_input);
            calc_input = String(result);
          } else if (label == "C") {
            calc_input = "";
          } else if (label == "<") {
            if (calc_input.length() > 0) calc_input.remove(calc_input.length() - 1);
          } else {
            calc_input += label;
          }
          calc_updateDisplay();
          delay(200);
        }
      }
    }
  }
}

int calculator_is_active() {
  return calc_active ? 1 : 0;
}

void calculator_request_open() {
  calc_open_requested = true;
}

int calculator_consume_open_request() {
  if (calc_open_requested) {
    calc_open_requested = false;
    return 1;
  }
  return 0;
}
