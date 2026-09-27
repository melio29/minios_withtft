#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

// Screens

enum ScreensEnum {
    _SCREEN_ID_FIRST = 1,
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_PAGE1 = 2,
    _SCREEN_ID_LAST = 2
};

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *page1;
    lv_obj_t *obj0;
    lv_obj_t *wifi;
    lv_obj_t *battery_label;
    lv_obj_t *img_battery;
    lv_obj_t *time;
    lv_obj_t *bottom_bar;
    lv_obj_t *home;
    lv_obj_t *recent;
    lv_obj_t *back;
    lv_obj_t *dot_1;
    lv_obj_t *dot_2;
    lv_obj_t *dot_3;
    lv_obj_t *img_calci;
    lv_obj_t *img_cam;
    lv_obj_t *img_notes;
    lv_obj_t *img_video;
    lv_obj_t *img_lo_ra;
    lv_obj_t *img_chatbot;
    lv_obj_t *img_settings;
    lv_obj_t *img_game;
    lv_obj_t *calculator;
    lv_obj_t *camera;
    lv_obj_t *lo_ra;
    lv_obj_t *notes;
    lv_obj_t *video_player;
    lv_obj_t *chatbot;
    lv_obj_t *settings;
    lv_obj_t *games;
    lv_obj_t *img_forward;
    lv_obj_t *img_music_1;
    lv_obj_t *img_files_1;
    lv_obj_t *img_clock_1;
    lv_obj_t *img_email_1;
    lv_obj_t *music_1;
    lv_obj_t *files_manager_1;
    lv_obj_t *clock_1;
    lv_obj_t *email_1;
    lv_obj_t *obj1;
    lv_obj_t *wifi_1;
    lv_obj_t *time_1;
    lv_obj_t *img_battery_1;
    lv_obj_t *battery_label_1;
    lv_obj_t *bottom_bar_1;
    lv_obj_t *home_1;
    lv_obj_t *recent_1;
    lv_obj_t *back_1;
    lv_obj_t *dot_4;
    lv_obj_t *dot_5;
    lv_obj_t *dot_6;
    lv_obj_t *img_back;
    lv_obj_t *img_contacts;
    lv_obj_t *contacts;
} objects_t;

extern objects_t objects;

void create_screen_main();
void tick_screen_main();

void create_screen_page1();
void tick_screen_page1();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/