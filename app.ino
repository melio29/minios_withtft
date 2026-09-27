// // #include <lvgl.h>
// // #include <TFT_eSPI.h>
// // #include "ui.h"

// // TFT_eSPI tft = TFT_eSPI();
 

// // /**********************
// //  * LVGL Draw Buffer
// //  **********************/
// // static lv_disp_draw_buf_t draw_buf;
// // static lv_color_t buf[480 * 20];

// // /**********************
// //  * Display Flush
// //  **********************/
// // void my_disp_flush(lv_disp_drv_t *disp,
// //                    const lv_area_t *area,
// //                    lv_color_t *color_p)
// // {
// //     uint32_t w = area->x2 - area->x1 + 1;
// //     uint32_t h = area->y2 - area->y1 + 1;

// //     tft.startWrite();
// //     tft.setAddrWindow(area->x1, area->y1, w, h);
// //     tft.pushColors((uint16_t *)&color_p->full, w * h, true);
// //     tft.endWrite(); 

// //     lv_disp_flush_ready(disp);
// // }

// // /**********************
// //  * Touch Read
// //  **********************/
// // void my_touchpad_read(lv_indev_drv_t * indev_driver,
// //                       lv_indev_data_t * data)
// // {
// //     uint16_t x, y;

// //     bool touched = tft.getTouch(&x, &y);

// //     if (!touched)
// //     {
// //         data->state = LV_INDEV_STATE_RELEASED;
// //     }
// //     else
// //     {
// //         data->state = LV_INDEV_STATE_PRESSED;
// //         data->point.x = x;
// //         data->point.y = y;

// //         Serial.print("Touch : ");
// //         Serial.print(x);
// //         Serial.print(" , ");
// //         Serial.println(y);
// //     }
// // }

// // void setup()
// // {
// //     Serial.begin(115200);

// //     tft.init();
// //     tft.setRotation(1);

// //     lv_init();

// //     lv_disp_draw_buf_init(&draw_buf, buf, NULL, 480 * 20);

// //     static lv_disp_drv_t disp_drv;
// //     lv_disp_drv_init(&disp_drv);

// //     disp_drv.hor_res = 480;
// //     disp_drv.ver_res = 320;
// //     disp_drv.flush_cb = my_disp_flush;
// //     disp_drv.draw_buf = &draw_buf;

// //     lv_disp_drv_register(&disp_drv);

// //     /************* TOUCH *************/
// //     static lv_indev_drv_t indev_drv;
// //     lv_indev_drv_init(&indev_drv);

// //     indev_drv.type = LV_INDEV_TYPE_POINTER;
// //     indev_drv.read_cb = my_touchpad_read;

// //     lv_indev_drv_register(&indev_drv);

// //     /************* UI *************/
// //     ui_init();

   

// // }

// // void loop()
// // {
// //     lv_timer_handler();
// //     delay(5);

// // }
// #include <lvgl.h>
// #include <TFT_eSPI.h>
// #include "ui.h"

// TFT_eSPI tft = TFT_eSPI();

// // calculator_setup(), calculator_open(), calculator_loop(), calculator_is_active()
// // are defined in calculator.ino. Declared explicitly here (instead of relying
// // on Arduino's auto-prototype generator, which can miss functions declared
// // inside extern "C" blocks).
// extern "C" {
//     void calculator_setup();
//     void calculator_open();
//     void calculator_loop();
//     int  calculator_is_active();
//     void calculator_request_open();
//     int  calculator_consume_open_request();
// }
 

// /**********************
//  * LVGL Draw Buffer
//  **********************/
// static lv_disp_draw_buf_t draw_buf;
// static lv_color_t buf[480 * 20];

// /**********************
//  * Display Flush
//  **********************/
// void my_disp_flush(lv_disp_drv_t *disp,
//                    const lv_area_t *area,
//                    lv_color_t *color_p)
// {
//     uint32_t w = area->x2 - area->x1 + 1;
//     uint32_t h = area->y2 - area->y1 + 1;

//     tft.startWrite();
//     tft.setAddrWindow(area->x1, area->y1, w, h);
//     tft.pushColors((uint16_t *)&color_p->full, w * h, true);
//     tft.endWrite(); 

//     lv_disp_flush_ready(disp);
// }

// /**********************
//  * Touch Read
//  **********************/
// void my_touchpad_read(lv_indev_drv_t * indev_driver,
//                       lv_indev_data_t * data)
// {
//     uint16_t x, y;

//     bool touched = tft.getTouch(&x, &y);

//     if (!touched)
//     {
//         data->state = LV_INDEV_STATE_RELEASED;
//     }
//     else
//     {
//         data->state = LV_INDEV_STATE_PRESSED;
//         data->point.x = x;
//         data->point.y = y;

//         Serial.print("Touch : ");
//         Serial.print(x);
//         Serial.print(" , ");
//         Serial.println(y);
//     }
// }

// void setup()
// {
//     Serial.begin(115200);

//     tft.init();
//     tft.setRotation(1);

//     lv_init();

//     lv_disp_draw_buf_init(&draw_buf, buf, NULL, 480 * 20);

//     static lv_disp_drv_t disp_drv;
//     lv_disp_drv_init(&disp_drv);

//     disp_drv.hor_res = 480;
//     disp_drv.ver_res = 320;
//     disp_drv.flush_cb = my_disp_flush;
//     disp_drv.draw_buf = &draw_buf;

//     lv_disp_drv_register(&disp_drv);

//     /************* TOUCH *************/
//     static lv_indev_drv_t indev_drv;
//     lv_indev_drv_init(&indev_drv);

//     indev_drv.type = LV_INDEV_TYPE_POINTER;
//     indev_drv.read_cb = my_touchpad_read;

//     lv_indev_drv_register(&indev_drv);

//     /************* UI *************/
//     ui_init();

//     /************* APPS *************/
//     calculator_setup();   // one-time touch calibration for the calculator app
// }

// void loop()
// {
//     if (calculator_is_active())
//     {
//         // Calculator has taken over the screen: run its own touch/draw loop,
//         // LVGL's timer handler is paused so it doesn't redraw over it.
//         calculator_loop();
//     }
//     else
//     {
//         lv_timer_handler();

//         // Only open the calculator AFTER LVGL's current frame is fully done —
//         // opening it mid-event gets immediately overwritten by LVGL's flush.
//         if (calculator_consume_open_request())
//         {
//             calculator_open();
//         }
//     }

//     delay(5);
// }
#include <lvgl.h>
#include <TFT_eSPI.h>
#include "ui.h"
#include "touch_cal.h"
#include "esp_heap_caps.h"   

TFT_eSPI tft = TFT_eSPI();


extern "C" {
    void calculator_open();
    void calculator_loop();
    int  calculator_is_active();
    void calculator_request_open();
    int  calculator_consume_open_request();
}


extern "C" {
    void chatbot_setup();
    void chatbot_open();
    void chatbot_loop();
    int  chatbot_is_active();
    void chatbot_request_open();
    int  chatbot_consume_open_request();
}


extern "C" {
    void set_setup();
    void set_open();
    void set_loop();
    int  set_is_active();
    void set_request_open();
    int  set_consume_open_request();
}


extern "C" {
    void notes_setup();
    void notes_open();
    void notes_loop();
    int  notes_is_active();
    void notes_request_open();
    int  notes_consume_open_request();
}


extern "C" {
    void em_setup();
    void em_open();
    void em_loop();
    int  em_is_active();
    void em_request_open();
    int  em_consume_open_request();
}
extern "C"{
    void gm_setup();
    void gm_open();
    void gm_loop();
    int  gm_is_active();
    void gm_request_open();
    int  gm_consume_open_request();
}
extern "C" {
    void camera_setup();
    void camera_open();
    void camera_loop();
    int  camera_is_active();
    void camera_request_open();
    int  camera_consume_open_request();
}

extern "C" {
    void time_setup();
    void time_open();
    void time_loop();
    int  time_is_active();
    void time_request_open();
    int  time_consume_open_request();
}


static lv_disp_draw_buf_t draw_buf;


#define LVGL_BUF_ROWS 320
static lv_color_t *buf1 = nullptr;
static lv_color_t *buf2 = nullptr;


void my_disp_flush(lv_disp_drv_t *disp,
                    const lv_area_t *area,
                    lv_color_t *color_p)
{
    uint32_t w = area->x2 - area->x1 + 1;
    uint32_t h = area->y2 - area->y1 + 1;

    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();

    lv_disp_flush_ready(disp);
}


void my_touchpad_read(lv_indev_drv_t * indev_driver,
                       lv_indev_data_t * data)
{
    uint16_t x, y;
    bool touched = tft.getTouch(&x, &y);

    if (!touched)
    {
        data->state = LV_INDEV_STATE_RELEASED;
    }
    else
    {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = x;
        data->point.y = y;
    }
}

void setup()
{
    Serial.begin(115200);
    delay(3000);
    Serial.println("1: Serial up");

    tft.init();
    Serial.println("2: tft.init done");
    tft.setRotation(3);
    Serial.println("3: rotation set");


    touch_calibration_init();
    Serial.println("4: touch cal done");

    lv_init();
    Serial.println("5: lv_init done");

    // ── PSRAM double buffer allocation ────────────────────────────
    buf1 = (lv_color_t *) heap_caps_malloc(480 * LVGL_BUF_ROWS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);
    buf2 = (lv_color_t *) heap_caps_malloc(480 * LVGL_BUF_ROWS * sizeof(lv_color_t), MALLOC_CAP_SPIRAM);

    if (buf1 && buf2)
    {
        Serial.println("6: LVGL double buffer allocated from PSRAM successfully.");
        lv_disp_draw_buf_init(&draw_buf, buf1, buf2, 480 * LVGL_BUF_ROWS);
    }
    else if (buf1)
    {
        Serial.println("6: PSRAM partial alloc — falling back to single PSRAM buffer.");
        lv_disp_draw_buf_init(&draw_buf, buf1, NULL, 480 * LVGL_BUF_ROWS);
    }
    else
    {

        Serial.println("6: PSRAM allocation FAILED — check Tools > PSRAM is enabled. Using small internal buffer.");
        static lv_color_t fallback_buf[480 * 20];
        lv_disp_draw_buf_init(&draw_buf, fallback_buf, NULL, 480 * 20);
    }

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = 480;
    disp_drv.ver_res = 320;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);
    Serial.println("7: display driver registered");

   
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = my_touchpad_read;
    lv_indev_drv_register(&indev_drv);
    Serial.println("8: touch driver registered");

    /************* UI *************/
    ui_init();
    Serial.println("9: ui_init done");

    /************* APPS one-time setup *************/
    chatbot_setup();
    Serial.printf("10: chatbot_setup done, free heap: %u, largest block: %u\n",
                   ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    notes_setup();
    Serial.printf("11: notes_setup done, free heap: %u, largest block: %u\n",
                   ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    set_setup();
    Serial.printf("12: set_setup done, free heap: %u, largest block: %u\n",
                   ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    gm_setup();
       Serial.printf("12: gm_setup done, free heap: %u, largest block: %u\n",
                   ESP.getFreeHeap(), ESP.getMaxAllocHeap());
   

    em_setup();   // ← re-enable when we come back to the email app
    Serial.printf("13: em_setup done, free heap: %u, largest block: %u\n",
                   ESP.getFreeHeap(), ESP.getMaxAllocHeap());

    camera_setup();
    Serial.printf("14:camera_setup done,free heap:%u,largest block:%u\n",
                     ESP.getFreeHeap(),ESP.getMaxAllocHeap());
    time_setup();
    Serial.printf("14:time_setup done,free heap:%u,largest block:%u\n",
                    ESP.getFreeHeap(),ESP.getMaxAllocHeap());
}

void loop()
{

    static bool lastPrinted = false;
bool anyActive = calculator_is_active() || chatbot_is_active() || set_is_active() || notes_is_active() || em_is_active();
if (anyActive != lastPrinted) {
  Serial.printf("[STATE] calc=%d chat=%d set=%d notes=%d em=%d\n",
    calculator_is_active(), chatbot_is_active(), set_is_active(), notes_is_active(), em_is_active());
  lastPrinted = anyActive;
}
    if (calculator_is_active())
    {
        calculator_loop();
    }
    else if (chatbot_is_active())
    {
        chatbot_loop();
    }
    else if (set_is_active())
    {
        set_loop();
    }
    else if (notes_is_active())
    {
        notes_loop();
    }
     else if (em_is_active())
     {
         em_loop();
     }
     else if(gm_is_active()){
        gm_loop();
     }
     else if(camera_is_active()){
        camera_loop();
     }
     else if(time_is_active()){
        time_loop();
     }
    else
    {
        lv_timer_handler();


        if (calculator_consume_open_request())
        {
            calculator_open();
        }
        else if (chatbot_consume_open_request())
        {
            chatbot_open();
        }
        else if (set_consume_open_request())
        {
            set_open();
        }
        else if (notes_consume_open_request())
        {
            notes_open();
        }
        else if (em_consume_open_request())
        {
            em_open();
        }
        else if(gm_consume_open_request()){
            gm_open();
        }
        else if(camera_consume_open_request()){
              camera_open();
        }
        else if(time_consume_open_request()){
            time_open();
        }
    }

    delay(5);
}
