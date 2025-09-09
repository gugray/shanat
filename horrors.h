#ifndef HORRORS_H
#define HORRORS_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

extern int drm_fd;
extern drmModeRes *resources;
extern drmModeConnectorPtr conn;
extern drmModeEncoderPtr enc;
extern drmModeCrtc *saved_crtc;
extern drmModeModeInfo mode;
extern gbm_device *gbm_dev;
extern gbm_surface *gbm_surf;
extern EGLDisplay egl_display;
extern EGLContext egl_ctx;
extern EGLSurface egl_surf;
extern gbm_bo *bo;
extern uint32_t fb_id, prev_fb_id;

void exit_with_cleanup(int status);
void die(const char *fun);
void init_horrors();
void put_on_screen();
void cleanup_horrors();

#endif
