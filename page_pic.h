/**
 * @file page_pic.h
 *
 */

 #ifndef PAGE_PIC_H
 #define PAGE_PIC_H
 
 #ifdef __cplusplus
 extern "C" {
 #endif

#if 1 

#include "ui_main.h"

extern lv_ui_page_data_t lv_my_ui_page_pic_t;

extern void sub_page_top_bar_init(lv_obj_t * page_cont, const char *text, void page_close_event_handler(lv_event_t * e));

extern lv_style_t style2;//表格/按钮内字体样式

// void ui(void);
 
 /**********************
  *      MACROS
  **********************/
 
 #endif /*LV_USE_BUTTON*/
 
 #ifdef __cplusplus
 } /*extern "C"*/
 #endif
 
 #endif /*LV_BUTTON_H*/
 