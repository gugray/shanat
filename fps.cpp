#include "fps.h"

#include <cstdio>
#include <math.h>

FPS::FPS(int targetFPS)
    : targetFPS(targetFPS)
    , cycle_usec(1000000 / targetFPS)
    , bufSize(targetFPS)
    , ix(0)
{
    elapsedMicros = new long[bufSize];
    for (int i = 0; i < bufSize; ++i)
        elapsedMicros[i] = 0;
}

void FPS::frame_start()
{
    gettimeofday(&ts_start, nullptr);
}

static long calc_elapsed_usec(const timeval &start, const timeval &end)
{
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    long total_microseconds = seconds * 1000000 + microseconds;
    return total_microseconds;
}

long FPS::get_avg_elapsed()
{
    int i = ix - 1;
    if (i < 0) i = bufSize - 1;
    long sum = 0, cnt = 0;
    while (i != ix && elapsedMicros[i] != 0)
    {
        sum += elapsedMicros[i];
        cnt += 1;
        i = i - 1;
        if (i < 0) i = bufSize - 1;
    }
    if (cnt == 0) return 0;
    return sum / cnt;
}

void FPS::frame_end()
{
    timeval ts_end;
    gettimeofday(&ts_end, nullptr);
    long elapsed = calc_elapsed_usec(ts_start, ts_end);

    long to_store = elapsed < cycle_usec ? cycle_usec : elapsed;
    elapsedMicros[ix] = to_store;
    ix = (ix + 1) % bufSize;

    int extrapolated_fps = round(1000000.0 / (double)elapsed);
    double elapsed_msec = (double)elapsed / 1000.0;

    long avg_elapsed = get_avg_elapsed();
    double avg_fps = 1000000.0 / (double)avg_elapsed;
    printf("FPS %5.1f / last frame %.2f msec (~%d FPS)    \r", avg_fps, elapsed_msec, extrapolated_fps);

    if (elapsed >= cycle_usec) return;
    usleep(cycle_usec - elapsed);
}
