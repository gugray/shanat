#include "horrors.h"

#include <unistd.h>

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>

int drm_fd = -1;
drmModeRes *resources = nullptr;
drmModeConnectorPtr conn = nullptr;
drmModeEncoderPtr enc = nullptr;
drmModeCrtc *saved_crtc = nullptr;
drmModeModeInfo mode;
gbm_device *gbm_dev = nullptr;
gbm_surface *gbm_surf = nullptr;
EGLDisplay egl_display = EGL_NO_DISPLAY;
EGLContext egl_ctx = EGL_NO_CONTEXT;
EGLSurface egl_surf = EGL_NO_SURFACE;
gbm_bo *bo = nullptr;
uint32_t fb_id = 0;
uint32_t prev_fb_id = 0;

void exit_with_cleanup(int status)
{
    cleanup_horrors();
    exit(status);
}

void die(const char *fun)
{
    fprintf(stderr, "Function call failed: %s\n", fun);
    exit_with_cleanup(1);
}

void cleanup_horrors()
{
    if (bo)
    {
        // Remove fb and release BO
        if (fb_id) drmModeRmFB(drm_fd, fb_id);
        gbm_surface_release_buffer(gbm_surf, bo);
    }

    // Uninit EGL
    if (egl_display != EGL_NO_DISPLAY)
    {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_ctx != EGL_NO_CONTEXT) eglDestroyContext(egl_display, egl_ctx);
        if (egl_surf != EGL_NO_SURFACE) eglDestroySurface(egl_display, egl_surf);
        eglTerminate(egl_display);
    }

    if (gbm_surf) gbm_surface_destroy(gbm_surf);
    if (gbm_dev) gbm_device_destroy(gbm_dev);

    // Restore saved CRTC if we saved one
    if (saved_crtc)
    {
        drmModeSetCrtc(drm_fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                       saved_crtc->x, saved_crtc->y, &conn->connector_id, 1, &saved_crtc->mode);
        drmModeFreeCrtc(saved_crtc);
    }

    if (enc) drmModeFreeEncoder(enc);
    if (conn) drmModeFreeConnector(conn);
    if (resources != nullptr) drmModeFreeResources(resources);
    if (drm_fd >= 0) close(drm_fd);
}

static void list_connectors()
{
    fprintf(stderr, "Available connectors:\n");
    for (int i = 0; i < resources->count_connectors; ++i)
    {
        drmModeConnectorPtr c = drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (!c) continue;
        printf("ID: %u type: %u connection: %d modes: %d\n",
               c->connector_id, c->connector_type, c->connection, c->count_modes);
        drmModeFreeConnector(c);
    }
}

static drmModeConnectorPtr get_preferred_connector()
{
    list_connectors();

    // Prefer composite connector, but will use first OK one otherwise
    drmModeConnectorPtr first_connector = nullptr;
    drmModeConnectorPtr composite_connector = nullptr;

    for (int i = 0; i < resources->count_connectors; ++i)
    {
        uint32_t conn_id = resources->connectors[i];
        drmModeConnectorPtr c = drmModeGetConnector(drm_fd, conn_id);
        if (!c) continue;
        if (c->count_modes == 0)
        {
            drmModeFreeConnector(c);
            continue;
        }
        if (composite_connector == nullptr && c->connector_type == DRM_MODE_CONNECTOR_Composite)
            composite_connector = c;
        else if (first_connector == nullptr)
            first_connector = c;
        else drmModeFreeConnector(c);
    }
    if (composite_connector != nullptr)
    {
        if (first_connector != nullptr) drmModeFreeConnector(first_connector);
        return composite_connector;
    }
    if (first_connector != nullptr)
    {
        if (composite_connector != nullptr) drmModeFreeConnector(composite_connector);
        return first_connector;
    }

    fprintf(stderr, "No connector found\n");
    exit_with_cleanup(1);
    return nullptr;
}

static drmModeModeInfo get_first_or_preferred_mode()
{
    drmModeModeInfo mode = conn->modes[0];
    for (int i = 0; i < conn->count_modes; ++i)
    {
        if (conn->modes[i].type & DRM_MODE_TYPE_PREFERRED)
        {
            mode = conn->modes[i];
            break;
        }
    }
    printf("Using connector %u mode '%s' %ux%u\n", conn->connector_id, mode.name, mode.hdisplay, mode.vdisplay);
    return mode;
}

static void init_egl()
{
    egl_display = eglGetDisplay((EGLNativeDisplayType)gbm_dev);
    if (egl_display == EGL_NO_DISPLAY) die("eglGetDisplay");
    if (!eglInitialize(egl_display, nullptr, nullptr)) die("eglInitialize");

    EGLint cfg_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE};
    EGLConfig cfg;
    EGLint num_cfg;
    if (!eglChooseConfig(egl_display, cfg_attribs, &cfg, 1, &num_cfg) || num_cfg < 1) die("eglChooseConfig");

    EGLint ctx_attribs[] = {EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE};
    egl_ctx = eglCreateContext(egl_display, cfg, EGL_NO_CONTEXT, ctx_attribs);
    if (egl_ctx == EGL_NO_CONTEXT) die("eglCreateContext");

    egl_surf = eglCreateWindowSurface(egl_display, cfg, (EGLNativeWindowType)gbm_surf, nullptr);
    if (egl_surf == EGL_NO_SURFACE) die("eglCreateWindowSurface");

    if (!eglMakeCurrent(egl_display, egl_surf, egl_surf, egl_ctx)) die("eglMakeCurrent");
}

static void set_crtc(drmModeModeInfo mode, uint32_t fb_id)
{
    uint32_t crtc_id = 0;
    // Prefer encoder's crtc, else first available
    if (enc && enc->crtc_id) crtc_id = enc->crtc_id;
    else if (resources->count_crtcs > 0) crtc_id = resources->crtcs[0];

    if (!crtc_id)
    {
        fprintf(stderr, "No available CRTC\n");
        exit_with_cleanup(1);
    }

    int ret = drmModeSetCrtc(drm_fd, crtc_id, fb_id, 0, 0, &conn->connector_id, 1, &mode);
    if (ret) die("drmModeSetCrtc");
}

void init_horrors(const char *devicePath)
{
    printf("Device: %s\n", devicePath);
    drm_fd = open(devicePath, O_RDWR | O_CLOEXEC);
    if (drm_fd < 0)
    {
        fprintf(stderr, "Failed to open device '%s'\n", devicePath);
        exit_with_cleanup(1);
    }

    resources = drmModeGetResources(drm_fd);
    if (!resources) die("drmModeGetResources");

    conn = get_preferred_connector();
    if (conn->encoder_id) enc = drmModeGetEncoder(drm_fd, conn->encoder_id);

    // Save current CRTC (if any) so we can restore later
    if (enc && enc->crtc_id) saved_crtc = drmModeGetCrtc(drm_fd, enc->crtc_id);

    // Pick preferred or first mode
    mode = get_first_or_preferred_mode();

    // GBM device and surface
    gbm_dev = gbm_create_device(drm_fd);
    if (!gbm_dev) die("gbm_create_device");

    gbm_surf = gbm_surface_create(gbm_dev,
                                  mode.hdisplay,
                                  mode.vdisplay,
                                  GBM_FORMAT_XRGB8888,
                                  GBM_BO_USE_SCANOUT | GBM_BO_USE_RENDERING);
    if (!gbm_surf) die("gbm_surface_create");

    // EGL init
    init_egl();
}

void put_on_screen()
{
    // Swap EGL buffers
    if (!eglSwapBuffers(egl_display, egl_surf)) die("eglSwapBuffers");

    // Get new buffer
    gbm_bo *new_bo = gbm_surface_lock_front_buffer(gbm_surf);
    if (!new_bo) die("gbm_surface_lock_front_buffer");

    uint32_t width = gbm_bo_get_width(new_bo);
    uint32_t height = gbm_bo_get_height(new_bo);
    uint32_t stride = gbm_bo_get_stride(new_bo);
    uint32_t handle = gbm_bo_get_handle(new_bo).u32;

    uint32_t handles[4] = {handle, 0, 0, 0};
    uint32_t pitches[4] = {stride, 0, 0, 0};
    uint32_t offsets[4] = {0, 0, 0, 0};

    uint32_t new_fb_id = 0;
    if (drmModeAddFB2(drm_fd, width, height, GBM_FORMAT_XRGB8888,
                      handles, pitches, offsets, &new_fb_id, 0))
    {
        die("drmModeAddFB2");
    }

    // Set new framebuffer before releasing old buffer
    set_crtc(mode, new_fb_id);

    // Now safe to release old buffer and remove old FB
    if (bo) gbm_surface_release_buffer(gbm_surf, bo);
    if (prev_fb_id) drmModeRmFB(drm_fd, prev_fb_id);

    // Update for next iteration
    bo = new_bo;
    prev_fb_id = fb_id;
    fb_id = new_fb_id;
}
