#include <cerrno>
#include <csignal>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <gbm.h>
#include <xf86drm.h>
#include <xf86drmMode.h>

static int drm_fd = -1;
static drmModeRes *resources = nullptr;
static drmModeConnectorPtr conn = nullptr;
static drmModeEncoderPtr enc = nullptr;
static drmModeCrtc *saved_crtc = nullptr;
static gbm_device *gbm_dev = nullptr;
static gbm_surface *gbm_surf = nullptr;
static EGLDisplay egl_display = EGL_NO_DISPLAY;
static EGLContext egl_ctx = EGL_NO_CONTEXT;
static EGLSurface egl_surf = EGL_NO_SURFACE;
static gbm_bo *bo = nullptr;
static uint32_t fb_id = 0;

static bool running = true;

static void cleanup()
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

static void sighandler(int)
{
    running = false;
}

static void exit_with_cleanup(int status)
{
    cleanup();
    exit(status);
}

static void die(const char *fun)
{
    fprintf(stderr, "Function call failed: %s\n", fun);
    exit_with_cleanup(1);
}

static GLuint compile_shader(GLenum type, const char *src)
{
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok)
    {
        GLint len = 0;
        glGetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
        char *log = new char[len ? len : 1];
        log[0] = '\0';
        glGetShaderInfoLog(s, len, nullptr, log);
        fprintf(stderr, "Shader compile error: %s\n", log);
        delete[] log;
        exit_with_cleanup(1);
    }
    return s;
}

static void list_composite_connectors()
{
    fprintf(stderr, "No connected composite connector found. Available connectors:\n");
    for (int i = 0; i < resources->count_connectors; ++i)
    {
        drmModeConnectorPtr c = drmModeGetConnector(drm_fd, resources->connectors[i]);
        if (!c) continue;
        printf("  id %u type %u connection %d modes %d\n",
               c->connector_id, c->connector_type, c->connection, c->count_modes);
        drmModeFreeConnector(c);
    }
}

static drmModeConnectorPtr get_composite_connector()
{
    for (int i = 0; i < resources->count_connectors; ++i)
    {
        uint32_t conn_id = resources->connectors[i];
        drmModeConnectorPtr c = drmModeGetConnector(drm_fd, conn_id);
        if (!c) continue;

        if (c->connector_type != DRM_MODE_CONNECTOR_Composite)
        {
            drmModeFreeConnector(c);
            continue;
        }
        if (c->count_modes == 0)
        {
            fprintf(stderr, "Composite connector has no modes\n");
            drmModeFreeConnector(c);
            exit_with_cleanup(1);
        }

        return c;
    }
    list_composite_connectors();
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

int main()
{
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    drm_fd = open("/dev/dri/card0", O_RDWR | O_CLOEXEC);
    if (drm_fd < 0) die("open /dev/dri/card0");

    resources = drmModeGetResources(drm_fd);
    if (!resources) die("drmModeGetResources");

    conn = get_composite_connector();
    if (conn->encoder_id) enc = drmModeGetEncoder(drm_fd, conn->encoder_id);

    // Save current CRTC (if any) so we can restore later
    if (enc && enc->crtc_id) saved_crtc = drmModeGetCrtc(drm_fd, enc->crtc_id);

    // Pick preferred or first mode
    drmModeModeInfo mode = get_first_or_preferred_mode();

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
    if (!ok)
    {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        char *log = new char[len ? len : 1];
        log[0] = '\0';
        glGetProgramInfoLog(prog, len, nullptr, log);
        fprintf(stderr, "Program link error: %s\n", log);
        delete[] log;
        exit_with_cleanup(1);
    }
    glUseProgram(prog);

    // Fullscreen triangle (NDC)
    GLfloat verts[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f};
    GLuint vbo;
    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

    glViewport(0, 0, mode.hdisplay, mode.vdisplay);
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glFinish();

    if (!eglSwapBuffers(egl_display, egl_surf)) die("eglSwapBuffers");

    // Lock front buffer and create DRM fb with addfb2
    bo = gbm_surface_lock_front_buffer(gbm_surf);
    if (!bo) die("gbm_surface_lock_front_buffer");

    uint32_t width = gbm_bo_get_width(bo);
    uint32_t height = gbm_bo_get_height(bo);
    uint32_t stride = gbm_bo_get_stride(bo);
    uint32_t handle = gbm_bo_get_handle(bo).u32;

    uint32_t handles[4] = {handle, 0, 0, 0};
    uint32_t pitches[4] = {stride, 0, 0, 0};
    uint32_t offsets[4] = {0, 0, 0, 0};

    if (drmModeAddFB2(drm_fd, width, height, GBM_FORMAT_XRGB8888,
                      handles, pitches, offsets, &fb_id, 0))
    {
        die("drmModeAddFB2");
    }

    set_crtc(mode, fb_id);

    printf("Output rendered on composite. Ctrl-C to exit.\n");

    while (running)
    {
        sleep(1);
    }

    printf("Goodbye!\n");
    cleanup();
    return 0;
}
