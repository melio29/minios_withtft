#include "touch_cal.h"
#include <TFT_eSPI.h>
#include <Preferences.h>
#include <string.h>

// Single tft instance lives in main.ino
extern TFT_eSPI tft;

static uint16_t s_calData[5] = { 0, 0, 0, 0, 0 };
static bool s_calibrated = false;

// Preferences/NVS is part of the standard ESP32 Arduino core and works
// identically on classic ESP32 and ESP32-S3 - nothing chip-specific here.
static const char *NVS_NS  = "touchcal";
static const char *NVS_KEY = "caldata";

static bool loadFromNVS()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NS, true)) return false; // read-only, ns may not exist yet

    bool ok = false;
    if (prefs.isKey(NVS_KEY) &&
        prefs.getBytesLength(NVS_KEY) == sizeof(s_calData))
    {
        prefs.getBytes(NVS_KEY, s_calData, sizeof(s_calData));
        ok = true;
    }
    prefs.end();
    return ok;
}

static void saveToNVS()
{
    Preferences prefs;
    if (!prefs.begin(NVS_NS, false)) return;
    prefs.putBytes(NVS_KEY, s_calData, sizeof(s_calData));
    prefs.end();
}

void touch_calibration_init()
{
    if (s_calibrated) return; // already done this boot

    // Reuse a calibration saved on a previous boot, if we have one.
    if (loadFromNVS())
    {
        tft.setTouch(s_calData);
        s_calibrated = true;
        return;
    }

    // First-ever boot on this board: calibrate once and persist it.
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(10, 140);
    tft.setTextColor(TFT_WHITE);
    tft.println("Touch Corner to calibrate");
    tft.calibrateTouch(s_calData, TFT_GREEN, TFT_BLACK, 15);

    tft.setTouch(s_calData);
    saveToNVS();
    s_calibrated = true;
}

int touch_calibration_is_ready()
{
    return s_calibrated ? 1 : 0;
}

int touch_calibration_get(uint16_t out[5])
{
    if (!s_calibrated) return 0;
    memcpy(out, s_calData, sizeof(s_calData));
    return 1;
}

void touch_calibration_reset()
{
    Preferences prefs;
    if (prefs.begin(NVS_NS, false))
    {
        prefs.remove(NVS_KEY);
        prefs.end();
    }
    s_calibrated = false;
}