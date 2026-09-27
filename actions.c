#include "actions.h"
#include "ui.h"
#include "email.h"

extern void calculator_request_open(void);
extern void chatbot_request_open(void);
extern void set_request_open(void);
extern void em_request_open(void);
extern void notes_request_open(void);
extern void gm_request_open(void);
extern void camera_request_open(void);
extern void time_request_open(void);
void action_go_to_page2(lv_event_t *e)
{
    lv_scr_load(objects.page1);
}

void action_go_to_main(lv_event_t *e)
{
    lv_scr_load(objects.main);
}

void action_open_calculator(lv_event_t * e){
    calculator_request_open();
}

void action_open_chatbot(lv_event_t * e){
    chatbot_request_open();
}

extern void action_open_camera(lv_event_t * e){
     camera_request_open();
}
extern void action_open_lo_ra(lv_event_t * e){

}
extern void action_open_notes(lv_event_t * e){
       notes_request_open();
}
extern void action_open_video_player(lv_event_t * e){

}
extern void action_open_settings(lv_event_t * e){
     set_request_open();
}
extern void action_open_game(lv_event_t * e){
    gm_request_open();
}
extern void action_open_music(lv_event_t * e){

}
extern void action_open_file_manager(lv_event_t * e){

}
extern void action_open_email(lv_event_t * e){
   em_request_open();
}
extern void action_open_clock(lv_event_t * e){
  time_request_open();
}
extern void action_open_contacts(lv_event_t * e){

}