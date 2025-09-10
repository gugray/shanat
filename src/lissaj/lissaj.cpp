#include "lissaj.h"

#include <math.h>

float LissajModel::pts[nAllPts * 4] = {0};

void LissajModel::updatePoints(float start)
{
    for (int i = 0; i < nAllPts; ++i)
    {
        const float t = 2 * M_PI * (start + (float)i / (float)nAllPts);
        pts[i * 4] = sin(3 * t);
        pts[i * 4 + 1] = sin(2 * t);
        pts[i * 4 + 2] = sin(5 * t);
        pts[i * 4 + 3] = (float)i / (float)nAllPts;
    }
}

Vec3 LissajModel::colors[nColors];
int LissajModel::clrIxFrom = 0;
int LissajModel::clrIxTo = 1;
float LissajModel::clrInter = 0;

void LissajModel::initColors()
{
    colors[0].vals[0] = 0.4;
    colors[0].vals[1] = 0.612;
    colors[0].vals[2] = 0.98;

    colors[1].vals[0] = 0.31;
    colors[1].vals[1] = 0.09;
    colors[1].vals[2] = 0.529;

    colors[2].vals[0] = 0.984;
    colors[2].vals[1] = 0.475;
    colors[2].vals[2] = 0.235;

    clrIxFrom = 0;
    clrIxTo = 1;
    clrInter = 0;
}

void LissajModel::getColor(float &r, float &g, float &b)
{
    const Vec3 from = colors[clrIxFrom];
    const Vec3 to = colors[clrIxTo];

    // Quadratic ease in-out
    float x = clrInter;
    x = x < 0.5 ? 2 * x * x : 1 - pow(-2 * x + 2, 2) / 2;

    r = from.vals[0] + (to.vals[0] - from.vals[0]) * x;
    g = from.vals[1] + (to.vals[1] - from.vals[1]) * x;
    b = from.vals[2] + (to.vals[2] - from.vals[2]) * x;
}

void LissajModel::updateColor()
{
    clrInter += 0.01;
    if (clrInter > 1)
    {
        clrInter = 0;
        clrIxFrom = clrIxTo;
        clrIxTo = (clrIxTo + 1) % nColors;
    }
}
