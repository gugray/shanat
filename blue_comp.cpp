/* 

Single-file shader to draw a (red! lol) triangle on screen

To compile:

g++ blue_comp_simp.cpp -o blue_comp_simp \
    -std=c++11 -I/usr/include/drm \
    -lEGL -lGLESv2 -lgbm -ldrm -lpthread -lm

Then: ./blue_comp

Needs some dependencies installed. Sort of like this, but maybe
not all of them (I experiemented a lot):

sudo apt install -y build-essential git cmake pkg-config \
    libglfw3-dev libgles2-mesa-dev libfreetype6-dev \
    libcurl4-openssl-dev libdrm-dev libgbm-dev \
    libxinerama-dev libxcursor-dev libxi-dev \
    xvfb mesa-utils libdrm-tests libraspberrypi-dev

 */


#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdint>
#include <csignal>
#include <cerrno>
#include <unistd.h>
#include <fcntl.h>
#include <string>

#include <drm/drm.h>
#include <drm/drm_mode.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

#include <gbm.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>

static int drm_fd = -1;
static gbm_device *gbm_dev = nullptr;
static gbm_surface *gbm_surf = nullptr;
static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext egl_ctx = EGL_NO_CONTEXT;
static EGLSurface egl_surf = EGL_NO_SURFACE;
static bool running = true;
static drmModeCrtc *saved_crtc = nullptr;
static uint32_t saved_crtc_id = 0;

static void sighandler(int) { running = false; }

static void die(const char *msg) {
    perror(msg);
    exit(1);
}

static GLuint compile_shader(GLenum type, const char *src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        std::string log(len ? len : 1, '\0');
        glGetShaderInfoLog(s, len, nullptr, &log[0]);
        fprintf(stderr, "Shader compile error: %s\n", log.c_str());
        exit(1);
    }
    return s;
}

static int find_composite_connector(int fd,
                                   drmModeRes *resources,
                                   drmModeConnector **out_conn,
                                   drmModeEncoder **out_enc,
                                   drmModeCrtc **out_crtc) {
    for (int i = 0; i < resources->count_connectors; ++i) {
        uint32_t conn_id = resources->connectors[i];
        drmModeConnector *conn = drmModeGetConnector(fd, conn_id);
        if (!conn) continue;

        if (conn->connector_type == DRM_MODE_CONNECTOR_Composite) {

            drmModeEncoder *enc = nullptr;
            if (conn->encoder_id)
                enc = drmModeGetEncoder(fd, conn->encoder_id);

            drmModeCrtc *crtc = nullptr;
            if (enc && enc->crtc_id)
                crtc = drmModeGetCrtc(fd, enc->crtc_id);

            *out_conn = conn;
            *out_enc  = enc;
            *out_crtc = crtc;
            return 0;
        }

        drmModeFreeConnector(conn);
    }
    return -1;
}

static const char *vert_src =
    "attribute vec2 a_pos;\n"
    "void main() {\n"
    "  gl_Position = vec4(a_pos, 0.0, 1.0);\n"
    "}\n";

static const char *frag_src =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

int main() {
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm_fd < 0) die("open /dev/dri/card0");

    drmModeRes *resources = drmModeGetResources(drm_fd);
    if (!resources) die("drmModeGetResources");

    drmModeConnector *conn = nullptr;
    drmModeEncoder *enc = nullptr;
    drmModeCrtc *crtc = nullptr;

    if (find_composite_connector(drm_fd, resources, &conn, &enc, &crtc) < 0) {
        fprintf(stderr, "No connected composite connector found. Available connectors:\n");
        for (int i = 0; i < resources->count_connectors; ++i) {
            drmModeConnector *c = drmModeGetConnector(drm_fd, resources->connectors[i]);
            if (!c) continue;
            printf("  id %u type %u connection %d modes %d\n",
                   c->connector_id, c->connector_type, c->connection, c->count_modes);
            drmModeFreeConnector(c);
        }
        drmModeFreeResources(resources);
        return 1;
    }

    if (conn->count_modes == 0) {
        fprintf(stderr, "Composite connector has no modes\n");
        drmModeFreeConnector(conn);
        drmModeFreeResources(resources);
        return 1;
    }

    // pick preferred or first mode
    drmModeModeInfo mode = conn->modes[0];
    for (int i = 0; i < conn->count_modes; ++i) {
        if (conn->modes[i].type & DRM_MODE_TYPE_PREFERRED) {
            mode = conn->modes[i];
            break;
        }
    }

    printf("Using connector %u mode '%s' %ux%u\n", conn->connector_id, mode.name, mode.hdisplay, mode.vdisplay);

    // Save current CRTC (if any) so we can restore later
    if (enc && enc->crtc_id) {
        saved_crtc = drmModeGetCrtc(drm_fd, enc->crtc_id);
        if (saved_crtc) saved_crtc_id = saved_crtc->crtc_id;
    }

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
    egl_display = eglGetDisplay((EGLNativeDisplayType)gbm_dev);
    if (egl_display == EGL_NO_DISPLAY) die("eglGetDisplay");
    if (!eglInitialize(egl_display, nullptr, nullptr)) die("eglInitialize");

    EGLint cfg_attribs[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RED_SIZE,     8,
        EGL_GREEN_SIZE,   8,
        EGL_BLUE_SIZE,    8,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
        EGL_NONE
    };
    EGLConfig cfg;
    EGLint num_cfg;
    if (!eglChooseConfig(egl_display, cfg_attribs, &cfg, 1, &num_cfg) || num_cfg < 1) die("eglChooseConfig");

    EGLint ctx_attribs[] = { EGL_CONTEXT_CLIENT_VERSION, 2, EGL_NONE };
    egl_ctx = eglCreateContext(egl_display, cfg, EGL_NO_CONTEXT, ctx_attribs);
    if (egl_ctx == EGL_NO_CONTEXT) die("eglCreateContext");

    egl_surf = eglCreateWindowSurface(egl_display, cfg, (EGLNativeWindowType)gbm_surf, nullptr);
    if (egl_surf == EGL_NO_SURFACE) die("eglCreateWindowSurface");

    if (!eglMakeCurrent(egl_display, egl_surf, egl_surf, egl_ctx)) die("eglMakeCurrent");

    // GL program
    GLuint vs = compile_shader(GL_VERTEX_SHADER, vert_src);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, frag_src);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glBindAttribLocation(prog, 0, "a_pos");
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::string log(len ? len : 1, '\0');
        glGetProgramInfoLog(prog, len, nullptr, &log[0]);
        fprintf(stderr, "Program link error: %s\n", log.c_str());
        exit(1);
    }
    glUseProgram(prog);

    // fullscreen triangle (NDC)
    GLfloat verts[] = {
        -1.0f, -1.0f,
         1.0f, -1.0f,
        -1.0f,  1.0f
    };
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glViewport(0, 0, mode.hdisplay, mode.vdisplay);
    glClearColor(0,0,0,1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFinish();

    if (!eglSwapBuffers(egl_display, egl_surf)) die("eglSwapBuffers");

    // Lock front buffer and create DRM fb with addfb2
    struct gbm_bo *bo = gbm_surface_lock_front_buffer(gbm_surf);
    if (!bo) die("gbm_surface_lock_front_buffer");

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    uint32_t handles[4] = { handle, 0, 0, 0 };
    uint32_t pitches[4] = { stride, 0, 0, 0 };
    uint32_t offsets[4] = { 0, 0, 0, 0 };
    uint32_t fb_id = 0;
    uint32_t crtc_id = 0;

    int ret = drmModeAddFB2(drm_fd, width, height, GBM_FORMAT_XRGB8888,
                            handles, pitches, offsets, &fb_id, 0);
    if (ret) {
        perror("drmModeAddFB2");
        gbm_surface_release_buffer(gbm_surf, bo);
        goto cleanup;
    }

    // choose CRTC: prefer encoder's crtc, else first available
    if (enc && enc->crtc_id) crtc_id = enc->crtc_id;
    else if (resources->count_crtcs > 0) crtc_id = resources->crtcs[0];

    if (!crtc_id) {
        fprintf(stderr, "No available CRTC\n");
        goto cleanup;
    }

    ret = drmModeSetCrtc(drm_fd, crtc_id, fb_id, 0, 0, &conn->connector_id, 1, &mode);
    if (ret) {
        perror("drmModeSetCrtc");
        goto cleanup;
    }

    printf("Blue triangle displayed on composite. Ctrl-C to exit.\n");
    while (running) sleep(1);

cleanup:
    // restore saved CRTC if we saved one
    if (saved_crtc) {
        drmModeSetCrtc(drm_fd, saved_crtc->crtc_id, saved_crtc->buffer_id,
                       saved_crtc->x, saved_crtc->y, &conn->connector_id, 1, &saved_crtc->mode);
        drmModeFreeCrtc(saved_crtc);
    }

    if (bo) {
        // remove fb and release BO
        if (fb_id) drmModeRmFB(drm_fd, fb_id);
        gbm_surface_release_buffer(gbm_surf, bo);
    }

    if (conn) drmModeFreeConnector(conn);
    if (enc) drmModeFreeEncoder(enc);
    drmModeFreeResources(resources);

    if (egl_display != EGL_NO_DISPLAY) {
        eglMakeCurrent(egl_display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (egl_ctx != EGL_NO_CONTEXT) eglDestroyContext(egl_display, egl_ctx);
        if (egl_surf != EGL_NO_SURFACE) eglDestroySurface(egl_display, egl_surf);
        eglTerminate(egl_display);
    }

    if (gbm_surf) gbm_surface_destroy(gbm_surf);
    if (gbm_dev) gbm_device_destroy(gbm_dev);
    if (drm_fd >= 0) close(drm_fd);

    printf("Exiting.\n");
    return 0;
}

