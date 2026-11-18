#include "camera_preview.h"

#include <errno.h>
#include <poll.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>

#if LV_COLOR_DEPTH != 16
#error "camera_preview currently requires LV_COLOR_DEPTH == 16"
#endif

#define CAMERA_ERROR_TEXT_MAX 160
#define CAMERA_READ_POLL_MS   100

struct camera_preview {
    lv_obj_t * image;
    lv_image_dsc_t image_dsc;

    uint8_t * display_buffer;
    uint8_t * latest_buffer;
    size_t rgb_buffer_size;
    uint16_t capture_width;
    uint16_t capture_height;
    uint16_t display_width;
    uint16_t display_height;
    uint16_t frame_rate;

    pthread_t capture_thread;
    pthread_mutex_t lock;
    atomic_bool running;
    bool thread_started;
    pid_t camera_pid;
    int camera_fd;

    uint64_t latest_sequence;
    uint64_t displayed_sequence;
    camera_preview_state_t state;
    char error_message[CAMERA_ERROR_TEXT_MAX];
};

static void * camera_capture_thread(void * arg);

static void camera_preview_set_error(camera_preview_t * preview,
                                     const char * message)
{
    pthread_mutex_lock(&preview->lock);
    snprintf(preview->error_message,
             sizeof(preview->error_message),
             "%s",
             message);
    preview->state = CAMERA_PREVIEW_STATE_ERROR;
    pthread_mutex_unlock(&preview->lock);
}

static void camera_preview_set_errno(camera_preview_t * preview,
                                     const char * operation)
{
    char message[CAMERA_ERROR_TEXT_MAX];

    snprintf(message,
             sizeof(message),
             "%s: %s",
             operation,
             strerror(errno));
    camera_preview_set_error(preview, message);
}

static int camera_read_frame(camera_preview_t * preview,
                             int fd,
                             uint8_t * buffer,
                             size_t buffer_size)
{
    size_t offset = 0;

    while(offset < buffer_size && atomic_load(&preview->running)) {
        struct pollfd poll_fd = {
            .fd = fd,
            .events = POLLIN
        };
        int poll_ret = poll(&poll_fd, 1, CAMERA_READ_POLL_MS);

        if(poll_ret < 0) {
            if(errno == EINTR) {
                continue;
            }
            return -1;
        }
        if(poll_ret == 0) {
            continue;
        }
        if(poll_fd.revents & POLLIN) {
            ssize_t read_size = read(fd,
                                     buffer + offset,
                                     buffer_size - offset);
            if(read_size > 0) {
                offset += (size_t)read_size;
                continue;
            }
            if(read_size < 0 && errno == EINTR) {
                continue;
            }
            return -1;
        }
        if(poll_fd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
            return -1;
        }
    }

    return offset == buffer_size ? 0 : -1;
}

static pid_t camera_process_start(camera_preview_t * preview,
                                  int * read_fd)
{
    int pipe_fd[2];
    pid_t pid;
    char width_text[16];
    char height_text[16];
    char frame_rate_text[16];

    if(pipe(pipe_fd) != 0) {
        camera_preview_set_errno(preview, "pipe");
        return -1;
    }

    snprintf(width_text,
             sizeof(width_text),
             "%u",
             preview->capture_width);
    snprintf(height_text,
             sizeof(height_text),
             "%u",
             preview->capture_height);
    snprintf(frame_rate_text,
             sizeof(frame_rate_text),
             "%u",
             preview->frame_rate);

    pid = fork();
    if(pid < 0) {
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        camera_preview_set_errno(preview, "fork");
        return -1;
    }

    if(pid == 0) {
        close(pipe_fd[0]);
        if(dup2(pipe_fd[1], STDOUT_FILENO) < 0) {
            _exit(127);
        }
        close(pipe_fd[1]);

        /* Camera Module 3 is mounted upside down; correct it at capture time. */
        execlp("libcamera-vid",
               "libcamera-vid",
               "--timeout", "0",
               "--nopreview",
               "--codec", "yuv420",
               "--width", width_text,
               "--height", height_text,
               "--framerate", frame_rate_text,
               "--rotation", "180",
               "--autofocus-mode", "continuous",
               "--verbose", "0",
               "--output", "-",
               (char *)NULL);

        _exit(127);
    }

    close(pipe_fd[1]);
    *read_fd = pipe_fd[0];
    return pid;
}

camera_preview_t * camera_preview_create(lv_obj_t * parent,
                                         uint16_t capture_width,
                                         uint16_t capture_height,
                                         uint16_t display_width,
                                         uint16_t display_height,
                                         uint16_t frame_rate)
{
    camera_preview_t * preview;

    if(parent == NULL || capture_width == 0 || capture_height == 0 ||
       display_width == 0 || display_height == 0 || frame_rate == 0 ||
       (capture_width & 1U) != 0 || (capture_height & 1U) != 0) {
        return NULL;
    }

    preview = calloc(1, sizeof(*preview));
    if(preview == NULL) {
        return NULL;
    }

    preview->capture_width = capture_width;
    preview->capture_height = capture_height;
    preview->display_width = display_width;
    preview->display_height = display_height;
    preview->frame_rate = frame_rate;
    preview->camera_pid = -1;
    preview->camera_fd = -1;
    preview->rgb_buffer_size =
        (size_t)display_width * display_height * sizeof(uint16_t);
    preview->display_buffer = calloc(1, preview->rgb_buffer_size);
    preview->latest_buffer = malloc(preview->rgb_buffer_size);

    if(preview->display_buffer == NULL || preview->latest_buffer == NULL) {
        free(preview->latest_buffer);
        free(preview->display_buffer);
        free(preview);
        return NULL;
    }

    if(pthread_mutex_init(&preview->lock, NULL) != 0) {
        free(preview->latest_buffer);
        free(preview->display_buffer);
        free(preview);
        return NULL;
    }

    atomic_init(&preview->running, false);
    preview->state = CAMERA_PREVIEW_STATE_IDLE;

    preview->image_dsc.header.magic = LV_IMAGE_HEADER_MAGIC;
    preview->image_dsc.header.cf = LV_COLOR_FORMAT_NATIVE;
    preview->image_dsc.header.w = display_width;
    preview->image_dsc.header.h = display_height;
    preview->image_dsc.header.stride = display_width * sizeof(uint16_t);
    preview->image_dsc.data_size = (uint32_t)preview->rgb_buffer_size;
    preview->image_dsc.data = preview->display_buffer;

    preview->image = lv_image_create(parent);
    if(preview->image == NULL) {
        pthread_mutex_destroy(&preview->lock);
        free(preview->latest_buffer);
        free(preview->display_buffer);
        free(preview);
        return NULL;
    }

    lv_image_set_src(preview->image, &preview->image_dsc);
    lv_obj_set_size(preview->image, display_width, display_height);
    lv_obj_center(preview->image);

    return preview;
}

int camera_preview_start(camera_preview_t * preview)
{
    int ret;

    if(preview == NULL || preview->thread_started) {
        return -1;
    }

    pthread_mutex_lock(&preview->lock);
    preview->error_message[0] = '\0';
    preview->state = CAMERA_PREVIEW_STATE_CONNECTING;
    pthread_mutex_unlock(&preview->lock);

    atomic_store(&preview->running, true);
    ret = pthread_create(&preview->capture_thread,
                         NULL,
                         camera_capture_thread,
                         preview);
    if(ret != 0) {
        atomic_store(&preview->running, false);
        pthread_mutex_lock(&preview->lock);
        snprintf(preview->error_message,
                 sizeof(preview->error_message),
                 "pthread_create: %s",
                 strerror(ret));
        preview->state = CAMERA_PREVIEW_STATE_ERROR;
        pthread_mutex_unlock(&preview->lock);
        return -1;
    }

    preview->thread_started = true;
    return 0;
}

bool camera_preview_refresh(camera_preview_t * preview)
{
    bool refreshed = false;

    if(preview == NULL || preview->image == NULL) {
        return false;
    }

    pthread_mutex_lock(&preview->lock);
    if(preview->latest_sequence != preview->displayed_sequence) {
        memcpy(preview->display_buffer,
               preview->latest_buffer,
               preview->rgb_buffer_size);
        preview->displayed_sequence = preview->latest_sequence;
        refreshed = true;
    }
    pthread_mutex_unlock(&preview->lock);

    if(refreshed) {
        lv_image_cache_drop(lv_image_get_src(preview->image));
        lv_obj_invalidate(preview->image);
    }

    return refreshed;
}

camera_preview_state_t camera_preview_get_state(camera_preview_t * preview)
{
    camera_preview_state_t state;

    if(preview == NULL) {
        return CAMERA_PREVIEW_STATE_ERROR;
    }

    pthread_mutex_lock(&preview->lock);
    state = preview->state;
    pthread_mutex_unlock(&preview->lock);

    return state;
}

void camera_preview_get_error(camera_preview_t * preview,
                              char * buffer,
                              size_t buffer_size)
{
    if(buffer == NULL || buffer_size == 0) {
        return;
    }

    buffer[0] = '\0';
    if(preview == NULL) {
        return;
    }

    pthread_mutex_lock(&preview->lock);
    snprintf(buffer, buffer_size, "%s", preview->error_message);
    pthread_mutex_unlock(&preview->lock);
}

void camera_preview_destroy(camera_preview_t * preview)
{
    pid_t camera_pid;

    if(preview == NULL) {
        return;
    }

    atomic_store(&preview->running, false);

    pthread_mutex_lock(&preview->lock);
    camera_pid = preview->camera_pid;
    pthread_mutex_unlock(&preview->lock);
    if(camera_pid > 0) {
        kill(camera_pid, SIGTERM);
    }

    if(preview->thread_started) {
        pthread_join(preview->capture_thread, NULL);
        preview->thread_started = false;
    }

    if(preview->image != NULL) {
        lv_image_cache_drop(lv_image_get_src(preview->image));
        lv_obj_delete(preview->image);
        preview->image = NULL;
    }

    pthread_mutex_destroy(&preview->lock);
    free(preview->latest_buffer);
    free(preview->display_buffer);
    free(preview);
}

static void * camera_capture_thread(void * arg)
{
    camera_preview_t * preview = arg;
    struct SwsContext * sws_ctx = NULL;
    uint8_t * yuv_frame = NULL;
    uint8_t * converted_frame = NULL;
    uint8_t * source_data[4] = {0};
    uint8_t * destination_data[4] = {0};
    int source_linesize[4] = {0};
    int destination_linesize[4] = {0};
    size_t luma_size =
        (size_t)preview->capture_width * preview->capture_height;
    size_t yuv_buffer_size = luma_size + luma_size / 2;
    pid_t camera_pid = -1;
    int camera_fd = -1;
    int ret;

    yuv_frame = malloc(yuv_buffer_size);
    converted_frame = malloc(preview->rgb_buffer_size);
    if(yuv_frame == NULL || converted_frame == NULL) {
        camera_preview_set_error(preview, "camera frame allocation failed");
        goto cleanup;
    }

    sws_ctx = sws_getContext(preview->capture_width,
                             preview->capture_height,
                             AV_PIX_FMT_YUV420P,
                             preview->display_width,
                             preview->display_height,
                             AV_PIX_FMT_RGB565LE,
                             SWS_FAST_BILINEAR,
                             NULL,
                             NULL,
                             NULL);
    if(sws_ctx == NULL) {
        camera_preview_set_error(preview, "sws_getContext failed");
        goto cleanup;
    }

    /* libcamera-vid YUV420 output uses the full-range JPEG/SYCC colour space. */
    if(sws_setColorspaceDetails(sws_ctx,
                                sws_getCoefficients(SWS_CS_ITU601),
                                1,
                                sws_getCoefficients(SWS_CS_ITU601),
                                1,
                                0,
                                1 << 16,
                                1 << 16) < 0) {
        camera_preview_set_error(preview,
                                 "sws_setColorspaceDetails failed");
        goto cleanup;
    }

    camera_pid = camera_process_start(preview, &camera_fd);
    if(camera_pid < 0) {
        goto cleanup;
    }

    pthread_mutex_lock(&preview->lock);
    preview->camera_pid = camera_pid;
    preview->camera_fd = camera_fd;
    pthread_mutex_unlock(&preview->lock);

    if(!atomic_load(&preview->running)) {
        kill(camera_pid, SIGTERM);
        goto cleanup;
    }

    source_data[0] = yuv_frame;
    source_data[1] = yuv_frame + luma_size;
    source_data[2] = yuv_frame + luma_size + luma_size / 4;
    source_linesize[0] = preview->capture_width;
    source_linesize[1] = preview->capture_width / 2;
    source_linesize[2] = preview->capture_width / 2;

    destination_data[0] = converted_frame;
    destination_linesize[0] =
        preview->display_width * sizeof(uint16_t);

    while(atomic_load(&preview->running)) {
        if(camera_read_frame(preview,
                             camera_fd,
                             yuv_frame,
                             yuv_buffer_size) != 0) {
            if(atomic_load(&preview->running)) {
                camera_preview_set_error(
                    preview,
                    "libcamera-vid stopped; check Camera Module 3 and command availability");
            }
            break;
        }

        ret = sws_scale(sws_ctx,
                        (const uint8_t * const *)source_data,
                        source_linesize,
                        0,
                        preview->capture_height,
                        destination_data,
                        destination_linesize);
        if(ret <= 0) {
            camera_preview_set_error(preview, "sws_scale failed");
            break;
        }

        pthread_mutex_lock(&preview->lock);
        memcpy(preview->latest_buffer,
               converted_frame,
               preview->rgb_buffer_size);
        preview->latest_sequence++;
        preview->state = CAMERA_PREVIEW_STATE_STREAMING;
        pthread_mutex_unlock(&preview->lock);
    }

cleanup:
    atomic_store(&preview->running, false);

    if(camera_pid > 0) {
        kill(camera_pid, SIGTERM);
    }
    if(camera_fd >= 0) {
        close(camera_fd);
    }
    if(camera_pid > 0) {
        while(waitpid(camera_pid, NULL, 0) < 0 && errno == EINTR) {
        }
    }

    pthread_mutex_lock(&preview->lock);
    preview->camera_pid = -1;
    preview->camera_fd = -1;
    if(preview->state != CAMERA_PREVIEW_STATE_ERROR) {
        preview->state = CAMERA_PREVIEW_STATE_STOPPED;
    }
    pthread_mutex_unlock(&preview->lock);

    sws_freeContext(sws_ctx);
    free(converted_frame);
    free(yuv_frame);

    return NULL;
}
