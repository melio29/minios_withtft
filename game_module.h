
#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Games app for the miniOS launcher.
// Bundles Snake + Flappy Bird behind one picker menu.
//
// Lifecycle (mirrors touch_cal.h's single-init pattern, but per-open
// instead of per-boot):
//   1. Call gm_setup() ONCE from main.ino's setup(), after
//      touch_calibration_init() and ui_init(). This allocates the
//      Flappy sprite, seeds random, loads the Snake high score from
//      flash, and sets up the flap button pin. It does NOT touch the
//      screen.
//   2. When the user taps the "Games" icon on the OS home screen, call
//      gm_request_open() from that tap handler.
//   3. In main.ino's loop(), poll gm_consume_open_request() every
//      iteration. When it returns 1, call gm_open() to hand the screen
//      over to the games module (draws the Snake/Flappy picker).
//   4. While gm_is_active() returns 1, route loop() calls to gm_loop()
//      instead of the OS home-screen loop. Each game has its own
//      top-left "back" icon that returns to the picker; the picker
//      itself has an "EXIT" tap target that sets the module inactive
//      again, handing control back to main.ino.

// One-time init at boot. Allocates resources, does NOT draw anything.
void gm_setup();

// Opens the games module: shows the Snake / Flappy picker screen.
// Call this after gm_consume_open_request() returns 1.
void gm_open();

// Call every loop() iteration while gm_is_active() is true.
// Handles input, game state, and drawing for whichever screen
// (picker / Snake / Flappy) is currently showing.
void gm_loop();

// 1 while the games module owns the screen (picker or either game),
// 0 once the user has exited back to the OS. main.ino uses this to
// decide whether to keep calling gm_loop() or resume its own UI.
int gm_is_active();

// Call from the "Games" icon's tap handler to request opening the
// module. Just sets a flag — does not draw or block.
void gm_request_open();

// Polled from main.ino's loop(). Returns 1 exactly once per request
// (clears the flag on read), 0 otherwise.
int gm_consume_open_request();

#ifdef __cplusplus
}
#endif