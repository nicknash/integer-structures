#include "timer.h"
#include <cstdlib>

void Timer::start()
{
    gettimeofday(&startTime, NULL);
    return;
}

float Timer::elapsed()
{
    struct timeval t;
    gettimeofday(&t, NULL);
    return t.tv_sec - startTime.tv_sec + (t.tv_usec - startTime.tv_usec) / 1.e6;
}
