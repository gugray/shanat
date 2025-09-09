#include "lissaj_model.h"

#include <math.h>

float LissajModel::pts[nAllPts * 4] = {0};

void LissajModel::updatePoints(float start)
{
    for (int i = 0; i < nAllPts; ++i)
    {
        const float t = 2 * M_PI * (start + i / nAllPts);
        pts[i*4] = sin(3 * t);
        pts[i*4+1] = sin(2 * t);
        pts[i*4+2] = sin(5 * t);
        pts[i*4+3] = (float)i / (float)nAllPts;
    }
}
