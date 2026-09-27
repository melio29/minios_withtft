#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_go_to_main(lv_event_t * e);
extern void action_go_to_page2(lv_event_t * e);
extern void action_open_calculator(lv_event_t * e);
extern void action_open_camera(lv_event_t * e);
extern void action_open_lo_ra(lv_event_t * e);
extern void action_open_notes(lv_event_t * e);
extern void action_open_video_player(lv_event_t * e);
extern void action_open_chatbot(lv_event_t * e);
extern void action_open_settings(lv_event_t * e);
extern void action_open_game(lv_event_t * e);
extern void action_open_music(lv_event_t * e);
extern void action_open_file_manager(lv_event_t * e);
extern void action_open_email(lv_event_t * e);
extern void action_open_clock(lv_event_t * e);
extern void action_open_contacts(lv_event_t * e);

#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/