#ifndef PERIODIC_TIMER_HPP
#define PERIODIC_TIMER_HPP

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

namespace msp {

class PeriodicTimer {
public:
    /**
     * @brief PeriodicTimer define a periodic timer
     * @param funct function that is called periodically
     * @param period_seconds period in seconds
     */
    PeriodicTimer(const std::function<void()> funct,
                  const double period_seconds);

    /**
     * @brief PeriodicTimer destructor
     */
    ~PeriodicTimer() { stop(); }

    /**
     * @brief start define and start background thread
     */
    bool start();

    /**
     * @brief stop tell thread to stop and wait for end
     */
    bool stop();

    /**
     * @brief getPeriod get period in seconds
     * @return period in seconds
     */
    double getPeriod() const { return period_us.count() / 1.e6; }

    /**
     * @brief setPeriod change the update period of timer thread
     * This will stop and restart the thread.
     * @param period_seconds period in seconds
     */
    void setPeriod(const double& period_seconds);

    /**
     * @brief setTimeScale scale the pace of ALL periodic timers, process-wide:
     * effective period = period / scale. For processes that follow a
     * simulation clock running at scale x real time, so that timers keep
     * their nominal rate in simulation time. Takes effect on each timer's
     * next cycle, without restarting threads. Clamped to [0.05, 20]
     * (a paused simulation must slow timers down, not freeze them).
     * @param scale simulation seconds per wall-clock second
     */
    static void setTimeScale(double scale);

    /**
     * @brief getTimeScale get the current process-wide time scale
     * @return scale in simulation seconds per wall-clock second
     */
    static double getTimeScale();

private:
    std::shared_ptr<std::thread> thread_ptr;
    std::function<void()> funct;
    std::chrono::duration<size_t, std::micro> period_us;
    std::timed_mutex mutex_timer;
    std::chrono::steady_clock::time_point tstart;

    std::atomic_flag running_ = ATOMIC_FLAG_INIT;

    static std::atomic<double> time_scale_;
};

}  // namespace msp

#endif
