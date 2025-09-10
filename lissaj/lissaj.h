#ifndef LISSAJ_H
#define LISSAJ_H

#include "shaders.h"

struct LissajModel
{
    static const int nAllPts = 128;
    static float pts[nAllPts * 4];
    static void updatePoints(float start);
};

#endif
