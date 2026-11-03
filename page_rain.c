/**
 * @file ui.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "page_rain.h"

#if 1


static uint16_t page_open(void);
static uint16_t page_close(void);

static void page_rain_table_update(lv_timer_t * timer);

static void page_close_event_handler(lv_event_t * e);
static void btn_event_handler(lv_event_t * e);

extern void rain_partition_except_close(void);


#define BUTTON_COUNT 11


extern button_info_t page1_btn_list[BUTTON_COUNT];//全局按钮结构体数组


extern lv_style_t style_bt1;//全局按钮样式——按下前
extern lv_style_t style_bt2;//全局按钮样式——按下后


extern lv_style_t style2;//表格/按钮内字体样式

extern lv_obj_t * btn_cont;//全局按钮容器

lv_obj_t * table_rain;

//放置用于存储传感器数据的全局变量(光照度、室温、湿度、杀虫仓温度、烘干仓温度、降雨状态（0无雨1有雨）) 
extern uint32_t light_value;//光照度
extern int8_t dht_data_buf[2];//顺序为：湿度、温度
extern uint16_t temp_data_buf[2];//顺序为：杀虫仓温度、烘干仓温度
extern bool rain_flag;//降雨标志位，无雨为0，有雨为1
extern char * rain_str;//降雨状态字符串

extern bool page1_flag;

button_info_t page_rain_btn_list[BUTTON_COUNT];//全局按钮结构体数组

    



lv_ui_page_data_t lv_my_ui_page_rain_t = {
    .page_cont = NULL,
    .open = page_open,
    .close = page_close
 };
 

 static uint16_t page_open(void)
{

    lv_obj_t * page_cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(page_cont, 0, 0);//去除容器内的填充

    sub_page_top_bar_init(page_cont, "休眠状态", page_close_event_handler);//子页面顶栏部分初始化
    
    sub_page_table_bt_cont_init(&table_rain, page_cont);//子页面表格和按钮容器部分初始化

    lv_obj_t * label;

    for(int i = 0; i < BUTTON_COUNT; i++)
    {

        if(i == 0)
        {
            page_rain_btn_list[i].btn = lv_button_create(page_cont);

            lv_obj_add_style(page_rain_btn_list[i].btn, &style_bt1, 0);
            lv_obj_set_size(page_rain_btn_list[i].btn, 110, 225);
        }
        else
        {
            page_rain_btn_list[i].btn = lv_button_create(btn_cont);

            if(i != 8)
            {
                lv_obj_add_style(page_rain_btn_list[i].btn, &style_bt1, 0);//除虫雨挡板外的按钮全部默认灰色
            }
            else
            {
                lv_obj_add_style(page_rain_btn_list[i].btn, &style_bt2, 0);//虫雨挡板按钮默认蓝色
            }

            lv_obj_set_size(page_rain_btn_list[i].btn, 110, 105);
        }

        if((i != 0) && (i != 5))//更新除模式切换和拍照以外的所有按钮状态
        {
            page1_btn_list[i].state = false;
        }

        label = lv_label_create(page_rain_btn_list[i].btn);
        lv_obj_add_style(label, &style2, 0);
              
        if(i == 0)
        {
            lv_label_set_text(label, "休眠模式");
        }
        else
        {
            lv_label_set_text_fmt(label, "%s", page1_btn_list[i].label);
        }

        if((i != 0) && (i != 5))
        {

            lv_obj_t * label_st = lv_label_create(label);
            lv_obj_add_style(label, &style2, 0);

            if(i != 8)
            {
                lv_label_set_text(label_st, "关闭");//除虫雨挡板外的按钮全部默认关闭 
            }
            else
            {
                lv_label_set_text(label_st, "开启");//虫雨挡板按钮默认开启
            }

            lv_obj_align_to(label_st, label,LV_ALIGN_OUT_BOTTOM_MID,0,0);

        }

        lv_obj_center(label);


    }

    // page1_btn_list[8].state = true;//函数sleep_process()中已执行

    lv_obj_align_to(page_rain_btn_list[0].btn, btn_cont, LV_ALIGN_OUT_LEFT_MID, -15, 0);

    lv_my_ui_page_rain_t.page_cont = page_cont;

    lv_timer_t * timer = lv_timer_create(page_rain_table_update, 500, NULL);//添加定时器

    lv_obj_set_user_data(page_cont, timer);

    return 0;

}


static uint16_t page_close(void)
{
   lv_timer_delete(lv_obj_get_user_data(lv_my_ui_page_rain_t.page_cont));
   lv_obj_delete(lv_my_ui_page_rain_t.page_cont);
   my_ui_main_go_back_handle();

   page1_flag = 0;

   return 0;
}

static void page_close_event_handler(lv_event_t * e)
{
    lv_obj_t * label = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);

    //lv_obj_t * g_page1_cont = lv_obj_get_parent(btn);
    //lv_obj_t * g_page1_cont = lv_obj_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        page_close();
    }
}

static void page_rain_table_update(lv_timer_t * timer)
{
  
  // lv_obj_t * label = lv_timer_get_user_data(timer);
  // LV_LOG_USER("page a timer running");  

  // lv_label_set_text_fmt(label, "%d", lv_rand(0,1000));
    lv_table_set_cell_value_fmt(table_rain, 0, 0, "光照度\n%d LUX",light_value);
    lv_table_set_cell_value_fmt(table_rain, 0, 1, "室温\n%d ℃",dht_data_buf[1]);
    lv_table_set_cell_value_fmt(table_rain, 0, 2, "湿度\n%d %%",dht_data_buf[0]);
    lv_table_set_cell_value_fmt(table_rain, 0, 3, "杀虫仓温度\n%d ℃",temp_data_buf[0] / 10);
    lv_table_set_cell_value_fmt(table_rain, 0, 4, "烘干仓温度\n%d ℃",temp_data_buf[1] / 10);
    lv_table_set_cell_value_fmt(table_rain, 0, 5, "降雨状态\n%s",rain_str);

}

 
 #endif
 