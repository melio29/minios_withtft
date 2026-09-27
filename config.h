
#ifndef CONFIG_H
#define CONFIG_H 

// ================= WiFi =================
#define WIFI_SSID       "your_wifi_name"
#define WIFI_PASSWORD   "your_wifi_password"

// ================= Gmail account =================
// You MUST use a Google "App Password", not your normal Gmail password.
// 1. Turn on 2-Step Verification: myaccount.google.com/security
// 2. Create an App Password: myaccount.google.com/apppasswords
// 3. Paste the 16-character password below (no spaces).
#define GMAIL_EMAIL          "you_email"
#define GMAIL_APP_PASSWORD   "you_gmail_apppassword"

// ================= Mail servers (do not change) =================
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465
#define IMAP_HOST "imap.gmail.com"
#define IMAP_PORT 993

// ================= Touch calibration (NO LONGER USED) =================
// NOTE: touch calibration for the whole merged project is now handled
// ONCE, centrally, in main.ino (doSharedTouchCalibration()). The values
// below and CALIBRATE_MODE are left here only for reference / in case you
// ever run this email app as a standalone sketch again — email.ino itself
// does not read or use these anymore.
#define CALIBRATE_MODE false
static uint16_t calData[5] = {258, 3593, 143, 3500, 7};

#endif
