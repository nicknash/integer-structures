#if !defined __TIMER_H

#define __TIMER_H

#include <sys/time.h>

class Timer
{
    struct timeval startTime;
public:
    void start();
    float elapsed();
};

#endif

