/**
 * @file ui_main.h
 *
 */

 #ifndef UI_MAIN_H
 #define UI_MAIN_H
 
 #ifdef __cplusplus
 extern "C" {
 #endif

#if 1 

#include "lvgl/lvgl.h"
#include "pthread.h"
#include <stdatomic.h>
#include <fcntl.h>
#include <unistd.h>

// #include "../lvgl/src/widgets/line/lv_line.h"
#define BUTTON_COUNT 11


typedef struct{
    lv_obj_t * page_cont;
    uint16_t (*open)(void);
    uint16_t (*close)(void);
}lv_ui_page_data_t;

typedef struct {
    lv_obj_t *btn;
    bool state;
    const char *label;
} button_info_t;

// 定义事件类型
typedef enum {

    EVENT_BLACK_LIGHT_PRESS,  //诱虫灯 
    EVENT_LED_PRESS, //补光灯 
    EVENT_TRANSPORT_PRESS, //移虫装置 
    EVENT_VIBRATOR_PRESS, //震动装置 
    EVENT_CAMERA_PRESS, //拍照 
    EVENT_KILL_PARTITION_PRESS, //杀虫隔板 
    EVENT_BAKE_PARTITION_PRESS, //烘干隔板 
    EVENT_RAIN_PARTITION_PRESS, //虫雨隔板 
    EVENT_KILL_PRESS, //杀虫
    EVENT_BAKE_PRESS, //烘干
    // EVENT_TEMP_GET, //杀虫仓、烘干仓温度获取 
    EVENT_MAX         // 事件类型总数

} EventType;



 void ui_main(void);
 void my_ui_main_go_back_handle(void);
 extern void manual_to_auto_gui_refresh(lv_obj_t *label1,lv_obj_t *label2);//手动模式转自动模式，刷新GUI
 extern void enqueueEvent(EventType type);

 /**********************
  *      MACROS
  **********************/
 
 #endif /*LV_USE_BUTTON*/
 
 #ifdef __cplusplus
 } /*extern "C"*/
 #endif
 
 #endif /*LV_BUTTON_H*/
 