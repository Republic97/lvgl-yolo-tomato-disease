#include "servo_control.h"
#include <pigpio.h>
#include <stdio.h>

#define PAN_GPIO            18
#define TILT_GPIO           19

#define SERVO_FREQUENCY     50
#define SERVO_MIN_PULSE_US  1000
#define SERVO_MAX_PULSE_US  2000

static int angle_to_pulse_us(int angle)
{
    if(angle < 0) {
        angle = 0;
    }
    else if(angle > 180) {
        angle = 180;
    }

    return SERVO_MIN_PULSE_US +
           angle * (SERVO_MAX_PULSE_US - SERVO_MIN_PULSE_US) / 180;
}

int servo_control_init(void)
{
    if(gpioInitialise() < 0) {
        printf("pigpio 初始化失败\n");
        return -1;
    }

    /* 初始位置放在中间 */
    servo_set_angle(SERVO_PAN, 80);
    servo_set_angle(SERVO_TILT, 80);

    return 0;
}

int servo_set_angle(servo_id_t servo, int angle)
{
    unsigned gpio;
    int pulse_us;
    unsigned duty_ppm;

    if(angle < 0 || angle > 180) {
        return -1;
    }

    gpio = (servo == SERVO_PAN) ? PAN_GPIO : TILT_GPIO;
    pulse_us = angle_to_pulse_us(angle);

    /*
     * gpioHardwarePWM 的占空比范围为 0～1000000。
     * 50 Hz 下：占空比 = pulse_us × 50。
     */
    duty_ppm = (unsigned)pulse_us * SERVO_FREQUENCY;

    if(gpioHardwarePWM(gpio,
                       SERVO_FREQUENCY,
                       duty_ppm) < 0) {
        printf("舵机 PWM 输出失败，GPIO=%u\n", gpio);
        return -1;
    }

    return 0;
}

void servo_control_deinit(void)
{
    gpioHardwarePWM(PAN_GPIO, 0, 0);
    gpioHardwarePWM(TILT_GPIO, 0, 0);
    gpioTerminate();
}
