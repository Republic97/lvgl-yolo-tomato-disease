/**
 * @file ui.c
 *
 */

/*********************
 *      INCLUDES
 *********************/


#include "page_pic.h"
#include "servo_control.h"

#if 1

static uint16_t page_open(void);
static uint16_t page_close(void);
static void page_close_event_handler(lv_event_t * e);
static void direction_button_event_handler(lv_event_t * e);
static void inference_toggle_event_handler(lv_event_t * e);
static void init_visual_resources(void);
static void servo_move_timer_cb(lv_timer_t * timer);
static lv_obj_t * create_overlay_button(lv_obj_t * parent,
                                        const char * text,
                                        int32_t width,
                                        int32_t height,
                                        lv_align_t align,
                                        int32_t x_offset,
                                        int32_t y_offset,
                                        const lv_font_t * font);

#define CONTENT_HEIGHT 420
#define BOTTOM_BAR_HEIGHT 60
#define DISPLAY_PANEL_HEIGHT 400 
#define PREVIEW_PANEL_WIDTH 520
#define RESULT_PANEL_WIDTH 244

typedef enum {
    DISPLAY_MODE_PREVIEW,
    DISPLAY_MODE_INFERENCE_RESULT
} display_mode_t;

static lv_obj_t * g_result_summary_label;
static lv_obj_t * g_preview_cont;
static lv_obj_t * g_preview_label;
static lv_font_t * g_font_aw;
static lv_font_t * g_font_cn;
static lv_style_t g_back_style;
static bool g_visual_resources_ready;
static display_mode_t g_display_mode;

/*************************************************************************************/

//舵机相关代码
typedef enum {
    SERVO_MOVE_NONE,
    SERVO_MOVE_UP,
    SERVO_MOVE_DOWN,
    SERVO_MOVE_LEFT,
    SERVO_MOVE_RIGHT
} servo_move_t;

static servo_move_t g_servo_move = SERVO_MOVE_NONE;
static lv_timer_t * g_servo_move_timer = NULL;

static int g_pan_angle = 80;
static int g_tilt_angle = 80;

/* 初次测试时使用保守范围，确认机械结构后再扩大 */
#define PAN_MIN_ANGLE  0 
#define PAN_MAX_ANGLE   180
#define TILT_MIN_ANGLE  0
#define TILT_MAX_ANGLE  180

#define SERVO_STEP_ANGLE  1
#define SERVO_STEP_TIME   30

static const servo_move_t g_action_up    = SERVO_MOVE_UP;
static const servo_move_t g_action_down  = SERVO_MOVE_DOWN;
static const servo_move_t g_action_left  = SERVO_MOVE_LEFT;
static const servo_move_t g_action_right = SERVO_MOVE_RIGHT;

/*************************************************************************************/



lv_ui_page_data_t lv_my_ui_page_pic_t = {
   .page_cont = NULL,
   .open = page_open,
   .close = page_close
};


static uint16_t page_open(void)
{
    lv_obj_t * page_cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(page_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(page_cont, 0, 0);
    lv_obj_set_style_border_width(page_cont, 0, 0);
    lv_obj_remove_flag(page_cont, LV_OBJ_FLAG_SCROLLABLE);

    init_visual_resources();

    /* 上方内容区：只使用 420 像素高度，给底部返回栏预留 60 像素。 */
    lv_obj_t * content_cont = lv_obj_create(page_cont);
    lv_obj_set_size(content_cont, lv_pct(100), CONTENT_HEIGHT);
    lv_obj_align(content_cont, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_pad_all(content_cont, 0, 0);
    lv_obj_set_style_border_width(content_cont, 0, 0);
    lv_obj_set_style_bg_opa(content_cont, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(content_cont, LV_OBJ_FLAG_SCROLLABLE);

    /* 左侧预览区：后续在树莓派上用实时摄像头画面替换背景。 */
    g_preview_cont = lv_obj_create(content_cont);
    lv_obj_set_size(g_preview_cont, PREVIEW_PANEL_WIDTH, DISPLAY_PANEL_HEIGHT);
    lv_obj_align(g_preview_cont, LV_ALIGN_TOP_LEFT, 12, 12);
    lv_obj_set_style_pad_all(g_preview_cont, 0, 0);
    lv_obj_set_style_bg_color(g_preview_cont, lv_color_hex(0x202830), 0);
    lv_obj_set_style_bg_opa(g_preview_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(g_preview_cont, lv_color_hex(0x6B7280), 0);
    lv_obj_set_style_border_width(g_preview_cont, 2, 0);
    lv_obj_set_style_radius(g_preview_cont, 12, 0);
    lv_obj_set_style_shadow_width(g_preview_cont, 0, 0);
    lv_obj_remove_flag(g_preview_cont, LV_OBJ_FLAG_SCROLLABLE);

    g_preview_label = lv_label_create(g_preview_cont);
    lv_obj_set_style_text_font(g_preview_label, g_font_cn, 0);
    lv_obj_set_style_text_color(g_preview_label, lv_color_hex(0xD1D5DB), 0);
    lv_obj_set_style_text_align(g_preview_label, LV_TEXT_ALIGN_CENTER, 0);
    lv_label_set_text(g_preview_label, "摄像头实时预览\n（电脑模拟）");
    lv_obj_align(g_preview_label, LV_ALIGN_CENTER, 0, -55);
    g_display_mode = DISPLAY_MODE_PREVIEW;

    /* 四个半透明方向按钮。 */
    lv_obj_t * btn_up = create_overlay_button(g_preview_cont, LV_SYMBOL_UP,
                                               52, 52, LV_ALIGN_BOTTOM_LEFT, 78, -126, g_font_aw);
    lv_obj_add_event_cb(btn_up, direction_button_event_handler, LV_EVENT_ALL, (void *)&g_action_up);

    lv_obj_t * btn_left = create_overlay_button(g_preview_cont, LV_SYMBOL_LEFT,
                                                 52, 52, LV_ALIGN_BOTTOM_LEFT, 22, -70, g_font_aw);
    lv_obj_add_event_cb(btn_left, direction_button_event_handler, LV_EVENT_ALL, (void *)&g_action_left);

    lv_obj_t * btn_right = create_overlay_button(g_preview_cont, LV_SYMBOL_RIGHT,
                                                  52, 52, LV_ALIGN_BOTTOM_LEFT, 134, -70, g_font_aw);
    lv_obj_add_event_cb(btn_right, direction_button_event_handler, LV_EVENT_ALL, (void *)&g_action_right);

    lv_obj_t * btn_down = create_overlay_button(g_preview_cont, LV_SYMBOL_DOWN,
                                                 52, 52, LV_ALIGN_BOTTOM_LEFT, 78, -14, g_font_aw);
    lv_obj_add_event_cb(btn_down, direction_button_event_handler, LV_EVENT_ALL, (void *)&g_action_down);

    /* 第一次点击显示推理结果，再次点击恢复实时预览。 */
    lv_obj_t * inference_btn = create_overlay_button(g_preview_cont, "推理/恢复",
                                                      150, 54, LV_ALIGN_BOTTOM_RIGHT, -22, -32, g_font_cn);
    lv_obj_set_style_radius(inference_btn, 14, 0);
    lv_obj_add_event_cb(inference_btn, inference_toggle_event_handler, LV_EVENT_CLICKED, NULL);

    /* 右侧推理结果区：显示各类别数量和单次推理耗时。 */
    lv_obj_t * result_cont = lv_obj_create(content_cont);
    lv_obj_set_size(result_cont, RESULT_PANEL_WIDTH, DISPLAY_PANEL_HEIGHT);
    lv_obj_align(result_cont, LV_ALIGN_TOP_RIGHT, -12, 12);
    lv_obj_set_style_bg_color(result_cont, lv_color_hex(0x202830), 0);
    lv_obj_set_style_bg_opa(result_cont, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(result_cont, lv_color_hex(0x6B7280), 0);
    lv_obj_set_style_border_width(result_cont, 2, 0);
    lv_obj_set_style_radius(result_cont, 12, 0);
    lv_obj_set_style_shadow_width(result_cont, 0, 0);
    lv_obj_remove_flag(result_cont, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * result_title = lv_label_create(result_cont);
    lv_obj_set_style_text_font(result_title, g_font_cn, 0);
    lv_obj_set_style_text_color(result_title, lv_color_hex(0xF9FAFB), 0);
    lv_label_set_text(result_title, "推理结果");
    lv_obj_align(result_title, LV_ALIGN_TOP_MID, 0, 24);

    g_result_summary_label = lv_label_create(result_cont);
    lv_obj_set_width(g_result_summary_label, 200);
    lv_obj_set_style_text_font(g_result_summary_label, g_font_cn, 0);
    lv_obj_set_style_text_color(g_result_summary_label, lv_color_hex(0xD1D5DB), 0);
    lv_obj_set_style_text_align(g_result_summary_label, LV_TEXT_ALIGN_LEFT, 0);
    lv_obj_set_style_text_line_space(g_result_summary_label, 8, 0);
    lv_label_set_text(g_result_summary_label,
                      "早疫病：0\n"
                      "健康：0\n"
                      "晚疫病：0\n"
                      "潜叶虫损伤：0\n"
                      "叶霉病：0\n\n"
                      "推理时间：-- ms");
    lv_obj_align(g_result_summary_label, LV_ALIGN_TOP_LEFT, 22, 82);

    /* 独立底栏，返回按钮不占用上方内容区。 */
    lv_obj_t * bottom_bar = lv_obj_create(page_cont);
    lv_obj_set_size(bottom_bar, lv_pct(100), BOTTOM_BAR_HEIGHT);
    lv_obj_align(bottom_bar, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_pad_all(bottom_bar, 0, 0);
    lv_obj_set_style_border_width(bottom_bar, 0, 0);
    lv_obj_set_style_bg_opa(bottom_bar, LV_OPA_TRANSP, 0);
    lv_obj_remove_flag(bottom_bar, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t * back_label = lv_label_create(bottom_bar);
    lv_obj_add_flag(back_label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(back_label, &g_back_style, 0);
    lv_label_set_text(back_label, LV_SYMBOL_LEFT);
    lv_obj_align(back_label, LV_ALIGN_CENTER, 0, -8);
    lv_obj_add_event_cb(back_label, page_close_event_handler, LV_EVENT_CLICKED, NULL);
    
        
    lv_my_ui_page_pic_t.page_cont = page_cont;

	g_servo_move = SERVO_MOVE_NONE;

	g_servo_move_timer = lv_timer_create(servo_move_timer_cb,
                                     SERVO_STEP_TIME,
                                     NULL);
	lv_timer_pause(g_servo_move_timer);

    return 0;
}

static void init_visual_resources(void)
{
    if(g_visual_resources_ready) {
        return;
    }

    g_font_aw = lv_freetype_font_create("A:/home/wuyue/Desktop/lvgl_v9_demo/FontAwesome5.woff",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        40,
        LV_FREETYPE_FONT_STYLE_NORMAL);

    g_font_cn = lv_freetype_font_create("A:/home/wuyue/Desktop/lvgl_v9_demo/apple_black.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        20,
        LV_FREETYPE_FONT_STYLE_NORMAL);

    lv_style_init(&g_back_style);
    lv_style_set_text_font(&g_back_style, g_font_aw);
    g_visual_resources_ready = true;
}

static lv_obj_t * create_overlay_button(lv_obj_t * parent,
                                        const char * text,
                                        int32_t width,
                                        int32_t height,
                                        lv_align_t align,
                                        int32_t x_offset,
                                        int32_t y_offset,
                                        const lv_font_t * font)
{
    lv_obj_t * btn = lv_button_create(parent);
    lv_obj_set_size(btn, width, height);
    lv_obj_align(btn, align, x_offset, y_offset);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x111827), 0);
    lv_obj_set_style_bg_opa(btn, LV_OPA_50, 0);
    lv_obj_set_style_border_color(btn, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_opa(btn, LV_OPA_70, 0);
    lv_obj_set_style_border_width(btn, 1, 0);
    lv_obj_set_style_radius(btn, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_bg_color(btn, lv_color_hex(0x2563EB), LV_STATE_PRESSED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_80, LV_STATE_PRESSED);

    lv_obj_t * label = lv_label_create(btn);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
    lv_label_set_text(label, text);
    lv_obj_center(label);

    return btn;
}

static void servo_move_timer_cb(lv_timer_t * timer)
{
    int next_angle;

    switch(g_servo_move) {
        case SERVO_MOVE_LEFT:
            next_angle = g_pan_angle - SERVO_STEP_ANGLE;

            if(next_angle < PAN_MIN_ANGLE) {
                next_angle = PAN_MIN_ANGLE;
            }

            if(next_angle == g_pan_angle) {
                g_servo_move = SERVO_MOVE_NONE;
                lv_timer_pause(timer);
                return;
            }

            if(servo_set_angle(SERVO_PAN, next_angle) == 0) {
                g_pan_angle = next_angle;
            }
            break;

        case SERVO_MOVE_RIGHT:
            next_angle = g_pan_angle + SERVO_STEP_ANGLE;

            if(next_angle > PAN_MAX_ANGLE) {
                next_angle = PAN_MAX_ANGLE;
            }

            if(next_angle == g_pan_angle) {
                g_servo_move = SERVO_MOVE_NONE;
                lv_timer_pause(timer);
                return;
            }

            if(servo_set_angle(SERVO_PAN, next_angle) == 0) {
                g_pan_angle = next_angle;
            }
            break;

        case SERVO_MOVE_UP:
            next_angle = g_tilt_angle + SERVO_STEP_ANGLE;

            if(next_angle > TILT_MAX_ANGLE) {
                next_angle = TILT_MAX_ANGLE;
            }

            if(next_angle == g_tilt_angle) {
                g_servo_move = SERVO_MOVE_NONE;
                lv_timer_pause(timer);
                return;
            }

            if(servo_set_angle(SERVO_TILT, next_angle) == 0) {
                g_tilt_angle = next_angle;
            }
            break;

        case SERVO_MOVE_DOWN:
            next_angle = g_tilt_angle - SERVO_STEP_ANGLE;

            if(next_angle < TILT_MIN_ANGLE) {
                next_angle = TILT_MIN_ANGLE;
            }

            if(next_angle == g_tilt_angle) {
                g_servo_move = SERVO_MOVE_NONE;
                lv_timer_pause(timer);
                return;
            }

            if(servo_set_angle(SERVO_TILT, next_angle) == 0) {
                g_tilt_angle = next_angle;
            }
            break;

        default:
            lv_timer_pause(timer);
            break;
    }
}

static void direction_button_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    const servo_move_t * action = lv_event_get_user_data(e);

    if(action == NULL || g_servo_move_timer == NULL) {
        return;
    }

    if(code == LV_EVENT_PRESSED) {
        g_servo_move = *action;
        lv_timer_resume(g_servo_move_timer);
    }
    else if(code == LV_EVENT_RELEASED ||
            code == LV_EVENT_PRESS_LOST) {
        g_servo_move = SERVO_MOVE_NONE;
        lv_timer_pause(g_servo_move_timer);
    }
}

static void inference_toggle_event_handler(lv_event_t * e)
{
    LV_UNUSED(e);

    if(g_preview_cont == NULL || g_preview_label == NULL) {
        return;
    }

    if(g_display_mode == DISPLAY_MODE_PREVIEW) {
        g_display_mode = DISPLAY_MODE_INFERENCE_RESULT;
        LV_LOG_USER("display mode: inference result");
        lv_obj_set_style_bg_color(g_preview_cont, lv_color_hex(0x173B32), 0);
        lv_label_set_text(g_preview_label, "FCE-YOLO 推理结果\n（电脑模拟）");
    }
    else {
        g_display_mode = DISPLAY_MODE_PREVIEW;
        LV_LOG_USER("display mode: camera preview");
        lv_obj_set_style_bg_color(g_preview_cont, lv_color_hex(0x202830), 0);
        lv_label_set_text(g_preview_label, "摄像头实时预览\n（电脑模拟）");
    }
}

static uint16_t page_close(void)
{
   lv_obj_t * page_cont = lv_my_ui_page_pic_t.page_cont;
   lv_my_ui_page_pic_t.page_cont = NULL;
   g_result_summary_label = NULL;
   g_preview_cont = NULL;
   g_preview_label = NULL;
   g_display_mode = DISPLAY_MODE_PREVIEW;
   g_servo_move = SERVO_MOVE_NONE;
   if(g_servo_move_timer != NULL) {
    lv_timer_delete(g_servo_move_timer);
    g_servo_move_timer = NULL;
   }
   lv_obj_delete(page_cont);
   my_ui_main_go_back_handle();
   return 0;
}

static void page_close_event_handler(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    

    if(code == LV_EVENT_CLICKED) {
        page_close();
    }
}

 #endif
