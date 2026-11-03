#include "mqtt.h"
#include "ui_main.h"


char insectMachineNum[32];//设备标识号
extern atomic_bool g_thread_running;//全局线程运行标志位

extern pthread_mutex_t g_lock,light_flag_lock,temp_get_lock,dht_get_lock;//全局互斥锁

extern bool sleep_flag,mode_flag;//休眠标志位，活跃状态为0，休眠状态为1
extern button_info_t page1_btn_list[BUTTON_COUNT];//全局按钮结构体数组

extern uint32_t light_value;//光照度
extern uint8_t dht_data_buf[2];//室内温湿度数组，顺序为：湿度、温度
extern uint8_t temp_data_buf[2];//杀虫仓/烘干仓温度数组，顺序为：杀虫仓温度、烘干仓温度

extern uint8_t black_light_on[],black_light_off[];
extern uint8_t led_on[],led_off[];
extern uint8_t transport_on[],transport_off[];
extern uint8_t rain_partition_on[],rain_partition_off[];
extern uint8_t vibrator_on[],vibrator_off[];
extern uint8_t bake_partition_on[],bake_partition_off[];
extern uint8_t kill_partition_on[],kill_partition_off[];
extern uint8_t bake_on[],bake_off[];
extern uint8_t kill_on[],kill_off[];

extern lv_style_t style_bt1;//全局按钮样式——按下前
extern lv_style_t style_bt2;//全局按钮样式——按下后

// #define TEST_USEING_TLS  
// extern const char *test_ca_get();

// bool cur_sleep_flag;//当前的休眠标志位，活跃状态为0，休眠状态为1
// bool cur_btn_state_list[BUTTON_COUNT];//当前的按钮状态数组，0为关，1为开





void Remote_Control_handler(void* client, message_data_t* msg)
{
    (void) client;

    //"自动模式", "诱虫灯", "补光灯", "移虫装置","震动装置", "拍照", "杀虫隔板", "烘干隔板","虫雨隔板", "杀虫", "烘干"
    //(自动模式为0，手动模式为1)
    if (!strcmp((char*)msg->message->payload, "mode switch"))
    {
        pthread_mutex_lock(&g_lock);
        lv_obj_t * label = lv_obj_get_child(page1_btn_list[0].btn, 0);
        lv_obj_t * label_st;

        page1_btn_list[0].state = !page1_btn_list[0].state;
        mode_flag = !mode_flag;
        lv_label_set_text_fmt(label, "%s", mode_flag ? "手动模式" : "自动模式");

        if(mode_flag)//接收到消息前是自动模式，接收到消息后切换成手动模式
        {
            lv_obj_add_style(page1_btn_list[5].btn, &style_bt2, 0);
            
        }
        else//接收到消息前是手动模式，接收到消息后切换成自动模式
        {

            lv_obj_add_style(page1_btn_list[5].btn, &style_bt1, 0);

        	manual_to_auto_gui_refresh(label,label_st);

        }
        pthread_mutex_unlock(&g_lock);
        
    }
	else  
    {
        //在互斥锁内获取当前的状态数据
        pthread_mutex_lock(&g_lock);

        lv_obj_t * label;
        lv_obj_t * label_st;
        
        if(page1_btn_list[0].state && (!sleep_flag))//1（手动模式）和0（活跃状态）才往下执行
        {
            uint8_t* payload = (uint8_t*)msg->message->payload;
            //"自动模式", "诱虫灯", "补光灯", "移虫装置","震动装置", "拍照", "杀虫隔板", "烘干隔板","虫雨隔板", "杀虫", "烘干"
            switch (payload[0]) {

            // typedef enum {

            //     EVENT_BLACK_LIGHT_PRESS,  //诱虫灯 
            //     EVENT_LED_PRESS, //补光灯 
            //     EVENT_TRANSPORT_PRESS, //移虫装置 
            //     EVENT_VIBRATOR_PRESS, //震动装置 
            //     EVENT_CAMERA_PRESS, //拍照 
            //     EVENT_KILL_PARTITION_PRESS, //杀虫隔板 
            //     EVENT_BAKE_PARTITION_PRESS, //烘干隔板 
            //     EVENT_RAIN_PARTITION_PRESS, //虫雨隔板 
            //     EVENT_MAX         // 事件类型总数

            // } EventType;

            case 0x01:

                page1_btn_list[1].state = !page1_btn_list[1].state;
                label = lv_obj_get_child(page1_btn_list[1].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[1].state? "开启" : "关闭");

                if(page1_btn_list[1].state)
                {
                    lv_obj_add_style(page1_btn_list[1].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[1].btn, &style_bt1, 0);
                }
                enqueueEvent(EVENT_BLACK_LIGHT_PRESS);
                
                break;

            case 0x02:

                page1_btn_list[2].state = !page1_btn_list[2].state;
                label = lv_obj_get_child(page1_btn_list[2].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[2].state? "开启" : "关闭");

                if(page1_btn_list[2].state)
                {
                    lv_obj_add_style(page1_btn_list[2].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[2].btn, &style_bt1, 0);
                }
                enqueueEvent(EVENT_LED_PRESS);
            
                break;

            case 0x03:

                page1_btn_list[3].state = !page1_btn_list[3].state;
                label = lv_obj_get_child(page1_btn_list[3].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[3].state? "开启" : "关闭");

                if(page1_btn_list[3].state)
                {
                    lv_obj_add_style(page1_btn_list[3].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[3].btn, &style_bt1, 0);
                }
                enqueueEvent(EVENT_TRANSPORT_PRESS);
            
                break;    

            case 0x04:

                page1_btn_list[4].state = !page1_btn_list[4].state;
                label = lv_obj_get_child(page1_btn_list[4].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[4].state? "开启" : "关闭");

                if(page1_btn_list[4].state)
                {
                    lv_obj_add_style(page1_btn_list[4].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[4].btn, &style_bt1, 0);
                }
                enqueueEvent(EVENT_VIBRATOR_PRESS);
        
                break;

            case 0x05:
                
                enqueueEvent(EVENT_CAMERA_PRESS);
        
                break;

            case 0x06:

                page1_btn_list[6].state = !page1_btn_list[6].state;
                label = lv_obj_get_child(page1_btn_list[6].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[6].state? "开启" : "关闭");

                if(page1_btn_list[6].state)
                {
                    lv_obj_add_style(page1_btn_list[6].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[6].btn, &style_bt1, 0);
                }
                enqueueEvent(EVENT_KILL_PARTITION_PRESS);
        
                break;

            case 0x07:

                page1_btn_list[7].state = !page1_btn_list[7].state;
                label = lv_obj_get_child(page1_btn_list[7].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[7].state? "开启" : "关闭");

                if(page1_btn_list[7].state)
                {
                    lv_obj_add_style(page1_btn_list[7].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[7].btn, &style_bt1, 0);
                }
                enqueueEvent(EVENT_BAKE_PARTITION_PRESS);

                break;

            case 0x08:

                page1_btn_list[8].state = !page1_btn_list[8].state;
                label = lv_obj_get_child(page1_btn_list[8].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[8].state? "开启" : "关闭");

                if(page1_btn_list[8].state)
                {
                    lv_obj_add_style(page1_btn_list[8].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[8].btn, &style_bt1, 0);
                }
                enqueueEvent(EVENT_RAIN_PARTITION_PRESS);

                break;

            case 0x09:

                page1_btn_list[9].state = !page1_btn_list[9].state;
                label = lv_obj_get_child(page1_btn_list[9].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[9].state? "开启" : "关闭");

                if(page1_btn_list[9].state)
                {
                    lv_obj_add_style(page1_btn_list[9].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[9].btn, &style_bt1, 0);
                }
                // enqueueEvent(EVENT_KILL_PRESS);//page1.c中会进行处理

                break;

            case 0x0A:

                page1_btn_list[10].state = !page1_btn_list[10].state;
                label = lv_obj_get_child(page1_btn_list[10].btn, 0);
                label_st = lv_obj_get_child(label, 0);
                lv_label_set_text_fmt(label_st, "%s", page1_btn_list[10].state? "开启" : "关闭");

                if(page1_btn_list[10].state)
                {
                    lv_obj_add_style(page1_btn_list[10].btn, &style_bt2, 0);
                }
                else
                {
                    lv_obj_add_style(page1_btn_list[10].btn, &style_bt1, 0);
                }
                enqueueEvent(EVENT_BAKE_PRESS);

                break;

            default:
                // 非法值处理
                break;

            }

        }

        pthread_mutex_unlock(&g_lock);

    }
    
}

void *mqtt_publish_thread(void *arg)
{
    mqtt_client_t *client = (mqtt_client_t *)arg;

/*
    改动一：msg_buf
    msg_buf原来的代码如下：
    char dht1_msg_buf[100] = { 0 };
    char dht2_msg_buf[100] = { 0 };
    char dht3_msg_buf[100] = { 0 };
    char delay_msg_buf[100] = { 0 };
    char rain_msg_buf[100] = { 0 };
*/
//改动后的msg_buf如下：
    char msg_buf[500] = { 0 }; // JSON 格式的消息缓冲区

    mqtt_message_t message;
    memset(&message, 0, sizeof(message));
    message.payload = (void *)msg_buf;

    sleep(2);

    mqtt_list_subscribe_topic(client);

    
    bool sleep_flag_upload;//上传的休眠标志位，活跃状态为0，休眠状态为1

    bool btn_state_list[BUTTON_COUNT];//上传的按钮状态数组，0为关，1为开

    uint32_t light_value_upload;//上传的光照度
    uint8_t dht_data_buf_upload[2];//上传的室内温湿度数组，顺序为：湿度、温度
    uint16_t temp_data_buf_upload[2];//上传的杀虫仓/烘干仓温度数组，顺序为：杀虫仓温度、烘干仓温度

    while (atomic_load(&g_thread_running)) 
    {

        // 获取从纪元开始到现在的秒数，即当前时间
        time_t now = time(NULL);
        // 将 time_t 类型的时间转换为表示本地时间的 struct tm 结构体
        struct tm *tm_info = localtime(&now);
        // 定义一个缓冲区，用于存储格式化后的时间字符串
        char dataTime_buf[20];
        // 把本地时间按照 "YYYY-MM-DD HH:MM:SS" 格式转换为字符串
        strftime(dataTime_buf, sizeof(dataTime_buf), "%Y-%m-%d %H:%M:%S", tm_info);

        //在互斥锁内获取上传的数据
        pthread_mutex_lock(&g_lock);

        sleep_flag_upload = sleep_flag;

        for(int i = 0; i < BUTTON_COUNT; i++)//传递按钮的状态值
        {
            btn_state_list[i] = page1_btn_list[i].state;
        }
        pthread_mutex_unlock(&g_lock);


        pthread_mutex_lock(&light_flag_lock);
        light_value_upload = light_value;
        pthread_mutex_unlock(&light_flag_lock);


        pthread_mutex_lock(&temp_get_lock);
        memcpy(temp_data_buf_upload, temp_data_buf, 4);
        pthread_mutex_unlock(&temp_get_lock);

        pthread_mutex_lock(&dht_get_lock);
        memcpy(dht_data_buf_upload, dht_data_buf, 2);
        pthread_mutex_unlock(&dht_get_lock);


        // 构建 JSON 格式消息
        snprintf(msg_buf, sizeof(msg_buf), "{");

        // 添加设备标识号
        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"deviceKey\":\"%s\",", insectMachineNum);

                 
        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"sleep_flag\":\"%d\",", sleep_flag_upload);

        //"自动模式", "诱虫灯", "补光灯", "移虫装置","震动装置", "拍照", "杀虫隔板", "烘干隔板","虫雨隔板", "杀虫", "烘干"
        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"mode_status\":\"%d\",", btn_state_list[0]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"black_light_status\":\"%d\",", btn_state_list[1]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"led_status\":\"%d\",", btn_state_list[2]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"move_status\":\"%d\",", btn_state_list[3]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"vibrator_status\":\"%d\",", btn_state_list[4]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"kill_partition_status\":\"%d\",", btn_state_list[6]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"bake_partition_status\":\"%d\",", btn_state_list[7]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"rain_partition_status\":\"%d\",", btn_state_list[8]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"kill_status\":\"%d\",", btn_state_list[9]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"bake_status\":\"%d\",", btn_state_list[10]);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"light_value\":\"%u\",", light_value_upload);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"room_temp\":\"Kill:%.1f,Bake:%.1f\",", temp_data_buf_upload[0] / 10.0f, temp_data_buf_upload[1] / 10.0f);

        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"dht\":\"Hum:%d%%,Temp:%d\",", dht_data_buf_upload[0], dht_data_buf_upload[1]);

        // 添加时间戳
        snprintf(msg_buf + strlen(msg_buf), sizeof(msg_buf) - strlen(msg_buf),
                 "\"dataTime\":\"%s\"}", dataTime_buf);

        // 发送 JSON 格式的 MQTT 消息
        message.qos = 0;
        mqtt_publish(client, "insectMachineMessage", &message);

        sleep(5);
    }
}





