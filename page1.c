/**
 * @file ui.c
 *
 */

/*********************
 *      INCLUDES
 *********************/

#include "page1.h"
#include "ui_main.h"

// #include <stdatomic.h>
// #include <signal.h>

/*

目前的一个问题    
1.自动低光模式和休眠模式的切换有问题，一是切换休眠模式不够快，二是切换回自动低光模式是否能从中断的地方继续    

解决方案：
编写一个定时检测雨水标志位的for循环，通过宏控制延时时间,当检测到下雨直接退出整个函数

示例代码：

    for (int i = 0; i < time; i++) 
    {
        pthread_mutex_lock(&rain_flag_lock);
        if (is_raining) {
            pthread_mutex_unlock(&rain_flag_lock);
            LV_LOG_USER("Rain detected, exiting function early.\n");
            return; // 直接退出函数
        }

        pthread_mutex_unlock(&rain_flag_lock);

        sleep(1)//延时1s

    }


*/

#if 1

static void page_close_event_handler(lv_event_t * e);
static void btn_event_handler(lv_event_t * e);
static void page1_sleep_detect_timer(lv_timer_t * timer);

lv_obj_t * g_page1_cont;//页面一容器

#define LONG_DURATION 60 //长延时设置为60秒
#define SHORT_DURATION 7 //短延时设置为7秒

bool mode_flag = 0;//模式标志位，自动模式为0，手动模式为1
bool sleep_flag = 0;//休眠标志位，活跃状态为0，休眠状态为1
bool light_flag = 1;//光照标志位，低光为0，高光为1


lv_obj_t * table1;//page1的传感器表格


bool page1_flag = 0;//页面标志位，所处页面为页面一时为1，所处页面为主页面时为0


button_info_t page1_btn_list[BUTTON_COUNT];//全局按钮结构体数组


lv_style_t style_bt1;//全局按钮样式——按下前
lv_style_t style_bt2;//全局按钮样式——按下后

lv_style_t style2;//表格/按钮内字体样式

lv_obj_t * btn_cont;//全局按钮容器


extern pthread_mutex_t g_lock;//全局互斥锁
pthread_mutex_t light_flag_lock,rain_flag_lock,temp_get_lock,dht_get_lock;//高光标志位互斥锁,雨水标志位互斥锁，杀虫仓/烘干仓温度获取互斥锁，dht11数据获取互斥锁
extern atomic_bool g_thread_running;//全局线程运行标志位


//放置用于存储传感器数据的全局变量(光照度、室温、湿度、杀虫仓温度、烘干仓温度、降雨状态) 
uint32_t light_value = 10000;//光照度
uint8_t dht_data_buf[2] = {30,30};//顺序为：湿度、温度
uint16_t temp_data_buf[2] = {30,30};//顺序为：杀虫仓温度、烘干仓温度
bool rain_flag = 0;//降雨标志位，无雨为0，有雨为1
char * rain_str = "无雨";//降雨状态字符串


extern int fd_dht11,fd_serial,fd_modbus;//dht11的文件句柄

//通过串口发送的指令
uint8_t black_light_on[] = {0x02,0x06,0x00,0x00,0x00,0x01,0x48,0x39};
uint8_t black_light_off[] = {0x02,0x06,0x00,0x00,0x00,0x00,0x89,0xf9};
uint8_t led_on[] = {0x01,0x06,0x00,0x05,0x00,0x01,0x58,0x0b};
uint8_t led_off[] = {0x01,0x06,0x00,0x05,0x00,0x00,0x99,0xcb};
uint8_t transport_on[] = {0x01,0x06,0x00,0x07,0x00,0x01,0xf9,0xcb};
uint8_t transport_off[] = {0x01,0x06,0x00,0x07,0x00,0x00,0x38,0x0b};
uint8_t rain_partition_on[] = {0x01,0x06,0x00,0x02,0x00,0x00,0x28,0x0a};
uint8_t rain_partition_off[] = {0x01,0x06,0x00,0x02,0x00,0x01,0xe9,0xca};
uint8_t vibrator_on[] = {0x01,0x06,0x00,0x06,0x00,0x01,0xa8,0x0b};
uint8_t vibrator_off[] = {0x01,0x06,0x00,0x06,0x00,0x00,0x69,0xcb};
uint8_t bake_partition_on[] = {0x01,0x06,0x00,0x04,0x00,0x01,0x09,0xcb};
uint8_t bake_partition_off[] = {0x01,0x06,0x00,0x04,0x00,0x00,0xc8,0x0b};
uint8_t kill_partition_on[] = {0x01,0x06,0x00,0x03,0x00,0x01,0xb8,0x0a};
uint8_t kill_partition_off[] = {0x01,0x06,0x00,0x03,0x00,0x00,0x79,0xca};
uint8_t bake_on[] = {0x01,0x06,0x00,0x09,0x00,0x01,0x98,0x08};
uint8_t bake_off[] = {0x01,0x06,0x00,0x09,0x00,0x00,0x59,0xc8};
uint8_t kill_on[] = {0x01,0x06,0x00,0x08,0x00,0x01,0xc9,0xc8};
uint8_t kill_off[] = {0x01,0x06,0x00,0x08,0x00,0x00,0x08,0x08};

uint8_t first_temp_relay_get[] = {0x01,0x03,0x00,0x00,0x00,0x0b,0x04,0x0d};//获取1号机数据的指令
// uint8_t second_relay_get[] = {0x02,0x03,0x00,0x00,0x00,0x03,0x05,0xf8};//获取2号机数据的指令


//通过modbus发送的指令
uint8_t first_light_get[] = {0x01,0x03,0x00,0x07,0x00,0x01,0x35,0xCB};//获取1号机（光照传感器）数据的指令
uint8_t second_rain_get[] = {0x02,0x03,0x00,0x00,0x00,0x01,0x84,0x39};//获取2号机（雨雪传感器）数据的指令


//杀虫仓、烘干仓的上下限温度
static uint8_t kill_temp_low = 75;
static uint8_t kill_temp_high = 85;
// static int8_t bake_temp_low = 75;
// static int8_t bake_temp_high = 85;

//CRC16校验函数
uint16_t Calculate_CRC16(const uint8_t *data, uint16_t length)
{
    uint16_t crc = 0xFFFF;
    for (uint16_t i = 0; i < length; i++)
    {
        crc ^= data[i]; // 直接 XOR 低字节，因为多项式是反转的
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)              // 检查最低位
                crc = (crc >> 1) ^ 0xA001; // 右移并应用多项式
            else
                crc >>= 1; // 只右移
        }
    }
    return  (crc << 8) | (crc >> 8);//得到的校验值的两个字节和实际是反过来的，需要交换
}



//通过串口发送指令并接收数据的函数
void serial_send_recv(int fd, uint8_t *cmd, uint8_t *recv_buf, int recv_len)
{
    uint8_t recv_temp_buf[30];

    // 发送指令
    write(fd, cmd, 8);

    //当要接收的数据过长时，需通过延时等待接收结束
    if(recv_len > 10)
    {
        // 等待缓冲区接收完数据
        usleep(150000); // 等待150ms
    }
    

    // 接收数据
    int bytes_read = read(fd, recv_temp_buf, recv_len);

    if(bytes_read != recv_len)
    {
        LV_LOG_ERROR("Error: Received %d bytes instead of %d\n", bytes_read, recv_len);
        return;
    }

    if((recv_len == 27) || (recv_len == 7))
    {
        //进行CRC16校验
        uint16_t crc_calculate = Calculate_CRC16(recv_temp_buf, recv_len - 2);
        uint16_t crc_recv = (recv_temp_buf[recv_len - 2] << 8) | recv_temp_buf[recv_len - 1];
        if(crc_calculate != crc_recv)
        {
            LV_LOG_ERROR("CRC16 checksum error\n");
            return;
        }
        else
        {
            memcpy(recv_buf, recv_temp_buf, recv_len - 2);
        }

    }
}

void manual_to_auto_gui_refresh(lv_obj_t *label1,lv_obj_t *label2)//手动模式转自动模式，刷新GUI
{
    for(int i = 1; i < (BUTTON_COUNT - 1); i++)//除模式切换和拍照以外所有按钮的设置
    {
        if(i != 5)
        {
            page1_btn_list[i].state = false;
            lv_obj_add_style(page1_btn_list[i].btn, &style_bt1, 0);
            label1 = lv_obj_get_child(page1_btn_list[i].btn, 0);
            label2 = lv_obj_get_child(label1, 0);
            lv_label_set_text(label2, "关闭");
        }
        
    }
}

//杀虫仓温度控制更新函数
void kill_temp_control_update(uint16_t current_temp) {

    // 控制逻辑
    if ((current_temp / 10) < kill_temp_low) {

        // 启动加热，闭合继电器
        pthread_mutex_lock(&g_lock);
        page1_btn_list[9].state = 1;
        pthread_mutex_unlock(&g_lock);

        enqueueEvent(EVENT_KILL_PRESS);

        
    } else if ((current_temp / 10) > kill_temp_high) {

        // 停止加热，断开继电器
        pthread_mutex_lock(&g_lock);
        page1_btn_list[9].state = 0;
        pthread_mutex_unlock(&g_lock);

        enqueueEvent(EVENT_KILL_PRESS);
        
    }
}

// //烘干仓温度控制更新函数
// void bake_temp_control_update(int8_t current_temp) {

//     // 控制逻辑
//     if ((current_temp / 10.0f) < bake_temp_low) {

//         // 启动加热，闭合继电器
//         pthread_mutex_lock(&g_lock);
//         page1_btn_list[10].state = 1;
//         pthread_mutex_unlock(&g_lock);

//         enqueueEvent(EVENT_BAKE_PRESS);
        
//     } else if ((current_temp / 10.0f) > bake_temp_high) {

//         // 停止加热，断开继电器
//         pthread_mutex_lock(&g_lock);
//         page1_btn_list[10].state = 0;
//         pthread_mutex_unlock(&g_lock);

//         enqueueEvent(EVENT_BAKE_PRESS);
        
//     }
// }


// // 定义事件类型
// typedef enum {

//     EVENT_BLACK_LIGHT_PRESS,  //诱虫灯 
//     EVENT_LED_PRESS, //补光灯 
//     EVENT_TRANSPORT_PRESS, //移虫装置 
//     EVENT_VIBRATOR_PRESS, //震动装置 
//     EVENT_CAMERA_PRESS, //拍照 
//     EVENT_KILL_PARTITION_PRESS, //杀虫隔板 
//     EVENT_BAKE_PARTITION_PRESS, //烘干隔板 
//     EVENT_RAIN_PARTITION_PRESS, //虫雨隔板 
//     EVENT_KILL_PRESS, //杀虫
//     EVENT_BAKE_PRESS, //烘干
//     EVENT_TEMP_GET, //杀虫仓、烘干仓温度获取 
//     EVENT_MAX         // 事件类型总数

// } EventType;


// 事件结构体，包含类型和回调函数指针
typedef struct {
    EventType type;
    void (*callback)(void);  // 指向回调函数
} Event;


// 事件队列（环形缓冲区）
#define EVENT_QUEUE_SIZE 20   //队列最大容量，和事件类型总数无关，当插入事件数量超过该值，停止插入新事件
Event eventQueue[EVENT_QUEUE_SIZE];
uint8_t eventQueueHead = 0;  // 队列头
uint8_t eventQueueTail = 0;  // 队列尾

// 全局回调函数表
static Event callbacks[EVENT_MAX];

// 注册回调函数
void registerCallback(EventType type, void (*callback)(void)) {
    callbacks[type].type = type;
    callbacks[type].callback = callback;
}

// 事件入队
void enqueueEvent(EventType type) {
    // 检查队列是否已满
    if ((eventQueueHead + 1) % EVENT_QUEUE_SIZE == eventQueueTail) {
        //若插入的事件一直没处理，头指针指到了倒数第二的位置时，认为队列已满，不再放入新的事件，直到队列中的事件至少有一件被处理
        LV_LOG_USER("Event queue full\n");
        return;
    }
    eventQueue[eventQueueHead].type = type;
    eventQueue[eventQueueHead].callback = callbacks[type].callback;//先发生的事件先入队
    eventQueueHead = (eventQueueHead + 1) % EVENT_QUEUE_SIZE;//然后移动一位头指针（取模后赋值给自己，不用担心溢出）
}

// 处理事件队列
void processEvents(void) {
    while (eventQueueTail != eventQueueHead) {//当头指针和尾指针重合，说明事件处理完毕
        Event event = eventQueue[eventQueueTail];
        if (event.callback != NULL) {
            event.callback();  //先入队的事件先处理
        }
        eventQueueTail = (eventQueueTail + 1) % EVENT_QUEUE_SIZE;//然后移动一位尾指针（取模后赋值给自己，不用担心溢出）
    }
}

//"自动模式", "诱虫灯", "补光灯", "移虫装置","震动装置", "拍照", "杀虫隔板", "烘干隔板","虫雨隔板", "杀虫", "烘干"
void rain_partition_except_close(void)//除虫雨隔板外全关闭
{
    pthread_mutex_lock(&g_lock);
    page1_btn_list[1].state = 0;
    page1_btn_list[2].state = 0;
    page1_btn_list[3].state = 0;
    page1_btn_list[4].state = 0;
    page1_btn_list[6].state = 0;
    page1_btn_list[7].state = 0;
    page1_btn_list[9].state = 0;
    page1_btn_list[10].state = 0;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_BLACK_LIGHT_PRESS);
    enqueueEvent(EVENT_LED_PRESS);
    enqueueEvent(EVENT_TRANSPORT_PRESS);
    enqueueEvent(EVENT_VIBRATOR_PRESS);
    enqueueEvent(EVENT_KILL_PARTITION_PRESS);
    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);
    enqueueEvent(EVENT_KILL_PRESS);
    enqueueEvent(EVENT_BAKE_PRESS);
    
}

//"自动模式", "诱虫灯", "补光灯", "移虫装置","震动装置", "拍照", "杀虫隔板", "烘干隔板","虫雨隔板", "杀虫", "烘干"
void pest_clean(void)//害虫清理
{

    //打开杀虫和烘干隔板，移虫装置，震动装置
    pthread_mutex_lock(&g_lock);
    page1_btn_list[3].state = 1;
    page1_btn_list[4].state = 1;
    page1_btn_list[6].state = 1;
    page1_btn_list[7].state = 1;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_TRANSPORT_PRESS);
    enqueueEvent(EVENT_VIBRATOR_PRESS);
    enqueueEvent(EVENT_KILL_PARTITION_PRESS);
    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);

    //长延时
    for (int i = 0; i < LONG_DURATION; i++) 
    {
        pthread_mutex_lock(&rain_flag_lock);
        if (rain_flag) {
            pthread_mutex_unlock(&rain_flag_lock);
            LV_LOG_USER("Rain detected, exiting function early.\n");
            return; //直接退出函数
        }
        pthread_mutex_unlock(&rain_flag_lock);

        // pthread_mutex_lock(&g_lock);
        // if (mode_flag) {//发现变更为手动模式，直接退出函数
        //     pthread_mutex_unlock(&g_lock);
        //     LV_LOG_USER("mode_flag modified, exiting function early.\n");
        //     return; // 直接退出函数
        // }
        // pthread_mutex_unlock(&g_lock);

        sleep(1);//延时1s
    }

    //关闭杀虫和烘干隔板，移虫装置，震动装置
    pthread_mutex_lock(&g_lock);
    page1_btn_list[3].state = 0;
    page1_btn_list[4].state = 0;
    page1_btn_list[6].state = 0;
    page1_btn_list[7].state = 0;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_TRANSPORT_PRESS);
    enqueueEvent(EVENT_VIBRATOR_PRESS);
    enqueueEvent(EVENT_KILL_PARTITION_PRESS);
    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);
}

void auto_light_process_1(void)//自动模式高光处理1
{

    //先关闭诱虫灯、补光灯、虫雨隔板、杀虫和烘干
    pthread_mutex_lock(&g_lock);
    page1_btn_list[1].state = 0;
    page1_btn_list[2].state = 0;
    page1_btn_list[8].state = 0;
    page1_btn_list[9].state = 0;
    page1_btn_list[10].state = 0;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_BLACK_LIGHT_PRESS);
    enqueueEvent(EVENT_LED_PRESS);
    enqueueEvent(EVENT_RAIN_PARTITION_PRESS);
    enqueueEvent(EVENT_KILL_PRESS);
    enqueueEvent(EVENT_BAKE_PRESS);

}

void auto_light_process_2(void)//自动模式高光处理2
{
    
    //再关闭剩下的：杀虫和烘干隔板，移虫装置，震动装置
    pthread_mutex_lock(&g_lock);
    page1_btn_list[3].state = 0;
    page1_btn_list[4].state = 0;
    page1_btn_list[6].state = 0;
    page1_btn_list[7].state = 0;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_KILL_PARTITION_PRESS);
    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);
    enqueueEvent(EVENT_TRANSPORT_PRESS);
    enqueueEvent(EVENT_VIBRATOR_PRESS);

}


void sleep_process(void)//休眠状态
{
    rain_partition_except_close();

    //接着开启虫雨隔板
    pthread_mutex_lock(&g_lock);
    page1_btn_list[8].state = 1;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_RAIN_PARTITION_PRESS);
}

void auto_dark_init(uint16_t init_temp_buf[2])//自动模式低光状态，初始化
{
    rain_partition_except_close();
    //接着关闭虫雨隔板，开启杀虫继电器
    pthread_mutex_lock(&g_lock);
    page1_btn_list[8].state = 0;
    page1_btn_list[9].state = 1;
    // page1_btn_list[10].state = 1;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_RAIN_PARTITION_PRESS);
    enqueueEvent(EVENT_KILL_PRESS);
    // enqueueEvent(EVENT_BAKE_PRESS);


    //预热，循环直到杀虫仓达到了最低温度
    while ((init_temp_buf[0] / 10) < kill_temp_low) 
    {
        pthread_mutex_lock(&temp_get_lock);
        memcpy(init_temp_buf, temp_data_buf, 2);
        pthread_mutex_unlock(&temp_get_lock);

        pthread_mutex_lock(&rain_flag_lock);
        if (rain_flag) {
            pthread_mutex_unlock(&rain_flag_lock);
            LV_LOG_USER("Rain detected, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&rain_flag_lock);

        pthread_mutex_lock(&g_lock);
        if (mode_flag) {//发现变更为手动模式，直接退出函数
            pthread_mutex_unlock(&g_lock);
            LV_LOG_USER("mode_flag modified, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&g_lock);
       
        sleep(1);//延时1s
    }

    //接着开启引虫灯/补光灯/移虫装置/震动装置
    pthread_mutex_lock(&g_lock);
    page1_btn_list[1].state = 1;
    page1_btn_list[2].state = 1;
    page1_btn_list[3].state = 1;
    page1_btn_list[4].state = 1;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_BLACK_LIGHT_PRESS);
    enqueueEvent(EVENT_LED_PRESS);
    enqueueEvent(EVENT_TRANSPORT_PRESS);
    enqueueEvent(EVENT_VIBRATOR_PRESS);

    //长延时，杀虫
    for (int i = 0; i < LONG_DURATION; i++) 
    {

        pthread_mutex_lock(&rain_flag_lock);
        if (rain_flag) {
            pthread_mutex_unlock(&rain_flag_lock);
            LV_LOG_USER("Rain detected, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&rain_flag_lock);

        pthread_mutex_lock(&g_lock);
        if (mode_flag) {//发现变更为手动模式，直接退出函数
            pthread_mutex_unlock(&g_lock);
            LV_LOG_USER("mode_flag modified, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&g_lock);

        sleep(1);//延时1s
    }
}

void auto_dark_process(uint16_t in_temp_data_buf[2])//自动模式低光状态，处理流程
{
    //更新杀虫仓的温度控制
    kill_temp_control_update(in_temp_data_buf[0]);//杀虫仓
    // bake_temp_control_update(in_temp_data_buf[1]);//烘干仓

    //打开杀虫隔板
    pthread_mutex_lock(&g_lock);
    page1_btn_list[6].state = 1;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_KILL_PARTITION_PRESS);

    //短延时
    for(int i = 0; i < SHORT_DURATION; i++) 
    {

        pthread_mutex_lock(&rain_flag_lock);
        if (rain_flag) {
            pthread_mutex_unlock(&rain_flag_lock);
            LV_LOG_USER("Rain detected, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&rain_flag_lock);

        pthread_mutex_lock(&g_lock);
        if (mode_flag) {//发现变更为手动模式，直接退出函数
            pthread_mutex_unlock(&g_lock);
            LV_LOG_USER("mode_flag modified, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&g_lock);

        sleep(1);//延时1s
    }

    //关闭杀虫隔板
    pthread_mutex_lock(&g_lock);
    page1_btn_list[6].state = 0;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_KILL_PARTITION_PRESS);


    //打开烘干装置
    pthread_mutex_lock(&g_lock);
    page1_btn_list[10].state = 1;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);


    //长延时（杀虫兼烘干）
    for(int i = 0; i < LONG_DURATION; i++) 
    {

        pthread_mutex_lock(&rain_flag_lock);
        if (rain_flag) {
            pthread_mutex_unlock(&rain_flag_lock);
            LV_LOG_USER("Rain detected, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&rain_flag_lock);

        pthread_mutex_lock(&g_lock);
        if (mode_flag) {//发现变更为手动模式，直接退出函数
            pthread_mutex_unlock(&g_lock);
            LV_LOG_USER("mode_flag modified, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&g_lock);

        sleep(1);//延时1s
    }

    //关闭烘干装置
    pthread_mutex_lock(&g_lock);
    page1_btn_list[10].state = 0;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);


    //打开烘干隔板
    pthread_mutex_lock(&g_lock);
    page1_btn_list[7].state = 1;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);

    //短延时
    for(int i = 0; i < SHORT_DURATION; i++) 
    {

        pthread_mutex_lock(&rain_flag_lock);
        if (rain_flag) {
            pthread_mutex_unlock(&rain_flag_lock);
            LV_LOG_USER("Rain detected, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&rain_flag_lock);

        pthread_mutex_lock(&g_lock);
        if (mode_flag) {//发现变更为手动模式，直接退出函数
            pthread_mutex_unlock(&g_lock);
            LV_LOG_USER("mode_flag modified, exiting function early.\n");
            return; // 直接退出函数
        }
        pthread_mutex_unlock(&g_lock);

        sleep(1);//延时1s
    }

    //关闭烘干隔板
    pthread_mutex_lock(&g_lock);
    page1_btn_list[7].state = 0;
    pthread_mutex_unlock(&g_lock);

    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);


    enqueueEvent(EVENT_CAMERA_PRESS);//拍照上传

}


//"自动模式", "诱虫灯", "补光灯", "移虫装置","震动装置", "拍照", "杀虫隔板", "烘干隔板","虫雨隔板", "杀虫", "烘干"
void black_light_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[1].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, black_light_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, black_light_off, NULL, 8);
    }

    usleep(50000);

}

void LED_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[2].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, led_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, led_off, NULL, 8);
    }

    usleep(50000);

}

void transport_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[3].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, transport_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, transport_off, NULL, 8);
    }

    usleep(50000);

}

void vibrator_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[4].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, vibrator_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, vibrator_off, NULL, 8);
    }

    usleep(50000);

}

void camera_PRESS_Handler(void) {

    int ret = system("python3 /home/wuyue/Desktop/lvgl_v9_demo/camera/camera.py");

    if (ret != 0) {
        LV_LOG_ERROR("\n camera.py execution failed!"); 
    }

    LV_LOG_USER("\n The photo has been taken successfully!"); 
}

void kill_partition_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[6].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, kill_partition_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, kill_partition_off, NULL, 8);
    }

    usleep(100000);//单片机处理电机动作需要时间，延时100ms

}

void bake_partition_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[7].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, bake_partition_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, bake_partition_off, NULL, 8);
    }

    usleep(100000);//单片机处理电机动作需要时间，延时100ms

}

void rain_partition_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[8].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, rain_partition_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, rain_partition_off, NULL, 8);
    }

    usleep(100000);//单片机处理电机动作需要时间，延时100ms

}

void kill_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[9].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, kill_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, kill_off, NULL, 8);
    }

    usleep(50000);

}

void bake_PRESS_Handler(void) {

    pthread_mutex_lock(&g_lock);
    if(page1_btn_list[10].state)//开启
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, bake_on, NULL, 8);
    }
    else//关闭
    {
        pthread_mutex_unlock(&g_lock);
        serial_send_recv(fd_serial, bake_off, NULL, 8);
    }

    usleep(50000);

}

// void Temp_Get_Handler(void) {

//    //通过串口获取含温度/继电器的数据
//     serial_send_recv(fd_serial, first_temp_relay_get, long_recv_buf, 27);

// }



//全局按钮样式初始化
void style_bt_init(void)
{
    lv_style_init(&style_bt1);
    lv_style_set_bg_color(&style_bt1, lv_palette_main(LV_PALETTE_GREY));
    lv_style_set_bg_opa(&style_bt1, LV_OPA_50);//按钮按下前的样式

    lv_style_init(&style_bt2);
    lv_style_set_bg_color(&style_bt2, lv_palette_main(LV_PALETTE_BLUE));
    lv_style_set_bg_opa(&style_bt2, LV_OPA_50);//按钮按下后的样式
}

void sub_page_top_bar_init(lv_obj_t * page_cont, const char *text, void page_close_event_handler(lv_event_t * e))
{
    lv_obj_t * label;

    lv_font_t * font1 = lv_freetype_font_create("A:/home/wuyue/Desktop/lvgl_v9_demo/apple_black.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        40,
        LV_FREETYPE_FONT_STYLE_NORMAL);//顶栏字体路径
        
    lv_font_t * font2 = lv_freetype_font_create("A:/home/wuyue/Desktop/lvgl_v9_demo/apple_black.ttf",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        20,
        LV_FREETYPE_FONT_STYLE_NORMAL);//表格/按钮内字体路径

    lv_font_t * font_aw = lv_freetype_font_create("A:/home/wuyue/Desktop/lvgl_v9_demo/FontAwesome5.woff",
        LV_FREETYPE_FONT_RENDER_MODE_BITMAP,
        40,
        LV_FREETYPE_FONT_STYLE_NORMAL);//返回/4G标志字体路径
        
    static lv_style_t style1;
    lv_style_init(&style1);
    lv_style_set_text_font(&style1, font1);//顶栏字体样式

    label = lv_label_create(page_cont);
    lv_obj_add_style(label, &style1, 0);
    lv_label_set_text_fmt(label, "%s", text);   
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);//"运行状态"标签

    static lv_style_t style_4G;
    lv_style_init(&style_4G);
    lv_style_set_text_font(&style_4G, font_aw);//4G标志样式

    //label = lv_label_create(page_cont);
    //lv_obj_add_style(label, &style_4G, 0);
    //lv_label_set_text(label, "\xEF\x80\x92");//4G标志utf-8
    //lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -10, 10);//4G标志位置

    static lv_style_t style_BK;
    lv_style_init(&style_BK);
    lv_style_set_text_font(&style_BK, font_aw);//返回标志样式

    label = lv_label_create(page_cont);
    lv_obj_add_flag(label, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_style(label, &style_BK, 0);
    lv_label_set_text(label, "\xEF\x81\x93");//返回标志utf-8 
    lv_obj_align(label, LV_ALIGN_TOP_LEFT, 10, 10);//返回标志位置
    lv_obj_add_event_cb(label, page_close_event_handler, LV_EVENT_CLICKED, NULL);

    static lv_style_t style_line;
    lv_style_init(&style_line);
    lv_style_set_line_width(&style_line, 2);//分隔线的样式，宽度为2

    static lv_point_precise_t line_points[] = {{0, 70}, {796, 70}};
    lv_obj_t * line = lv_line_create(page_cont);
    lv_line_set_points(line, line_points, 2);
    lv_obj_add_style(line, &style_line, 0);//顶栏和内容的分隔线

    lv_style_init(&style2);
    lv_style_set_text_font(&style2, font2);//表格/按钮内字体样式

}

void sub_page_table_bt_cont_init(lv_obj_t * *table, lv_obj_t * page_cont)
{

    *table = lv_table_create(page_cont);
    lv_obj_add_style(*table, &style2, 0);

    lv_table_set_cell_value_fmt(*table, 0, 0, "光照度\n%d LUX",light_value);
    lv_table_set_cell_value_fmt(*table, 0, 1, "室温\n%d ℃",dht_data_buf[1]);
    lv_table_set_cell_value_fmt(*table, 0, 2, "湿度\n%d %%",dht_data_buf[0]);
    lv_table_set_cell_value_fmt(*table, 0, 3, "杀虫仓温度\n%d ℃",temp_data_buf[0]);
    lv_table_set_cell_value_fmt(*table, 0, 4, "烘干仓温度\n%d ℃",temp_data_buf[1]);
    lv_table_set_cell_value_fmt(*table, 0, 5, "降雨状态\n%s",rain_str);

    lv_table_set_column_width(*table, 0, 160);
    lv_table_set_column_width(*table, 1, 110);
    lv_table_set_column_width(*table, 2, 110);
    lv_table_set_column_width(*table, 3, 140);
    lv_table_set_column_width(*table, 4, 140);
    lv_table_set_column_width(*table, 5, 120);//设置表格列宽

    lv_obj_align(*table, LV_ALIGN_TOP_MID, 0, 75);//表格放置位置
    
    btn_cont = lv_obj_create(page_cont);
    lv_obj_set_size(btn_cont, lv_pct(78), 230);
    lv_obj_set_style_pad_all(btn_cont, 0, 0);//去除容器内的填充
    lv_obj_align(btn_cont, LV_ALIGN_CENTER, 68, 90);//按钮矩阵位置
    lv_obj_set_style_border_opa(btn_cont, 0, 0);//隐藏按钮容器边框

    lv_obj_set_flex_flow(btn_cont, LV_FLEX_FLOW_ROW_WRAP); // 水平排列
    lv_obj_set_flex_align(btn_cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
    lv_obj_set_style_pad_gap(btn_cont, 15, 0);

}




/*
传感器数据获取/单次拍照线程函数
*/
static void * sensor_thread_function(void *arg)
{

    LV_LOG_USER("\n sensor_thread is running!");

    uint8_t light_recv_buf[10];//用于存储光照传感器返回数据的数组
    uint8_t rain_recv_buf[10];//用于存储雨雪传感器返回数据的数组

    while(atomic_load(&g_thread_running))
    {

        //通过modbus获取光照传感器和雨水传感器的数据
        serial_send_recv(fd_modbus, first_light_get, light_recv_buf, 7);
        usleep(50000);//延时50ms
        serial_send_recv(fd_modbus, second_rain_get, rain_recv_buf, 7);


        //根据光照传感器的数据更新光照标志位
        pthread_mutex_lock(&light_flag_lock);
        //将获取的16进制光照值转换为十进制，并存储到light_value变量中
        light_value = (light_recv_buf[3] << 8) | light_recv_buf[4];

        //在互斥锁内获取光照传感器的数据
        if(light_value > 10)//高光
        {
            light_flag = 1;

        }
        else//低光
        {
            light_flag = 0;
        }
        pthread_mutex_unlock(&light_flag_lock);


        //根据雨水传感器的数据更新雨水标志位
        pthread_mutex_lock(&rain_flag_lock);
        if(rain_recv_buf[4])//下雨
        {
            rain_flag = 1;
        }
        else//无雨
        {
            rain_flag = 0;
        }
        pthread_mutex_unlock(&rain_flag_lock);

        //根据雨水标志位更新降雨状态字符串
        if(!rain_flag)
        {
            rain_str = "无雨";
        }
        else
        {
            rain_str = "有雨";
        }

        pthread_mutex_lock(&dht_get_lock);
        if (read(fd_dht11, dht_data_buf, 2) == 2);//读取dht11的数据 
        else
        {
            LV_LOG_USER("\n dht11 read error!"); 
        }
        pthread_mutex_unlock(&dht_get_lock);


        pthread_mutex_lock(&g_lock);
        lv_table_set_cell_value_fmt(table1, 0, 0, "光照度\n%d LUX",light_value);
        lv_table_set_cell_value_fmt(table1, 0, 1, "室温\n%d ℃",dht_data_buf[1]);
        lv_table_set_cell_value_fmt(table1, 0, 2, "湿度\n%d %%",dht_data_buf[0]);
        lv_table_set_cell_value_fmt(table1, 0, 5, "降雨状态\n%s",rain_str);
        pthread_mutex_unlock(&g_lock);
 
        usleep(500000);//延时500ms
    }

    LV_LOG_USER("\n sensor_thread has exited!"); 


    return NULL;
 
}


/*
自动模式/手动模式动作线程函数
*/
static void * action_thread_function(void *arg)
{

    LV_LOG_USER("\n action_thread is running!");

    bool in_mode_flag = 0;//内部模式标志位，自动模式为0，手动模式为1
    bool in_sleep_flag = 0;//内部休眠标志位，活跃状态为0，休眠状态为1

    bool in_light_flag_o = 1;//内部高光标志位，低光为0，高光为1
    bool in_light_flag_n = 1;//内部高光标志位，低光为0，高光为1

    bool auto_light_process_flag = 0;//高光处理标志位，未处理为0，已处理为1
    bool auto_dark_init_flag = 0;//低光初始化标志位，未初始化为0，已初始化为1
    bool sleep_process_flag = 0;//休眠处理标志位，未处理为0，已处理为1
    bool manual_dark_process_flag = 0;//低光手动处理标志位，未处理为0，已处理为1
    bool in_kill_control_flag = 0;//内部杀虫控制标志位，停止控制为0，开始控制为1
    bool in_bake_control_flag = 0;//内部烘干控制标志位，停止控制为0，开始控制为1
    bool kill_close_flag = 1;//杀虫关闭标志位，未执行为0，已执行为1
    // bool bake_close_flag = 1;//烘干关闭标志位，未执行为0，已执行为1

    uint16_t in_temp_data_buf[2];//内部获取到的温度值，顺序为：杀虫仓温度、烘干仓温度


    while(atomic_load(&g_thread_running))
    {
        pthread_mutex_lock(&g_lock);
        in_mode_flag = mode_flag;
        in_sleep_flag = sleep_flag;
        in_kill_control_flag = page1_btn_list[9].state;
        in_bake_control_flag = page1_btn_list[10].state;
        pthread_mutex_unlock(&g_lock);
        

        pthread_mutex_lock(&light_flag_lock);
        in_light_flag_o = in_light_flag_n;
        in_light_flag_n = light_flag;
        pthread_mutex_unlock(&light_flag_lock);


        pthread_mutex_lock(&temp_get_lock);
        memcpy(in_temp_data_buf, temp_data_buf, 2);
        pthread_mutex_unlock(&temp_get_lock);

        if(!in_sleep_flag)//活跃状态
        {
            if(!in_mode_flag)//自动模式
            {

                if(!in_light_flag_n)//低光，开始执行自动处理流程
                {
                    auto_light_process_flag = 0;

                    if(!auto_dark_init_flag)
                    {
                        auto_dark_init(in_temp_data_buf);//开启引虫灯/补光灯/移虫装置/震动装置/杀虫/烘干
                        auto_dark_init_flag = 1;
                    }
                    auto_dark_process(in_temp_data_buf);//对隔板按时间顺序操作，并拍照上传
                }
                else//高光，关闭一切设备
                {
                    auto_dark_init_flag = 0;

                    if(!auto_light_process_flag)
                    {
                        auto_light_process_1();//先执行公共部分

                        if(!(in_light_flag_n > in_light_flag_o))//不是由暗转亮
                        {
                            auto_light_process_2();//关闭剩下的部分
                        }
                        else//由暗转亮
                        {
                            pest_clean();
                        }

                        auto_light_process_flag = 1;
                    }
                }

                manual_dark_process_flag = 0;//自动模式及时清除低光手动处理的标志位

            }
            else//手动模式
            {
                /*
                无雨天气，自动模式切换为手动模式时，清除自动模式的标志位
                */
                auto_light_process_flag = 0;
                auto_dark_init_flag = 0;

                if(in_kill_control_flag)//开启带保护的杀虫控制
                {
                    kill_temp_control_update(in_temp_data_buf[0]);

                    kill_close_flag = 0;//及时对杀虫关闭标志位清零
                }
                else//关闭杀虫继电器
                {
                    if(!kill_close_flag)
                    {
                        // pthread_mutex_lock(&g_lock);
                        // page1_btn_list[9].state = 0;
                        // pthread_mutex_unlock(&g_lock);
                        enqueueEvent(EVENT_KILL_PRESS);

                        kill_close_flag = 1;
                    }
                }


                // if(in_bake_control_flag)//开启带保护的烘干控制
                // {
                //     bake_temp_control_update(in_temp_data_buf[1]);

                //     bake_close_flag = 0;//及时对烘干关闭标志位清零
                // }
                // else//关闭烘干继电器
                // {
                //     if(!bake_close_flag)
                //     {
                //         // pthread_mutex_lock(&g_lock);
                //         // page1_btn_list[10].state = 0;
                //         // pthread_mutex_unlock(&g_lock);
                //         enqueueEvent(EVENT_BAKE_PRESS);

                //         bake_close_flag = 1;
                //     }
                // }

                if(!manual_dark_process_flag)
                {
                    if(!in_light_flag_n)
                    {
                        //低光，自动转手动，关闭除虫雨挡板以外的所有组件（无雨状态下，自动模式的虫雨挡板默认关闭）
                        rain_partition_except_close();

                        pthread_mutex_lock(&g_lock);
                        for(int i = 0; i < BUTTON_COUNT; i++)//所有按钮的设置
                        {
                            if((i != 0) && (i != 5))//更新除模式切换和拍照以外的所有按钮状态
                            {
                                page1_btn_list[i].state = false;
                            }
                        }
                        pthread_mutex_unlock(&g_lock);
                    }

                    manual_dark_process_flag = 1;
                }
            }

            sleep_process_flag = 0;//活跃状态及时清除休眠状态的标志位

        }
        else//休眠状态
        {
            /*
            有雨天气，自动模式会切换为休眠模式，此时需要清除自动模式的标志位
            */
            auto_light_process_flag = 0;
            auto_dark_init_flag = 0;


            if(!sleep_process_flag)
            {
                sleep_process();//执行休眠状态的动作

                sleep_process_flag = 1;
            }

        }

        usleep(500000);//延时500ms

    }

    LV_LOG_USER("\n action_thread has exited!"); 

    return NULL;
     
}

//返回的数组中温度/继电器状态所在位置

//short_recv_buf[3] 2号机诱虫灯状态在返回数组中的位置

//long_recv_buf[4] 1号机杀虫仓温度在返回数组中的位置
//long_recv_buf[6] 1号机烘干仓温度在返回数组中的位置

//long_recv_buf[13] 1号机补光灯状态在返回数组中的位置
//long_recv_buf[17] 1号机移虫装置状态在返回数组中的位置
//long_recv_buf[7] 1号机虫雨挡板状态在返回数组中的位置
//long_recv_buf[15] 1号机震动装置状态在返回数组中的位置
//long_recv_buf[11] 1号机烘干挡板状态在返回数组中的位置
//long_recv_buf[9] 1号机杀虫挡板状态在返回数组中的位置
static void * temp_relay_get_thread_function(void *arg)
{

    LV_LOG_USER("\n temp_relay_get_thread is running!"); 

    uint8_t long_recv_buf[30];//用于存储1号机返回的较长原始数据的数组
    // uint8_t short_recv_buf[12];//2号机返回的较短原始数据

    while(atomic_load(&g_thread_running))
    {
        processEvents();//处理事件队列  

        //通过串口获取含温度/继电器的数据
        // enqueueEvent(EVENT_TEMP_GET);
        serial_send_recv(fd_serial, first_temp_relay_get, long_recv_buf, 27);
        // serial_send_recv(fd_serial, second_relay_get, short_recv_buf, 11);

        pthread_mutex_lock(&temp_get_lock);
        temp_data_buf[0] = (long_recv_buf[3] << 8) | long_recv_buf[4];
        temp_data_buf[1] = (long_recv_buf[5] << 8) | long_recv_buf[6];//将数组中的温度数据复制给全局数组temp_data_buf
        pthread_mutex_unlock(&temp_get_lock);


        pthread_mutex_lock(&g_lock);
        lv_table_set_cell_value_fmt(table1, 0, 3, "杀虫仓温度\n%d ℃",temp_data_buf[0] / 10);
        lv_table_set_cell_value_fmt(table1, 0, 4, "烘干仓温度\n%d ℃",temp_data_buf[1] / 10);
        //16进制转换成实际温度值，保留一位小数
        pthread_mutex_unlock(&g_lock);

        usleep(500000);//延时500ms
    }

    LV_LOG_USER("\n temp_relay_get_thread has exited!"); 

    return NULL;
     
}
    


void page_1(void)
{
    /*******************************************事件驱动初始化***********************************************************/

    registerCallback(EVENT_BLACK_LIGHT_PRESS, black_light_PRESS_Handler);
    registerCallback(EVENT_LED_PRESS, LED_PRESS_Handler);
    registerCallback(EVENT_TRANSPORT_PRESS, transport_PRESS_Handler);
    registerCallback(EVENT_VIBRATOR_PRESS, vibrator_PRESS_Handler);
    registerCallback(EVENT_CAMERA_PRESS, camera_PRESS_Handler);
    registerCallback(EVENT_KILL_PARTITION_PRESS, kill_partition_PRESS_Handler);
    registerCallback(EVENT_BAKE_PARTITION_PRESS, bake_partition_PRESS_Handler);
    registerCallback(EVENT_RAIN_PARTITION_PRESS, rain_partition_PRESS_Handler);
    registerCallback(EVENT_KILL_PRESS, kill_PRESS_Handler);
    registerCallback(EVENT_BAKE_PRESS, bake_PRESS_Handler);
    // registerCallback(EVENT_TEMP_GET, Temp_Get_Handler);

    /*******************************************页面部分***********************************************************/


    g_page1_cont = lv_obj_create(lv_screen_active());
    lv_obj_set_size(g_page1_cont, lv_pct(100), lv_pct(100));
    lv_obj_set_style_pad_all(g_page1_cont, 0, 0);//去除容器内的填充

    sub_page_top_bar_init(g_page1_cont, "运行状态", page_close_event_handler);//子页面顶栏部分初始化

    
    sub_page_table_bt_cont_init(&table1, g_page1_cont);//子页面表格和按钮容器部分初始化
    

    style_bt_init();//全局按钮样式初始化

    lv_timer_t * timer = lv_timer_create(page1_sleep_detect_timer, 500, NULL);//添加定时器

    const char * bt_labels[11] = {"自动模式", "诱虫灯", "补光灯", "移虫装置","震动装置", "拍照", "杀虫隔板", "烘干隔板","虫雨隔板", "杀虫", "烘干"};//按钮标签内容数组

    lv_obj_t * label;

    for(int i = 0; i < BUTTON_COUNT; i++)//所有按钮的设置
    {
        if(i == 0)
        {
            page1_btn_list[i].btn = lv_button_create(g_page1_cont);

            lv_obj_add_flag(page1_btn_list[i].btn, LV_OBJ_FLAG_CHECKABLE);
            lv_obj_add_style(page1_btn_list[i].btn, &style_bt2, 0);
            lv_obj_add_style(page1_btn_list[i].btn, &style_bt2, LV_STATE_CHECKED);
            lv_obj_set_size(page1_btn_list[i].btn, 110, 225);
        }
        else
        {
            page1_btn_list[i].btn = lv_button_create(btn_cont);

            lv_obj_add_style(page1_btn_list[i].btn, &style_bt1, 0);
            lv_obj_set_size(page1_btn_list[i].btn, 110, 105);
        }

        page1_btn_list[i].state = false;

        page1_btn_list[i].label = bt_labels[i];
        label = lv_label_create(page1_btn_list[i].btn);
        lv_obj_add_style(label, &style2, 0);
              
        lv_label_set_text_fmt(label, "%s", page1_btn_list[i].label);

        if((i != 0) && (i != 5))
        {
            lv_obj_t * label_st = lv_label_create(label);
            lv_obj_add_style(label, &style2, 0);
            lv_label_set_text(label_st, "关闭");
            lv_obj_align_to(label_st, label,LV_ALIGN_OUT_BOTTOM_MID,0,0);

        }

        lv_obj_center(label);

        lv_obj_add_event_cb(page1_btn_list[i].btn, btn_event_handler, LV_EVENT_CLICKED, &page1_btn_list[i]);

    }
    
    lv_obj_align_to(page1_btn_list[0].btn, btn_cont, LV_ALIGN_OUT_LEFT_MID, -15, 0);

    lv_obj_set_user_data(page1_btn_list[0].btn, page1_btn_list[5].btn);

    /*******************************************线程部分***********************************************************/

    if (pthread_mutex_init(&light_flag_lock, NULL) != 0) { //初始化光照标志互斥锁
        LV_LOG_ERROR("\n mutex init has failed!"); 
        return; 
    }

    if (pthread_mutex_init(&rain_flag_lock, NULL) != 0) { //初始化雨水标志互斥锁
        LV_LOG_ERROR("\n mutex init has failed!"); 
        return; 
    }

    if (pthread_mutex_init(&temp_get_lock, NULL) != 0) { //初始化杀虫/烘干仓温度获取互斥锁
        LV_LOG_ERROR("\n mutex init has failed!"); 
        return; 
    }

    if (pthread_mutex_init(&dht_get_lock, NULL) != 0) { //初始化dht11数据获取互斥锁
        LV_LOG_ERROR("\n mutex init has failed!"); 
        return; 
    }

    pthread_t sensor_thread_id;
    pthread_create(&sensor_thread_id, NULL, sensor_thread_function, NULL);
    pthread_detach(sensor_thread_id);  // 设置为自动回收

    pthread_t action_thread_id;
    pthread_create(&action_thread_id, NULL, action_thread_function, NULL);
    pthread_detach(action_thread_id);  // 设置为自动回收

    pthread_t temp_relay_get_thread_id;
    pthread_create(&temp_relay_get_thread_id, NULL, temp_relay_get_thread_function, NULL);
    pthread_detach(temp_relay_get_thread_id);  // 设置为自动回收

}

static void btn_event_handler(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);
    lv_obj_t * label_st = lv_obj_get_child(label, 0);

    lv_obj_t * btn_cam = lv_obj_get_user_data(btn);

    button_info_t * btn_info = (button_info_t *)lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {

        if(!(strcmp(lv_label_get_text(label), "手动模式") && strcmp(lv_label_get_text(label), "自动模式")))
        {
            btn_info->state = !btn_info->state;
            lv_label_set_text_fmt(label, "%s", btn_info->state ? "手动模式" : "自动模式");
            mode_flag = !mode_flag;

            if(mode_flag)//手动模式
            {
                lv_obj_add_style(btn_cam, &style_bt2, 0);
                
            }
            else//自动模式
            {
                lv_obj_add_style(btn_cam, &style_bt1, 0);

                manual_to_auto_gui_refresh(label,label_st);//设置除模式切换和拍照以外的所有按钮
                
            }
        }
        else 
        {
            if(mode_flag)
            {
                if(strcmp(lv_label_get_text(label), "拍照") != 0)//手动模式
                {
                    btn_info->state = !btn_info->state;
                    lv_label_set_text_fmt(label_st, "%s", btn_info->state ? "开启" : "关闭");
                    if(btn_info->state)
                    {
                        lv_obj_add_style(btn, &style_bt2, 0);
                    }
                    else
                    {
                        lv_obj_add_style(btn, &style_bt1, 0);
                    }

                    if(strcmp(lv_label_get_text(label), "诱虫灯") == 0)
                    enqueueEvent(EVENT_BLACK_LIGHT_PRESS);
                    else if(strcmp(lv_label_get_text(label), "补光灯") == 0)
                    enqueueEvent(EVENT_LED_PRESS);
                    else if(strcmp(lv_label_get_text(label), "移虫装置") == 0)
                    enqueueEvent(EVENT_TRANSPORT_PRESS);
                    else if(strcmp(lv_label_get_text(label), "震动装置") == 0)
                    enqueueEvent(EVENT_VIBRATOR_PRESS);
                    else if(strcmp(lv_label_get_text(label), "杀虫隔板") == 0)
                    enqueueEvent(EVENT_KILL_PARTITION_PRESS);
                    else if(strcmp(lv_label_get_text(label), "烘干隔板") == 0)
                    enqueueEvent(EVENT_BAKE_PARTITION_PRESS);
                    else if(strcmp(lv_label_get_text(label), "虫雨隔板") == 0)
                    enqueueEvent(EVENT_RAIN_PARTITION_PRESS);
                    else if(strcmp(lv_label_get_text(label), "烘干") == 0)
                    enqueueEvent(EVENT_BAKE_PRESS);

                }
                else//手动模式拍照
                {
                    enqueueEvent(EVENT_CAMERA_PRESS);
                }
                
            }
            
        }

    }
    
}

static uint16_t page_close(void)
{
   lv_obj_add_flag(g_page1_cont, LV_OBJ_FLAG_HIDDEN);
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

static void page1_sleep_detect_timer(lv_timer_t * timer)
{
    bool in_rain_flag = 0;//内部雨水标志位，无雨为0，有雨为1

    pthread_mutex_lock(&rain_flag_lock);
    in_rain_flag = rain_flag;
    pthread_mutex_unlock(&rain_flag_lock);

    if(in_rain_flag && !sleep_flag)//有雨且未进入休眠状态
    {
    
        sleep_flag = 1;
    
        if(page1_flag)
        {

            lv_obj_add_flag(g_page1_cont, LV_OBJ_FLAG_HIDDEN);
    
            lv_my_ui_page_rain_t.open();//进入休眠状态

        }
    
    }
    else if(!in_rain_flag && sleep_flag)//无雨且仍处于休眠状态
    {
        
        sleep_flag = 0;
    
        if(page1_flag)
        {
            lv_obj_remove_flag(g_page1_cont, LV_OBJ_FLAG_HIDDEN);
            lv_timer_delete(lv_obj_get_user_data(lv_my_ui_page_rain_t.page_cont));
            lv_obj_delete(lv_my_ui_page_rain_t.page_cont);
        }
        

        //若为手动模式，默认修改为全关闭
        if(mode_flag)
        {
            lv_obj_t * label;
            lv_obj_t * label_st;

            manual_to_auto_gui_refresh(label,label_st);//设置除模式切换和拍照以外的所有按钮

            page1_btn_list[8].state = 0;
            enqueueEvent(EVENT_RAIN_PARTITION_PRESS);//从有雨到无雨，对于手动模式只剩虫雨挡板未关闭，此处进行关闭
        }
        
    }
} 
#endif
 
