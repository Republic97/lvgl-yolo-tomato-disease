#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <stdlib.h>
#include "lvgl/lvgl.h"


int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop);
int open_port(char *com);
