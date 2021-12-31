#pragma once

#include "lajolla.h"
#include <mutex>

/// For printing how much work is done for an operation.
/// The operations are thread-safe so we can safely use this in multi-thread environment.
class ProgressReporter {
    public:
    ProgressReporter(uint64_t total_work) : total_work(total_work), work_done(0) {
    }
    void update(uint64_t num) {
        std::lock_guard<std::mutex> lock(mutex);
        work_done += num;
        Real work_ratio = (Real)work_done / (Real)total_work;
        fprintf(stdout,
                "\r %.2f Percent Done (%llu / %llu)",
                work_ratio * Real(100.0),
                (unsigned long long)work_done,
                (unsigned long long)total_work);
    }
    void done() {
        work_done = total_work;
        fprintf(stdout,
                "\r %.2f Percent Done (%llu / %llu)\n",
                Real(100.0),
                (unsigned long long)work_done,
                (unsigned long long)total_work);
    }
    uint64_t get_work_done() const {
        return work_done;
    }

    private:
    const uint64_t total_work;
    uint64_t work_done;
    std::mutex mutex;
};
