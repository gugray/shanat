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

static bool running = true;

static void sighandler(int)
{
    running = false;
}

static void main_inner();

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
    HotFile hf(frag_glsl_file);
    hf.check_update(frag_glsl_content, frag_glsl_modif);

    while (running)
    {
        if (hf.check_update(frag_glsl_content, frag_glsl_modif))
            printf("File updated\n");
        usleep(100000);
    }
}
