#ifndef FPS_H
#define FPS_H

#include <csignal>
#include <sys/time.h>

class FPS
{
  private:
    const int target_fps;
    const long cycle_usec;
    const int buf_size;
    long *elapsec_usec;
    int ix;
    timeval ts_init;
    timeval ts_start;

  private:
    long get_avg_elapsed();

  public:
    FPS(int target_fps);
    float frame_start();
    void frame_end();
};

#endif
