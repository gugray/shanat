#include <csignal>
#include <cstdio>

#include "fps.h"
#include "horrors.h"

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

static const char *vert_src = R"(
attribute vec2 a_pos;
void main() {
    gl_Position = vec4(a_pos, 0.0, 1.0);
}
)";

static const char *frag_src =
    "precision mediump float;\n"
    "void main() {\n"
    "  gl_FragColor = vec4(1.0, 0.0, 0.0, 1.0);\n"
    "}\n";

int main()
{
    signal(SIGINT, sighandler);
    signal(SIGTERM, sighandler);

    init_horrors();

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

    GLuint vbo;
    glGenBuffers(1, &vbo);

    GLfloat xtop = -1.0f;

    FPS fps(25);

    while (running)
    {
        fps.frame_start();

        GLfloat verts[] = {
            -1.0f, -1.0f,
            1.0f, -1.0f,
            -1.0f, 1.0f};

        verts[4] = xtop;
        xtop += 0.01;
        if (xtop > 1.0f) xtop = -1.0f;

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

        glViewport(0, 0, mode.hdisplay, mode.vdisplay);
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glFinish();

        put_on_screen();

        fps.frame_end();
    }

    glDeleteBuffers(1, &vbo);

    printf("\nGoodbye!\n");
    cleanup_horrors();
    return 0;
}
