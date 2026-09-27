#pragma once
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Runs the one-and-only touch calibration for the whole device.
// - If calibration was already done this boot -> returns instantly.
// - If calibration was done on a PREVIOUS boot -> loads it from flash (NVS)
//   and returns instantly, no need to touch the corners again.
// - Otherwise -> shows the calibration screen once, saves the result to
//   flash so it's never needed again on this board.
//
// Call this ONCE from main.ino's setup(), before ui_init(). Every future
// app (calculator, camera, notes, ...) just relies on this having already

void touch_calibration_init();

// 1 once calibration data is valid and applied to the tft object, else 0.
int touch_calibration_is_ready();

// Optional: copy the 5 raw calibration values out, e.g. for debugging.
int touch_calibration_get(uint16_t out[5]);

// Optional: wipe the saved calibration from flash and force a
// recalibration next boot. Not called anywhere by default.
void touch_calibration_reset();

#ifdef __cplusplus
}
#endif