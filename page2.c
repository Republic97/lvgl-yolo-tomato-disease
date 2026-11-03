/**
 * @file ui.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "page2.h"

#if 1

static uint16_t page_open(void);
static uint16_t page_close(void);
static void page_close_event_handler(lv_event_t * e);

extern char insectMachineNum[32];//设备标识号


lv_ui_page_data_t lv_my_ui_page2_t = {
   .page_cont = NULL,
   .open = page_open,
   .close = page_close
};


static uint16_t page_open(void)
{
    lv_obj_t * page_cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(page_cont, 0, 0);

    sub_page_top_bar_init(page_cont, "关于本机", page_close_event_handler);//子页面顶栏部分初始化

    lv_obj_t * table1 = lv_table_create(page_cont);
    lv_obj_add_style(table1, &style2, 0);

    lv_obj_t * table2 = lv_table_create(page_cont);
    lv_obj_add_style(table2, &style2, 0);

    lv_table_set_cell_value(table1, 0, 0, "芯片型号");
    lv_table_set_cell_value(table1, 1, 0, "内存容量");
    lv_table_set_cell_value(table1, 2, 0, "存储容量");
    lv_table_set_cell_value(table1, 0, 1, "博通bcm2711");
    lv_table_set_cell_value(table1, 1, 1, "4G");
    lv_table_set_cell_value(table1, 2, 1, "32G");
    lv_table_set_column_width(table1, 1, 200);
    lv_obj_align(table1, LV_ALIGN_LEFT_MID, 20, 20);

    lv_table_set_cell_value(table2, 0, 0, "照片像素");
    lv_table_set_cell_value(table2, 1, 0, "识别算法");
    lv_table_set_cell_value(table2, 2, 0, "设备标识码");
    lv_table_set_cell_value(table2, 0, 1, "1200万");
    lv_table_set_cell_value(table2, 1, 1, "FCE-YOLO");
    lv_table_set_cell_value_fmt(table2, 2, 1, "%s",insectMachineNum);//动态获取设备标识码
    lv_table_set_column_width(table2, 1, 240);
    lv_table_set_column_width(table2, 0, 180);
    lv_obj_align(table2, LV_ALIGN_RIGHT_MID, -20, 20);
        
    lv_my_ui_page2_t.page_cont = page_cont;

    return 0;
}

static uint16_t page_close(void)
{
   lv_obj_delete(lv_my_ui_page2_t.page_cont);
   my_ui_main_go_back_handle();
   return 0;
}

static void page_close_event_handler(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    

    if(code == LV_EVENT_CLICKED) {
        page_close();
    }
}

 #endif
