#ifndef EEZ_LVGL_UI_IMAGES_H
#define EEZ_LVGL_UI_IMAGES_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const lv_img_dsc_t img_imgwifi_on;
extern const lv_img_dsc_t img_imgwifi_off;
extern const lv_img_dsc_t img_bl_on;
extern const lv_img_dsc_t img_bl_off;
extern const lv_img_dsc_t img_battery;
extern const lv_img_dsc_t img_home_on;
extern const lv_img_dsc_t img_home_off;
extern const lv_img_dsc_t img_back_on;
extern const lv_img_dsc_t img_back_off;
extern const lv_img_dsc_t img_rec_on;
extern const lv_img_dsc_t img_rec_off;
extern const lv_img_dsc_t img_img_cam;
extern const lv_img_dsc_t img_img_file;
extern const lv_img_dsc_t img_img_calci;
extern const lv_img_dsc_t img_img_games;
extern const lv_img_dsc_t img_img_chatbot;
extern const lv_img_dsc_t img_img_music;
extern const lv_img_dsc_t img_img_notes;
extern const lv_img_dsc_t img_img_voltmeter;
extern const lv_img_dsc_t img_img_lo_ra;
extern const lv_img_dsc_t img_img_settings;
extern const lv_img_dsc_t img_img_video;
extern const lv_img_dsc_t img_img_stopwatch;
extern const lv_img_dsc_t img_img_clk;
extern const lv_img_dsc_t img_img_email;
extern const lv_img_dsc_t img_img_update;
extern const lv_img_dsc_t img_img_serimon;
extern const lv_img_dsc_t img_img_forward;
extern const lv_img_dsc_t img_img_back;
extern const lv_img_dsc_t img_img_contacts;

#ifndef EXT_IMG_DESC_T
#define EXT_IMG_DESC_T
typedef struct _ext_img_desc_t {
    const char *name;
    const lv_img_dsc_t *img_dsc;
} ext_img_desc_t;
#endif

extern const ext_img_desc_t images[30];

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_IMAGES_H*/