#ifndef LISSAJ_H
#define LISSAJ_H

extern const char *lissaj_vert;
extern const char *lissaj_frag;

struct LissajModel
{
    static const int nAllPts = 128;
    static float pts[nAllPts * 4];
    static void updatePoints(float start);
};

#endif
