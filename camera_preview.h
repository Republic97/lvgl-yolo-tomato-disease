#ifndef CAMERA_PREVIEW_H
#define CAMERA_PREVIEW_H

#include "lvgl/lvgl.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct camera_preview camera_preview_t;

typedef enum {
    CAMERA_PREVIEW_STATE_IDLE,
    CAMERA_PREVIEW_STATE_CONNECTING,
    CAMERA_PREVIEW_STATE_STREAMING,
    CAMERA_PREVIEW_STATE_ERROR,
    CAMERA_PREVIEW_STATE_STOPPED
} camera_preview_state_t;

/*
 * Create the LVGL image used by the camera preview.
 * The capture thread is not started until camera_preview_start() is called.
 */
camera_preview_t * camera_preview_create(lv_obj_t * parent,
                                         uint16_t capture_width,
                                         uint16_t capture_height,
                                         uint16_t display_width,
                                         uint16_t display_height,
                                         uint16_t frame_rate);

/* Start libcamera-vid capture in a background thread. */
int camera_preview_start(camera_preview_t * preview);

/*
 * Copy the newest converted frame into the LVGL image buffer.
 * This function must be called from the LVGL thread.
 */
bool camera_preview_refresh(camera_preview_t * preview);

camera_preview_state_t camera_preview_get_state(camera_preview_t * preview);

void camera_preview_get_error(camera_preview_t * preview,
                              char * buffer,
                              size_t buffer_size);

/* Stop capture, release FFmpeg resources and delete the preview image. */
void camera_preview_destroy(camera_preview_t * preview);

#endif
