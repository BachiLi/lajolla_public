#pragma once

#include "vector.h"

#include <mutex>
#include <functional>
#include <atomic>

// From https://github.com/mmp/pbrt-v3/blob/master/src/core/parallel.h
extern thread_local int ThreadIndex;

void parallel_for(const std::function<void(int64_t)> &func, int64_t count, int64_t chunk_size = 1);
void parallel_for(std::function<void(Vector2i)> func, const Vector2i count);

void parallel_init(int num_threads);
void parallel_cleanup();
