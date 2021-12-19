#pragma once

#include "vector.h"

#include <mutex>
#include <functional>
#include <atomic>

// From https://github.com/mmp/pbrt-v3/blob/master/src/core/parallel.h

class Barrier {
  public:
    Barrier(int count) : count(count) { assert(count > 0); }
    ~Barrier() { assert(count == 0); }
    void Wait();

  private:
    std::mutex mutex;
    std::condition_variable cv;
    int count;
};

void parallel_for(const std::function<void(int64_t)> &func, int64_t count, int64_t chunkSize = 1);
extern thread_local int ThreadIndex;
void parallel_for(std::function<void(Vector2i)> func, const Vector2i count);

void parallel_init(int num_threads);
void parallel_cleanup();
