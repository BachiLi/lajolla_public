#pragma once

#include "lajolla.h"

#include <chrono>
#include <ctime>

struct Timer {
    std::chrono::time_point<std::chrono::system_clock> last;
};

inline Real tick(Timer &timer) {
    std::chrono::time_point<std::chrono::system_clock> now = std::chrono::system_clock::now();
    std::chrono::duration<double> elapsed = now - timer.last;
    Real ret = elapsed.count();
    timer.last = now;
    return ret;
}
