#ifndef LISSAJ_H
#define LISSAJ_H

#include "../../shanat-shared/geo.h"
#include "shaders.h"

struct LissajModel
{
    static const int nAllPts = 128;
    static float pts[nAllPts * 4];
    static void updatePoints(float start);

    static const int nColors = 3;
    static Vec3 colors[nColors];
    static int clrIxFrom, clrIxTo;
    static float clrInter;
    static void initColors();
    static void getColor(float &r, float &g, float &b);
    static void updateColor();
};

#endif
