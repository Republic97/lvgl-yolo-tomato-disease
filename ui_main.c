/**
 * @file ui_main.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

 #include "ui_main.h"



 #if 1


 #include "page1.h"
 #include "page2.h"

 #include "page_rain.h"
 #include "page_pic.h"

//  #define APP_SUM 20

 LV_IMAGE_DECLARE(img_cogwheel_argb);

 static void page_event_handler(lv_event_t * e);
 static void shutdown_event_handler(lv_event_t * e);

 static lv_obj_t * g_page_cont;//主页面容器
 extern lv_obj_t * g_page1_cont;//页面一容器
 
 extern bool page1_flag;//页面标志位，所处页面为页面一时为1，所处页面为主页面时为0
 extern bool sleep_flag;//休眠标志位，活跃状态为0，休眠状态为1

 extern void page_1(void);



 static lv_ui_page_data_t * g_page_list[2] = {
  &lv_my_ui_page2_t,
  &lv_my_ui_page_pic_t
};

 /**********************
  *   GLOBAL FUNCTIONS
  **********************/
static void page_main(void)
{
    g_page_cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(g_page_cont, lv_pct(100), lv_pct(100));

    lv_obj_t * img;
    lv_obj_t * label;

    lv_font_t * font1 = lv_freetype_font_create("A:/home/wuyue/Desktop/lvgl_v9_demo/apple_black.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        60,
        LV_FREETYPE_FONT_STYLE_NORMAL);//虫情测报仪字体路径
        
    lv_font_t * font2 = lv_freetype_font_create("A:/home/wuyue/Desktop/lvgl_v9_demo/apple_black.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        30,
        LV_FREETYPE_FONT_STYLE_NORMAL);//首页图标名称字体路径

    lv_font_t * font_aw = lv_freetype_font_create("A:/home/wuyue/Desktop/lvgl_v9_demo/FontAwesome5.woff",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        40,
        LV_FREETYPE_FONT_STYLE_NORMAL);//4G标志字体路径
        
    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_text_font(&style1, font1);//虫情测报仪样式

    label = lv_label_create(g_page_cont);
    lv_obj_add_style(label, &style1, 0);
    lv_label_set_text(label, "番茄叶片病虫害检测系统");    
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 90);//"虫情测报仪"标签

    static lv_style_t style_symbol;
    lv_style_init(&style_symbol);
    lv_style_set_text_font(&style_symbol, font_aw);//4G标志样式

    // label = lv_label_create(g_page_cont);
    // lv_obj_add_style(label, &style_symbol, 0);
    // lv_label_set_text(label, "\xEF\x80\x92");//4G标志utf-8
    // lv_obj_set_align(label, LV_ALIGN_TOP_RIGHT);//4G标志标签

    label = lv_label_create(g_page_cont);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(label, &style_symbol, 0);
    lv_label_set_text(label, "\xEF\x80\x91");//电源标志utf-8
    lv_obj_set_align(label, LV_ALIGN_TOP_LEFT);//电源标志标签
    lv_obj_add_event_cb(label, shutdown_event_handler, LV_EVENT_CLICKED, NULL);

    static lv_style_t style2;
    lv_style_init(&style2);
    lv_style_set_text_font(&style2, font2);//首页图标名称样式
    
    
    //图标一
    // img = lv_image_create(g_page_cont);
    // lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    // lv_image_set_src(img, "A:/home/wuyue/Desktop/lvgl_v9_demo/state.png");
    // lv_obj_set_size(img, 150, 150);
    // lv_image_set_align(img, LV_IMAGE_ALIGN_STRETCH);
    // lv_obj_align(img, LV_ALIGN_CENTER, -220, 80);

    // label = lv_label_create(g_page_cont);
    // lv_obj_add_style(label, &style2, 0);
    // lv_label_set_text(label, "运行状态");
    // lv_obj_align_to(label, img,LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    // lv_obj_add_event_cb(img, page_event_handler, LV_EVENT_CLICKED, label);

    //图标二
    img = lv_image_create(g_page_cont);
    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(img, &img_cogwheel_argb);
    lv_obj_set_size(img, 150, 150);
    lv_image_set_align(img, LV_IMAGE_ALIGN_STRETCH);
    lv_obj_align(img, LV_ALIGN_CENTER, 130, 80);
    lv_obj_set_user_data(img, g_page_list[0]);

    label = lv_label_create(g_page_cont);
    lv_obj_add_style(label, &style2, 0);
    lv_label_set_text(label, "关于本机");
    lv_obj_align_to(label, img,LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(img, page_event_handler, LV_EVENT_CLICKED, label);

    //图标三
    img = lv_image_create(g_page_cont);
    lv_obj_add_flag(img, LV_OBJ_FLAG_CLICKABLE);
    lv_image_set_src(img, "/home/wuyue/Desktop/lvgl_v9_demo/picture.png");
    lv_obj_set_size(img, 150, 150);
    lv_image_set_align(img, LV_IMAGE_ALIGN_STRETCH);
    lv_obj_align(img, LV_ALIGN_CENTER, -130, 80);
    lv_obj_set_user_data(img, g_page_list[1]);

    label = lv_label_create(g_page_cont);
    lv_obj_add_style(label, &style2, 0);
    lv_label_set_text(label, "拍照识别");
    lv_obj_align_to(label, img,LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_add_event_cb(img, page_event_handler, LV_EVENT_CLICKED, label);

}

 void ui_main(void)
 {

    // page_1();
    page_main();
    
 }

 void my_ui_main_go_back_handle(void)
{
    lv_obj_remove_flag(g_page_cont, LV_OBJ_FLAG_HIDDEN);
}

//关机回调函数
static void shutdown_event_handler(lv_event_t * e)
{
    int ret = system("sudo shutdown");

    if (ret != 0) {
        LV_LOG_ERROR("\n shutdown failed!"); 
    }
}
 
 static void page_event_handler(lv_event_t * e)
 {
   lv_obj_t * img = lv_event_get_target(e);
   lv_event_code_t code = lv_event_get_code(e);
   lv_obj_t * label = lv_event_get_user_data(e);

   lv_obj_t * page_cont = lv_obj_get_parent(img);

   if(strcmp(lv_label_get_text(label), "运行状态") == 0)
   {
        lv_obj_add_flag(page_cont, LV_OBJ_FLAG_HIDDEN);

        if(!sleep_flag)
        {
            lv_obj_remove_flag(g_page1_cont, LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_my_ui_page_rain_t.open();
        }

        page1_flag = 1;
   }
   else
   {
        lv_ui_page_data_t * page_data = lv_obj_get_user_data(img);

        if(code == LV_EVENT_CLICKED) 
        {
            // LV_LOG_USER("%s", lv_label_get_text(label));       
            
            page_data->open();//精简为一条代码

            lv_obj_add_flag(page_cont, LV_OBJ_FLAG_HIDDEN);

        }
   }
}

 #endif
 
