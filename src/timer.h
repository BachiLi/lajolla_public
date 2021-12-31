#pragma once

#include "lajolla.h"

#include <chrono>
#include <ctime>

/// For measuring how long an operation takes.
/// C++ chrono unfortunately makes the whole type system very complicated.
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
