#include "../shanat-shared/fps.h"
#include "../shanat-shared/geo.h"
#include "../shanat-shared/horrors.h"
#include "hot_file.h"

#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <math.h>
#include <memory>
#include <stdint.h>
#include <unistd.h>

static const char *frag_glsl_file = "frag.glsl";
static int64_t frag_glsl_modif;
static std::string frag_glsl_content;

GLuint vs = 0;
GLuint fs = 0;
GLuint prog = 0;

static bool running = true;

static void sighandler(int)
{
    running = false;
}

static void main_inner();
static void report_shader_link_error(GLuint prog);
static GLuint compile_shader(GLenum type, const char *src, bool exit_on_error);
static void update_program();

int main()
{
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    main_inner();

    printf("\nGoodbye!\n");
    return 0;
}

static void main_inner()
{
    // Set up graphics
    init_horrors();

    // FPS control
    FPS fps(25);

    // Fragment shader source
    HotFile hf(frag_glsl_file);
    hf.check_update(frag_glsl_content, frag_glsl_modif);
    printf("Watching shader file: %s\n", frag_glsl_file);
    if (frag_glsl_modif == 0) printf("File appears to be missing\n");

    // Compile shaders, link program
    update_program();

    // OpenGL fidgeting
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Buffer the shader will be outputting to
    GLuint vbo;
    glGenBuffers(1, &vbo);

    // Vertex shader's two fixed triangles
    GLfloat sweep_verts[] = {
        -1, -1,
        1, -1,
        -1, 1,
        -1, 1,
        1, -1,
        1, 1};

    // Get uniform locations
    GLint time_loc = glGetUniformLocation(prog, "time");
    GLint resolution_loc = glGetUniformLocation(prog, "resolution");
    // ===================================================

    while (running)
    {
        if (hf.check_update(frag_glsl_content, frag_glsl_modif))
        {
            printf("File updated                                               \n");
            update_program();
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        float current_time = fps.frame_start();

        glUniform1f(time_loc, current_time);
        glUniform2f(resolution_loc, (float)mode.hdisplay, (float)mode.vdisplay);

        glBufferData(GL_ARRAY_BUFFER, sizeof(sweep_verts), sweep_verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glViewport(0, 0, mode.hdisplay, mode.vdisplay);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glFinish();

        put_on_screen();

        fps.frame_end();
    }

    glDeleteBuffers(1, &vbo);
    cleanup_horrors();
}

static GLuint compile_shader(GLenum type, const char *src, bool exit_on_error)
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
        if (exit_on_error)
            exit_with_cleanup(1);
        // TODO: Report via file
        glDeleteShader(s);
        return 0;
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

static const char *vert_sweep_glsl = R"(
attribute vec2 a_pos;
void main() {
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)";

static void update_program()
{
    // Delete previous
    if (vs != 0) glDeleteShader(vs);
    if (fs != 0) glDeleteShader(fs);
    if (prog == 0) glDeleteProgram(prog);
    prog = vs = fs = 0;

    // Compile shaders; fall back to placeholder fragment shader on error
    vs = compile_shader(GL_VERTEX_SHADER, vert_sweep_glsl, true);
    fs = compile_shader(GL_FRAGMENT_SHADER, frag_glsl_content.c_str(), false);

    prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    const GLint ixPosAttribute = 0;
    glBindAttribLocation(prog, ixPosAttribute, "a_pos");
    glLinkProgram(prog);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) report_shader_link_error(prog);
    glUseProgram(prog);
}
