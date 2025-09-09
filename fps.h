#ifndef FPS_H
#define FPS_H

#include <csignal>

class FPS
{
  private:
    const int targetFPS;
    const long cycle_usec;
    const int bufSize;
    long *elapsedMicros;
    int ix;
    timeval ts_start;

  private:
    long get_avg_elapsed();

  public:
    FPS(int targetFPS);
    void frame_start();
    void frame_end();
};

#endif
