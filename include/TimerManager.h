/**
 * @file TimerManager.h
 * @brief High-performance timer management using Linux timerfd and epoll
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once

 #include "Timer.h"
 #include <sys/timerfd.h>
 #include <sys/epoll.h>
 #include <unistd.h>
 #include <functional>
 #include <unordered_map>
 #include <thread>
 #include <atomic>
 #include <memory>
 #include <mutex>
 #include <stdexcept>
 
 namespace swt {
 class SLLooper;
 class TimerManager;
 
 /**
  * @class TimerManager
  * @brief Linux-specific timer management using timerfd and epoll for high performance
  * 
  * TimerManager provides efficient timer functionality by leveraging Linux kernel features:
  * - **timerfd**: Kernel-level timer with file descriptor interface
  * - **epoll**: Scalable I/O event notification for monitoring multiple timers
  * - **Separate thread**: Non-blocking timer processing independent of main event loop
  * 
  * Key features:
  * - Support for both one-shot and periodic timers
  * - Thread-safe timer management with atomic operations
  * - Automatic cleanup of expired/cancelled timers
  * - Integration with \ref swt::SLLooper "SLLooper" for callback execution
  * 
  * @code{.cpp}
  * auto looper = std::make_shared<SLLooper>();
  * auto timerMgr = std::make_shared<TimerManager>(looper);
  * 
  * // Create one-shot timer
  * auto callback = []() { std::cout << "Timer fired!" << std::endl; };
  * std::atomic<bool> cancelled{false};
  * TimerId id = timerMgr->createTimer(callback, 1000, false, &cancelled);
  * 
  * // Cancel if needed
  * timerMgr->cancelTimer(id);
  * @endcode
  * 
  * @note Linux-specific implementation requiring kernel version 2.6.25+
  * @warning Must be used with SLLooper for proper callback execution
  * 
  * @see \ref swt::Timer "Timer", \ref swt::SLLooper "SLLooper"
  */
 class TimerManager {
 public:
     /**
      * @struct TimerInfo
      * @brief Internal timer information structure
      * 
      * Contains all data needed to manage a single timer including
      * file descriptor, callback, timing parameters, and cancellation state.
      */
     struct TimerInfo {
         int fd;                           /**< timerfd file descriptor for kernel timer */
         std::function<void()> callback;   /**< callback function to execute on timer expiration */
         bool periodic;                    /**< true for periodic timer, false for one-shot */
         uint64_t interval_ms;             /**< interval in milliseconds for periodic timers */
         TimerId id;                       /**< unique timer identifier */
         std::atomic<bool>* cancelled;     /**< pointer to Timer object's cancellation flag */
     };
 
 private:
     int mEpollFd;                                        /**< epoll file descriptor for event monitoring */
     std::thread mTimerThread;                             /**< dedicated timer processing thread */
     std::atomic<bool> mRunning{true};                     /**< flag to control timer thread lifecycle */
     std::unordered_map<TimerId, TimerInfo> mTimers;       /**< map of active timers */
     std::mutex mTimersMutex;                              /**< mutex protecting timer map access */
     std::weak_ptr<SLLooper> mLooper;                      /**< weak reference to parent SLLooper */
     std::atomic<TimerId> mNextId{1};                      /**< atomic counter for unique timer IDs */
 
 public:
     /**
      * @brief Constructor - initializes epoll and starts timer thread
      * @param looper Weak reference to parent SLLooper for callback execution
      * @throws std::runtime_error if epoll creation fails
      * 
      * Creates epoll file descriptor and starts dedicated timer thread.
      * The weak_ptr prevents circular dependency with SLLooper.
      * 
      * @see \ref swt::SLLooper "SLLooper"
      */
     explicit TimerManager(std::weak_ptr<SLLooper> looper);
     
     /**
      * @brief Destructor - cleanup all timers and join timer thread
      * 
      * Performs orderly shutdown:
      * 1. Set mRunning to false to stop timer thread
      * 2. Cancel and cleanup all active timers
      * 3. Join timer thread
      * 4. Close epoll file descriptor
      */
     ~TimerManager();
 
     // ========== Internal API for Timer objects ==========
 
     /**
      * @brief Create a new timer with specified parameters
      * @param callback Function to call when timer expires
      * @param delay_ms Delay in milliseconds before first expiration
      * @param periodic true for repeating timer, false for one-shot
      * @param cancelled Pointer to atomic flag for cancellation checking
      * @return Unique TimerId for timer management
      * @throws std::runtime_error if timerfd creation or epoll registration fails
      * 
      * Creates Linux timerfd, configures timing parameters, adds to epoll
      * monitoring, and stores in internal timer map.
      * 
      * @note Thread-safe operation protected by mTimersMutex
      * @see \ref swt::TimerManager::cancelTimer "cancelTimer", \ref swt::TimerManager::restartTimer "restartTimer"
      */
     TimerId createTimer(std::function<void()> callback, uint64_t delay_ms, 
                        bool periodic, std::atomic<bool>* cancelled);
 
     /**
      * @brief Cancel and remove timer
      * @param id Timer identifier returned by createTimer()
      * @return true if timer was found and cancelled, false if not found
      * 
      * Removes timer from epoll monitoring, closes timerfd, and removes
      * from internal map. Safe to call multiple times on same timer.
      * 
      * @note Thread-safe operation
      * @see \ref swt::TimerManager::createTimer "createTimer"
      */
     bool cancelTimer(TimerId id);
 
     /**
      * @brief Check if timer exists and is active
      * @param id Timer identifier to check
      * @return true if timer exists in active timer map
      * 
      * @note Thread-safe read operation
      * @see \ref swt::TimerManager::createTimer "createTimer"
      */
     bool hasTimer(TimerId id);
 
     /**
      * @brief Restart existing timer with new delay
      * @param id Timer identifier to restart
      * @param delay_ms New delay in milliseconds
      * @return true if timer was found and restarted, false if not found
      * 
      * Updates existing timer's delay without recreating timerfd.
      * Maintains all other timer properties (periodic, callback, etc.).
      * 
      * @note Thread-safe operation
      * @see \ref swt::TimerManager::createTimer "createTimer"
      */
     bool restartTimer(TimerId id, uint64_t delay_ms);
 
     /**
      * @brief Get count of currently active timers
      * @return Number of timers in active state
      * 
      * Useful for debugging and monitoring timer usage.
      * 
      * @note Thread-safe operation
      */
     size_t getActiveTimerCount();
 
     /**
      * @brief Update cancellation pointer for moved Timer objects
      * @param id Timer identifier
      * @param newPtr New pointer to atomic cancellation flag
      * 
      * Called when Timer objects are moved to update internal
      * pointer to the new location of cancellation flag.
      * 
      * @note Thread-safe operation, handles Timer move semantics
      */
     void updateCancelledPtr(TimerId id, std::atomic<bool>* newPtr);
 
 private:
     // ========== Internal implementation methods ==========
 
     /**
      * @brief Main timer thread function - epoll event loop
      * 
      * Runs continuous epoll_wait loop with 100ms timeout:
      * 1. Monitor up to 64 timer events per iteration
      * 2. Handle timer expiration by posting callback to SLLooper
      * 3. Cleanup expired one-shot timers
      * 4. Continue until mRunning becomes false
      * 
      * @note Runs in separate thread, communicates with main thread via SLLooper
      * @see \ref swt::SLLooper "SLLooper"
      */
     void timerThreadFunc();
 
     /**
      * @brief Create and configure Linux timerfd
      * @param delay_ms Initial delay in milliseconds
      * @param periodic true for repeating timer, false for one-shot
      * @return File descriptor for created timer
      * @throws std::runtime_error if timerfd_create or timerfd_settime fails
      * 
      * Creates timerfd with CLOCK_MONOTONIC and configures initial timing.
      * For periodic timers, sets both initial delay and repeat interval.
      */
     int createTimerFd(uint64_t delay_ms, bool periodic);
 
     /**
      * @brief Handle timer expiration event
      * @param timerInfo Timer information for expired timer
      * 
      * Called when epoll detects timer expiration:
      * 1. Check if timer was cancelled
      * 2. Post callback to SLLooper for execution
      * 3. For one-shot timers, schedule cleanup
      * 4. Read timerfd to acknowledge expiration
      */
     void handleTimerExpired(const TimerInfo& timerInfo);
 
     /**
      * @brief Remove timer from internal data structures
      * @param id Timer identifier to cleanup
      * 
      * Internal cleanup function that removes timer from epoll,
      * closes file descriptor, and removes from timer map.
      * 
      * @note Must be called with appropriate locking
      */
     void cleanupTimer(TimerId id);
 
     /**
      * @brief Update existing timerfd with new timing parameters
      * @param fd Timer file descriptor to update
      * @param delay_ms New delay in milliseconds
      * @param periodic Periodic flag (should match original timer type)
      * @throws std::runtime_error if timerfd_settime fails
      * 
      * Updates timing parameters of existing timerfd without recreating
      * the file descriptor or epoll registration.
      */
     void updateTimerFd(int fd, uint64_t delay_ms, bool periodic);
 };
 
 } // namespace swt