#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include "lvgl/lvgl.h"


#include <mqtt_config.h>
#include <mqtt_log.h>
#include <mqttclient.h>

extern void Remote_Control_handler(void* client, message_data_t* msg);
extern void *mqtt_publish_thread(void *arg);