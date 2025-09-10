#include "fps.h"
#include "geo.h"
#include "horrors.h"

#include "lissaj/lissaj.h"

#include <csignal>
#include <cstdio>
#include <math.h>

static bool running = true;

static void sighandler(int)
{
    running = false;
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

static void report_shader_link_error(GLuint prog)
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

int main()
{
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    init_horrors();
    FPS fps(25);

    // Compile shaders, link program
    GLuint vs = compile_shader(GL_VERTEX_SHADER, lissaj_vert);
    GLuint fs = compile_shader(GL_FRAGMENT_SHADER, lissaj_frag);
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    const GLint ixPosAttribute = 0;
    glBindAttribLocation(prog, ixPosAttribute, "a_pos");
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) report_shader_link_error(prog);
    glUseProgram(prog);

    // OpenGL fidgeting
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Buffer the shader will be outputting to
    GLuint vbo;
    glGenBuffers(1, &vbo);

    // Non-boilerplate
    // ===================================================

    // Initialize model
    Mat4 proj, view;
    perspective(60, (float)mode.hdisplay / (float)mode.vdisplay, 0.1, 100, proj);
    const float camDist = 3;
    Vec3 camPosition, target, up;
    target.set(0, 0, 0);
    up.set(0, 1, 0);

    // Get uniform locations
    GLint time_loc = glGetUniformLocation(prog, "time");
    GLint resolution_loc = glGetUniformLocation(prog, "resolution");
    GLint clr_loc = glGetUniformLocation(prog, "clr");
    GLint view_loc = glGetUniformLocation(prog, "view");
    GLint proj_loc = glGetUniformLocation(prog, "proj");
    // ===================================================

    while (running)
    {
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        float current_time = fps.frame_start();

        // Non-boilerplate
        // ===================================================
        LissajModel::updatePoints(current_time * 0.5);
        const float camAngle = current_time * 1.0;
        camPosition.set(
            camDist * sin(camAngle),
            2,
            camDist * cos(camAngle));
        lookAt(camPosition, target, up, view);

        // Set uniforms
        glUniform1f(time_loc, current_time);
        glUniform2f(resolution_loc, (float)mode.hdisplay, (float)mode.vdisplay);
        glUniform3f(clr_loc, 0.984, 0.475, 0.235);
        glUniformMatrix4fv(view_loc, 1, GL_FALSE, view.vals);
        glUniformMatrix4fv(proj_loc, 1, GL_FALSE, proj.vals);

        glBufferData(GL_ARRAY_BUFFER, sizeof(LissajModel::pts), LissajModel::pts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(ixPosAttribute);
        glVertexAttribPointer(ixPosAttribute, 4, GL_FLOAT, GL_FALSE, 0, 0);
        // ===================================================

        glViewport(0, 0, mode.hdisplay, mode.vdisplay);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_POINTS, 0, LissajModel::nAllPts);
        glFinish();

        put_on_screen();

        fps.frame_end();
    }

    glDeleteBuffers(1, &vbo);

    printf("\nGoodbye!\n");
    cleanup_horrors();
    return 0;
}
