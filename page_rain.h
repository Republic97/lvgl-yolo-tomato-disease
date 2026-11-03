/**
 * @file page1.h
 *
 */

 #ifndef PAGE_RAIN_H
 #define PAGE_RAIN_H
 
 #ifdef __cplusplus
 extern "C" {
 #endif

#if 1 

#include "ui_main.h"

extern lv_ui_page_data_t lv_my_ui_page_rain_t;

extern void sub_page_top_bar_init(lv_obj_t * page_cont, const char *text, void page_close_event_handler(lv_event_t * e));
extern void style_bt_init(void);
extern void sub_page_table_bt_cont_init(lv_obj_t * *table, lv_obj_t * page_cont);


// void ui(void);
 
 /**********************
  *      MACROS
  **********************/
 
 #endif /*LV_USE_BUTTON*/
 
 #ifdef __cplusplus
 } /*extern "C"*/
 #endif
 
 #endif /*LV_BUTTON_H*/
 