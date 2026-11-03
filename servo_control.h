#ifndef SERVO_CONTROL_H
#define SERVO_CONTROL_H

typedef enum {
    SERVO_PAN,     /* 左右舵机 */
    SERVO_TILT     /* 上下舵机 */
} servo_id_t;

int servo_control_init(void);
int servo_set_angle(servo_id_t servo, int angle);
void servo_control_deinit(void);

#endif
