#ifndef LISSAJ_MODEL
#define LISSAJ_MODEL

struct LissajModel
{
    static const int nAllPts = 128;
    static float pts[nAllPts * 4];
    static void updatePoints(float start);
};


#endif
