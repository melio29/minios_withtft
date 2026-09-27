#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;

//
// Event handlers
//

lv_obj_t *tick_value_change_obj;

//
// Screens
//

void create_screen_main() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_go_to_page2, LV_EVENT_CLICKED, (void *)0);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x0b1220), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 32);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x101a2e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x101a2e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // wifi
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.wifi = obj;
            lv_obj_set_pos(obj, 342, 2);
            lv_obj_set_size(obj, 28, 28);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_imgwifi_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_imgwifi_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_imgwifi_on, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_imgwifi_on, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            lv_obj_set_pos(obj, 378, 2);
            lv_obj_set_size(obj, 28, 28);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_bl_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_bl_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_bl_on, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_bl_on, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            lv_obj_set_pos(obj, 201, 141);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
        }
        {
            // battery_label
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.battery_label = obj;
            lv_obj_set_pos(obj, 409, 8);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xe5e7eb), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 10193, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "70%");
        }
        {
            // img_battery
            lv_obj_t *obj = lv_img_create(parent_obj);
            objects.img_battery = obj;
            lv_obj_set_pos(obj, 440, 1);
            lv_obj_set_size(obj, 30, 30);
            lv_img_set_src(obj, &img_battery);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE|LV_OBJ_FLAG_CLICKABLE);
        }
        {
            // time
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.time = obj;
            lv_obj_set_pos(obj, 10, 8);
            lv_obj_set_size(obj, 70, 19);
            lv_label_set_recolor(obj, true);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xe5e7eb), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "12.30 PM");
        }
        {
            // bottom_bar
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.bottom_bar = obj;
            lv_obj_set_pos(obj, 0, 270);
            lv_obj_set_size(obj, 480, 50);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x101a2e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // home
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.home = obj;
            lv_obj_set_pos(obj, 223, 277);
            lv_obj_set_size(obj, 35, 35);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_home_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_home_on, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_home_on, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // recent
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.recent = obj;
            lv_obj_set_pos(obj, 60, 277);
            lv_obj_set_size(obj, 37, 35);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_rec_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_rec_on, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_rec_on, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // back
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.back = obj;
            lv_obj_set_pos(obj, 372, 277);
            lv_obj_set_size(obj, 40, 35);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_back_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_back_on, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_back_on, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // dot_1
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.dot_1 = obj;
            lv_obj_set_pos(obj, 234, 260);
            lv_obj_set_size(obj, 6, 6);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x4b5563), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // dot_2
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.dot_2 = obj;
            lv_obj_set_pos(obj, 252, 260);
            lv_obj_set_size(obj, 6, 6);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x4b5563), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // dot_3
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.dot_3 = obj;
            lv_obj_set_pos(obj, 217, 260);
            lv_obj_set_size(obj, 6, 6);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x3b82f6), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // img_calci
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_calci = obj;
            lv_obj_set_pos(obj, 27, 53);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_calci, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_calci, NULL);
            lv_obj_add_event_cb(obj, action_open_calculator, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // img_cam
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_cam = obj;
            lv_obj_set_pos(obj, 153, 53);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_cam, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_cam, NULL);
            lv_obj_add_event_cb(obj, action_open_camera, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // img_notes
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_notes = obj;
            lv_obj_set_pos(obj, 391, 53);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_notes, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_notes, NULL);
            lv_obj_add_event_cb(obj, action_open_notes, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // img_video
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_video = obj;
            lv_obj_set_pos(obj, 28, 160);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_video, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_video, NULL);
            lv_obj_add_event_cb(obj, action_open_video_player, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // img_LoRa
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_lo_ra = obj;
            lv_obj_set_pos(obj, 278, 53);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_lo_ra, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_lo_ra, NULL);
            lv_obj_add_event_cb(obj, action_open_lo_ra, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // img_chatbot
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_chatbot = obj;
            lv_obj_set_pos(obj, 153, 160);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_chatbot, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_chatbot, NULL);
            lv_obj_add_event_cb(obj, action_open_chatbot, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // img_settings
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_settings = obj;
            lv_obj_set_pos(obj, 278, 160);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_settings, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_settings, NULL);
            lv_obj_add_event_cb(obj, action_open_settings, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // img_game
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_game = obj;
            lv_obj_set_pos(obj, 392, 160);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_games, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_games, NULL);
            lv_obj_add_event_cb(obj, action_open_game, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // calculator
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.calculator = obj;
            lv_obj_set_pos(obj, 31, 125);
            lv_obj_set_size(obj, 64, 16);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Calculator");
        }
        {
            // Camera
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.camera = obj;
            lv_obj_set_pos(obj, 159, 125);
            lv_obj_set_size(obj, 64, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Camera");
        }
        {
            // LoRa
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.lo_ra = obj;
            lv_obj_set_pos(obj, 293, 125);
            lv_obj_set_size(obj, 35, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "LoRa");
        }
        {
            // notes
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.notes = obj;
            lv_obj_set_pos(obj, 409, 125);
            lv_obj_set_size(obj, 35, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Text");
        }
        {
            // video_player
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.video_player = obj;
            lv_obj_set_pos(obj, 22, 233);
            lv_obj_set_size(obj, 82, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Video Player");
        }
        {
            // chatbot
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.chatbot = obj;
            lv_obj_set_pos(obj, 159, 233);
            lv_obj_set_size(obj, 54, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Chatbot");
        }
        {
            // Settings
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.settings = obj;
            lv_obj_set_pos(obj, 286, 233);
            lv_obj_set_size(obj, 50, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Settings");
        }
        {
            // Games
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.games = obj;
            lv_obj_set_pos(obj, 404, 233);
            lv_obj_set_size(obj, 51, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Games");
        }
        {
            // img_forward
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_forward = obj;
            lv_obj_set_pos(obj, 456, 126);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, 20);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_forward, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_forward, NULL);
            lv_obj_add_event_cb(obj, action_go_to_page2, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
}

void create_screen_page1() {
    lv_obj_t *obj = lv_obj_create(0);
    objects.page1 = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 480, 320);
    lv_obj_add_event_cb(obj, action_go_to_main, LV_EVENT_CLICKED, (void *)0);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x0b1220), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_row(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_column(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    {
        lv_obj_t *parent_obj = obj;
        {
            // img_music_1
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_music_1 = obj;
            lv_obj_set_pos(obj, 39, 53);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_music, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_music, NULL);
            lv_obj_add_event_cb(obj, action_open_music, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // img_files_1
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_files_1 = obj;
            lv_obj_set_pos(obj, 154, 53);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_file, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_file, NULL);
            lv_obj_add_event_cb(obj, action_open_file_manager, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // img_clock_1
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_clock_1 = obj;
            lv_obj_set_pos(obj, 386, 53);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_clk, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_clk, NULL);
            lv_obj_add_event_cb(obj, action_open_clock, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // img_email_1
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_email_1 = obj;
            lv_obj_set_pos(obj, 275, 53);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_email, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_email, NULL);
            lv_obj_add_event_cb(obj, action_open_email, LV_EVENT_CLICKED, (void *)0);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_set_style_align(obj, LV_ALIGN_TOP_LEFT, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // Music_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.music_1 = obj;
            lv_obj_set_pos(obj, 53, 124);
            lv_obj_set_size(obj, 36, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Music");
        }
        {
            // files_manager_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.files_manager_1 = obj;
            lv_obj_set_pos(obj, 147, 124);
            lv_obj_set_size(obj, 79, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "File Manager");
        }
        {
            // clock_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.clock_1 = obj;
            lv_obj_set_pos(obj, 398, 123);
            lv_obj_set_size(obj, 36, 16);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Clock");
        }
        {
            // email_1
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.email_1 = obj;
            lv_obj_set_pos(obj, 288, 124);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "E-mail");
        }
        {
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.obj1 = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 480, 32);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x101a2e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_color(obj, lv_color_hex(0x101a2e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    // wifi_1
                    lv_obj_t *obj = lv_imgbtn_create(parent_obj);
                    objects.wifi_1 = obj;
                    lv_obj_set_pos(obj, 322, -14);
                    lv_obj_set_size(obj, 28, 28);
                    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_imgwifi_off, NULL);
                    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_imgwifi_off, NULL);
                    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_imgwifi_on, NULL);
                    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_imgwifi_on, NULL);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                }
                {
                    lv_obj_t *obj = lv_imgbtn_create(parent_obj);
                    lv_obj_set_pos(obj, 351, -13);
                    lv_obj_set_size(obj, 28, 28);
                    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_bl_off, NULL);
                    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_bl_off, NULL);
                    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_bl_on, NULL);
                    lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_bl_on, NULL);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                }
                {
                    // time_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.time_1 = obj;
                    lv_obj_set_pos(obj, -10, -10);
                    lv_obj_set_size(obj, 70, 19);
                    lv_label_set_recolor(obj, true);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xe5e7eb), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "12.30 PM");
                }
                {
                    // img_battery_1
                    lv_obj_t *obj = lv_img_create(parent_obj);
                    objects.img_battery_1 = obj;
                    lv_obj_set_pos(obj, 411, -14);
                    lv_obj_set_size(obj, 30, 30);
                    lv_img_set_src(obj, &img_battery);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE|LV_OBJ_FLAG_CLICKABLE);
                }
                {
                    // battery_label_1
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    objects.battery_label_1 = obj;
                    lv_obj_set_pos(obj, 382, -8);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
                    lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_color(obj, lv_color_hex(0xe5e7eb), LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_outline_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_width(obj, 10193, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text_static(obj, "70%");
                }
            }
        }
        {
            // bottom_bar_1
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.bottom_bar_1 = obj;
            lv_obj_set_pos(obj, 0, 270);
            lv_obj_set_size(obj, 480, 50);
            lv_obj_set_style_text_color(obj, lv_color_hex(0x212121), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x101a2e), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // home_1
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.home_1 = obj;
            lv_obj_set_pos(obj, 223, 278);
            lv_obj_set_size(obj, 35, 35);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_home_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_home_on, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_home_on, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // recent_1
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.recent_1 = obj;
            lv_obj_set_pos(obj, 60, 277);
            lv_obj_set_size(obj, 37, 35);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_rec_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_rec_on, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_rec_on, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // back_1
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.back_1 = obj;
            lv_obj_set_pos(obj, 372, 277);
            lv_obj_set_size(obj, 40, 35);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_back_off, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_RELEASED, NULL, &img_back_on, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_CHECKED_PRESSED, NULL, &img_back_on, NULL);
            lv_obj_add_flag(obj, LV_OBJ_FLAG_CHECKABLE);
        }
        {
            // dot_4
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.dot_4 = obj;
            lv_obj_set_pos(obj, 255, 253);
            lv_obj_set_size(obj, 6, 6);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x4b5563), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // dot_5
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.dot_5 = obj;
            lv_obj_set_pos(obj, 222, 253);
            lv_obj_set_size(obj, 6, 6);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x4b5563), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // dot_6
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.dot_6 = obj;
            lv_obj_set_pos(obj, 238, 253);
            lv_obj_set_size(obj, 6, 6);
            lv_obj_set_style_align(obj, LV_ALIGN_DEFAULT, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_width(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_height(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0x3b82f6), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // img_back
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_back = obj;
            lv_obj_set_pos(obj, 9, 132);
            lv_obj_set_size(obj, 20, 20);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_back, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_back, NULL);
            lv_obj_add_event_cb(obj, action_go_to_main, LV_EVENT_CLICKED, (void *)0);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // img_contacts
            lv_obj_t *obj = lv_imgbtn_create(parent_obj);
            objects.img_contacts = obj;
            lv_obj_set_pos(obj, 39, 160);
            lv_obj_set_size(obj, 64, 64);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_RELEASED, NULL, &img_img_contacts, NULL);
            lv_imgbtn_set_src(obj, LV_IMGBTN_STATE_PRESSED, NULL, &img_img_contacts, NULL);
            lv_obj_add_event_cb(obj, action_open_contacts, LV_EVENT_CLICKED, (void *)0);
        }
        {
            // Contacts
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.contacts = obj;
            lv_obj_set_pos(obj, 44, 230);
            lv_obj_set_size(obj, 64, 16);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(obj, lv_color_hex(0xffffff), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text_static(obj, "Contacts");
        }
    }
    
    tick_screen_page1();
}

void tick_screen_page1() {
}

typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_page1,
};
void tick_screen(int screen_index) {
    if (screen_index >= 0 && screen_index < 2) {
        tick_screen_funcs[screen_index]();
    }
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen(screenId - 1);
}

//
// Fonts
//

ext_font_desc_t fonts[] = {
    { "monsterrat", &ui_font_monsterrat },
#if LV_FONT_MONTSERRAT_8
    { "MONTSERRAT_8", &lv_font_montserrat_8 },
#endif
#if LV_FONT_MONTSERRAT_10
    { "MONTSERRAT_10", &lv_font_montserrat_10 },
#endif
#if LV_FONT_MONTSERRAT_12
    { "MONTSERRAT_12", &lv_font_montserrat_12 },
#endif
#if LV_FONT_MONTSERRAT_14
    { "MONTSERRAT_14", &lv_font_montserrat_14 },
#endif
#if LV_FONT_MONTSERRAT_16
    { "MONTSERRAT_16", &lv_font_montserrat_16 },
#endif
#if LV_FONT_MONTSERRAT_18
    { "MONTSERRAT_18", &lv_font_montserrat_18 },
#endif
#if LV_FONT_MONTSERRAT_20
    { "MONTSERRAT_20", &lv_font_montserrat_20 },
#endif
#if LV_FONT_MONTSERRAT_22
    { "MONTSERRAT_22", &lv_font_montserrat_22 },
#endif
#if LV_FONT_MONTSERRAT_24
    { "MONTSERRAT_24", &lv_font_montserrat_24 },
#endif
#if LV_FONT_MONTSERRAT_26
    { "MONTSERRAT_26", &lv_font_montserrat_26 },
#endif
#if LV_FONT_MONTSERRAT_28
    { "MONTSERRAT_28", &lv_font_montserrat_28 },
#endif
#if LV_FONT_MONTSERRAT_30
    { "MONTSERRAT_30", &lv_font_montserrat_30 },
#endif
#if LV_FONT_MONTSERRAT_32
    { "MONTSERRAT_32", &lv_font_montserrat_32 },
#endif
#if LV_FONT_MONTSERRAT_34
    { "MONTSERRAT_34", &lv_font_montserrat_34 },
#endif
#if LV_FONT_MONTSERRAT_36
    { "MONTSERRAT_36", &lv_font_montserrat_36 },
#endif
#if LV_FONT_MONTSERRAT_38
    { "MONTSERRAT_38", &lv_font_montserrat_38 },
#endif
#if LV_FONT_MONTSERRAT_40
    { "MONTSERRAT_40", &lv_font_montserrat_40 },
#endif
#if LV_FONT_MONTSERRAT_42
    { "MONTSERRAT_42", &lv_font_montserrat_42 },
#endif
#if LV_FONT_MONTSERRAT_44
    { "MONTSERRAT_44", &lv_font_montserrat_44 },
#endif
#if LV_FONT_MONTSERRAT_46
    { "MONTSERRAT_46", &lv_font_montserrat_46 },
#endif
#if LV_FONT_MONTSERRAT_48
    { "MONTSERRAT_48", &lv_font_montserrat_48 },
#endif
};

//
// Color themes
//

uint32_t active_theme_index = 0;

//
//
//

void create_screens() {

// Set default LVGL theme
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    // Initialize screens
    // Create screens
    create_screen_main();
    create_screen_page1();
}