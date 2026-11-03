#include "lvgl/lvgl.h"
#include "lvgl/demos/lv_demos.h"
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>
#include <time.h>

#include "ui_main.h"
//#include "mqtt.h"
#include "serial_init.h"
#include "servo_control.h"

pthread_mutex_t g_lock;

atomic_bool g_thread_running = true;

int fd_dht11,fd_serial,fd_modbus;//dht11的文件句柄

char insectMachineNum[32];//设备标识号


// 信号处理函数
static void signal_handler(int sig) {
    atomic_store(&g_thread_running, false);
}


int main(void)
{

    //注册信号处理器，将Ctrl+C设置为执行我们自己的函数
    signal(SIGINT, signal_handler);

    // fd_dht11 = open("/dev/dht11_1", O_RDWR | O_NONBLOCK);

    /****************************************modbus初始化****************************************************/

    // int iRet_m;

    // fd_modbus = open_port("/dev/ttySC1");

    // iRet_m = set_opt(fd_modbus, 9600, 8, 'N', 1);
	// if (iRet_m)
	// {
	// 	LV_LOG_USER("set port2 err!\n");
	// 	return -1;
	// }

    /****************************************串口初始化****************************************************/

    // int iRet_s;

    // fd_serial = open_port("/dev/ttySC0");

    // iRet_s = set_opt(fd_serial, 4800, 8, 'N', 1);
	// if (iRet_s)
	// {
	// 	LV_LOG_USER("set port1 err!\n");
	// 	return -1;
	// }

    /****************************************MQTT初始化****************************************************/

    // int res;
    // pthread_t thread1;
    // mqtt_client_t *client = NULL;
     FILE *fp;
    
    // LV_LOG_USER("\n MQTT is running!"); 

     fp = fopen("/proc/device-tree/serial-number", "r");
     fgets(insectMachineNum, sizeof(insectMachineNum), fp);
     fclose(fp);

    // // 计算所需缓冲区大小
    // uint8_t topic_len = strlen(insectMachineNum) + strlen("_control") + 1;

    // // 动态分配内存
    // char *Remote_Control_theme = (char*)malloc(topic_len);

    // // 安全拼接远程控制字符串
    // snprintf(Remote_Control_theme, topic_len, "%s_control", insectMachineNum);


    // mqtt_log_init();

    // client = mqtt_lease();

    // #ifdef TEST_USEING_TLS
    // mqtt_set_port(client, "8883");
    // mqtt_set_ca(client, (char*)test_ca_get());
    // #else
    // mqtt_set_port(client, "1883");
    // #endif

    // // mqtt_set_host(client, "iot.100ask.net"); /* 韦东山服务器网址 */
    // mqtt_set_client_id(client, random_string(10));
    // mqtt_set_user_name(client, random_string(10));
    // mqtt_set_password(client, random_string(10));
    // mqtt_set_clean_session(client, 1);

    // mqtt_connect(client);
    
    // //订阅主题
    // //mqtt_subscribe(client, "insectMachineMessage", QOS0, NULL);
    // mqtt_subscribe(client, Remote_Control_theme, QOS0, Remote_Control_handler);
    // // mqtt_subscribe(client, insectMachineNum, QOS0, Remote_Control_handler);

    
    // res = pthread_create(&thread1, NULL, mqtt_publish_thread, client);
    // if(res != 0) {
    //     LV_LOG_USER("create mqtt publish thread fail");
    //     exit(res);
    // }


    /****************************************LVGL初始化****************************************************/

    lv_init();

    /*Linux frame buffer device init*/
    lv_display_t * disp = lv_linux_fbdev_create();
    lv_linux_fbdev_set_file(disp, "/dev/fb0");

    lv_indev_t * touch;
    touch = lv_evdev_create(LV_INDEV_TYPE_POINTER,"/dev/input/event7");

    if (pthread_mutex_init(&g_lock, NULL) != 0) { //初始化互斥锁
         LV_LOG_ERROR("\n mutex init has failed!"); 
         return 0; 
    }

	if(servo_control_init() != 0) {
    	printf("舵机初始化失败\n");
    }

    ui_main();

    /*Handle LVGL tasks*/
    while(atomic_load(&g_thread_running)) {

        pthread_mutex_lock(&g_lock);
        lv_timer_handler();
        pthread_mutex_unlock(&g_lock);

        usleep(5000);
    }

    // close(fd_dht11);

    // close(fd_serial);

    lv_deinit();//释放 LVGL 资源

	servo_control_deinit();

    return 0;
}
