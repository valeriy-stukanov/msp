#include "PeriodicTimer.hpp"

#include <algorithm>
#include <cstdint>

namespace msp {

std::atomic<double> PeriodicTimer::time_scale_{1.0};

PeriodicTimer::PeriodicTimer(std::function<void()> funct,
                             const double period_seconds) :
    funct(funct) {
    period_us =
        std::chrono::duration<size_t, std::micro>(size_t(period_seconds * 1e6));
}

void PeriodicTimer::setTimeScale(const double scale) {
    time_scale_.store(std::clamp(scale, 0.05, 20.0),
                      std::memory_order_relaxed);
}

double PeriodicTimer::getTimeScale() {
    return time_scale_.load(std::memory_order_relaxed);
}

bool PeriodicTimer::start() {
    // only start thread if period is above 0
    if(!(period_us.count() > 0)) return false;
    // only start once
    if(running_.test_and_set()) return false;
    // lock mutex so that the try_lock_until in the new thread always times out
    // and loops
    mutex_timer.lock();
    // start the thread
    thread_ptr = std::shared_ptr<std::thread>(new std::thread([this] {
        // log now.
        tstart = std::chrono::steady_clock::now();
        while(true) {
            // call function
            funct();
            // increment the reference time to know when to timeout waiting,
            // paced by the process-wide time scale (see setTimeScale)
            tstart += std::chrono::microseconds(static_cast<int64_t>(
                period_us.count() /
                time_scale_.load(std::memory_order_relaxed)));
            // wait until end of period or stop is called
            if(mutex_timer.try_lock_until(tstart)) {
                // gets here if lock was acquired (means someone called stop and
                // manually unlocked the mutex)
                mutex_timer.unlock();
                break;
            }
        }  // function over, return
    }));
    return true;
}

bool PeriodicTimer::stop() {
    bool rc = false;
    if(running_.test_and_set()) {
        // was already running, so let the thread finish
        mutex_timer.unlock();
        if(thread_ptr != nullptr && thread_ptr->joinable()) {
            thread_ptr->join();
        }
        rc = true;
    }
    running_.clear();
    return rc;
}

void PeriodicTimer::setPeriod(const double& period_seconds) {
    stop();
    period_us =
        std::chrono::duration<size_t, std::micro>(size_t(period_seconds * 1e6));
    start();
}

}  // namespace msp
