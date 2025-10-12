/**
 * @file TimerManager.h
 * @brief High-performance timer management using Linux timerfd+epoll or sigev_thread
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 * 
 * This file provides a high-performance timer management system with two configurable backends:
 * - timerfd + epoll (Linux-specific, high performance)  
 * - timer_create + sigev_thread (POSIX-compliant, portable)
 * 
 * The backend is selected at compile time via preprocessor macros.
 */

 #pragma once

 #include "Timer.h"
 #include <sys/timerfd.h>
 #include <sys/epoll.h>
 #include <unistd.h>
 #include <signal.h>
 #include <time.h>
 #include <functional>
 #include <unordered_map>
 #include <thread>
 #include <atomic>
 #include <memory>
 #include <mutex>
 #include <stdexcept>
 
 /**
  * @defgroup TimerBackendMacros Timer Backend Selection Macros
  * @brief Preprocessor macros for selecting timer backend implementation
  * @{
  */
 
 /** 
  * @def TIMER_USE_TIMERFD_EPOLL
  * @brief Enable timerfd + epoll backend (Linux-specific, high performance)
  * 
  * This backend uses Linux-specific timerfd and epoll for high-performance
  * timer management with a single dedicated thread.
  */
 
 /** 
  * @def TIMER_USE_SIGEV_THREAD
  * @brief Enable timer_create + sigev_thread backend (POSIX-compliant, portable)
  * 
  * This backend uses POSIX timer_create with SIGEV_THREAD for portable
  * timer management across different UNIX systems.
  */
 
 // ========== Timer Backend Selection ==========
 // Uncomment ONE of the following lines to select timer backend:
 
 //#define TIMER_USE_SIGEV_THREAD    ///< Use timer_create + sigev_thread
 #define TIMER_USE_TIMERFD_EPOLL     ///< Use timerfd + epoll (default)
 
 // Validate macro selection
 #if defined(TIMER_USE_SIGEV_THREAD) && defined(TIMER_USE_TIMERFD_EPOLL)
     #error "Cannot define both TIMER_USE_SIGEV_THREAD and TIMER_USE_TIMERFD_EPOLL"
 #endif
 
 #if !defined(TIMER_USE_SIGEV_THREAD) && !defined(TIMER_USE_TIMERFD_EPOLL)
     #define TIMER_USE_TIMERFD_EPOLL  ///< Default to timerfd+epoll
 #endif
 
 /** @} */ // end of TimerBackendMacros group
 
 /**
  * @namespace swt
  * @brief Software Timer namespace containing all timer-related classes
  */
 namespace swt {
 
 // Forward declaration
 class SLLooper;
 
 /**
  * @typedef TimerId
  * @brief Unique identifier type for timers
  * @details 64-bit unsigned integer used to uniquely identify timer instances
  */
 using TimerId = uint64_t;
 
 /**
  * @class TimerManager
  * @brief High-performance timer management with configurable backend
  * 
  * @details
  * TimerManager provides a unified interface for managing timers with two
  * different backend implementations:
  * 
  * **TIMERFD_EPOLL Backend (Linux):**
  * - Uses Linux timerfd + epoll for high performance
  * - Single dedicated thread processes all timer events
  * - Excellent scalability for many concurrent timers
  * - Low CPU overhead and precise timing
  * 
  * **SIGEV_THREAD Backend (POSIX):**
  * - Uses POSIX timer_create with SIGEV_THREAD
  * - Each timer callback runs in its own thread
  * - Portable across UNIX systems
  * - Higher resource usage but better compatibility
  * 
  * @par Thread Safety
  * All public methods are thread-safe and can be called from multiple threads
  * concurrently. Timer callbacks are executed on the SLLooper main thread.
  * 
  * @par Performance Characteristics
  * - TIMERFD_EPOLL: O(1) timer creation/deletion, O(log n) event processing
  * - SIGEV_THREAD: O(1) timer creation/deletion, O(1) event processing per timer
  * 
  * @par Example Usage
  * @code
  * auto looper = std::make_shared<SLLooper>();
  * TimerManager manager(looper);
  * 
  * // Create one-shot timer
  * std::atomic<bool> cancelled(false);
  * TimerId id = manager.createTimer([]() { 
  *     std::cout << "Timer fired!" << std::endl; 
  * }, 1000, false, &cancelled);
  * 
  * // Create periodic timer  
  * TimerId periodicId = manager.createTimer([]() {
  *     std::cout << "Periodic timer!" << std::endl;
  * }, 500, true, nullptr);
  * @endcode
  * 
  * @warning Timer callbacks must not block for extended periods as they run
  *          on the main event loop thread.
  * 
  * @see SLLooper
  * @see Timer
  */
 class TimerManager {
 public:
     /**
      * @struct TimerInfo
      * @brief Internal timer information structure
      * 
      * @details
      * This structure holds all information needed to manage a timer,
      * including backend-specific data and common properties.
      * The structure uses a union to minimize memory usage when
      * only one backend is compiled.
      * 
      * @note This is an internal structure and should not be used
      *       directly by client code.
      */
     struct TimerInfo {
 #ifdef TIMER_USE_TIMERFD_EPOLL
         int fd;                           /**< @brief timerfd file descriptor for Linux backend */
 #else
         timer_t timer;                    /**< @brief POSIX timer handle for sigev_thread backend */
         TimerId timerId;                  /**< @brief Timer ID for callback mapping in sigev backend */
 #endif
         std::function<void()> callback;   /**< @brief User callback function to execute on timer expiry */
         bool periodic;                    /**< @brief true for repeating timer, false for one-shot */
         uint64_t interval_ms;             /**< @brief Timer interval in milliseconds */
         TimerId id;                       /**< @brief Unique timer identifier */
         std::atomic<bool>* cancelled;     /**< @brief Pointer to cancellation flag (optional) */
     };
 
 private:
 #ifdef TIMER_USE_TIMERFD_EPOLL
     /**
      * @name TIMERFD_EPOLL Backend Members
      * @brief Private members specific to timerfd+epoll implementation
      * @{
      */
     
     int mEpollFd;                         /**< @brief epoll file descriptor for event monitoring */
     std::thread mTimerThread;             /**< @brief Dedicated thread for timer event processing */
     
     /** @} */
 #else
     /**
      * @name SIGEV_THREAD Backend Members  
      * @brief Private members specific to sigev_thread implementation
      * @{
      */
     
     /** @brief Global map from TimerId to TimerManager for callback routing */
     static std::unordered_map<TimerId, TimerManager*> sTimerManagerMap;
     
     /** @brief Mutex protecting the global timer manager map */
     static std::mutex sManagerMapMutex;
     
     /** @} */
 #endif
 
     /**
      * @name Common Private Members
      * @brief Private members shared by both backend implementations
      * @{
      */
     
     std::atomic<bool> mRunning{true};                    /**< @brief Flag indicating if timer manager is running */
     std::unordered_map<TimerId, TimerInfo> mTimers;      /**< @brief Map of active timers by ID */
     std::mutex mTimersMutex;                             /**< @brief Mutex protecting the timers map */
     std::weak_ptr<SLLooper> mLooper;                     /**< @brief Weak reference to parent SLLooper */
     std::atomic<TimerId> mNextId{1};                     /**< @brief Next available timer ID (atomic counter) */
     
     /** @} */
 
 public:
     /**
      * @brief Construct a new TimerManager
      * 
      * @param looper Weak reference to parent SLLooper for callback posting
      * 
      * @details
      * Initializes the timer management system with the compile-time selected
      * backend. For TIMERFD_EPOLL, creates epoll instance and starts timer
      * thread. For SIGEV_THREAD, performs minimal initialization.
      * 
      * @throws std::runtime_error If backend initialization fails (e.g., epoll_create fails)
      * 
      * @par Example
      * @code
      * auto looper = std::make_shared<SLLooper>();
      * TimerManager manager(looper);  // Automatically uses configured backend
      * @endcode
      * 
      * @warning The SLLooper must remain alive for the lifetime of TimerManager
      * 
      * @see ~TimerManager()
      */
     explicit TimerManager(std::weak_ptr<SLLooper> looper);
     
     /**
      * @brief Destroy the TimerManager and cleanup all resources
      * 
      * @details
      * Safely shuts down the timer system by:
      * 1. Setting running flag to false
      * 2. Cancelling all active timers  
      * 3. Cleaning up backend-specific resources
      * 4. Joining timer thread (TIMERFD_EPOLL only)
      * 
      * All timer callbacks in progress will complete before destruction finishes.
      * 
      * @note Destructor is thread-safe and can be called while timers are active
      * 
      * @par Cleanup Details
      * - **TIMERFD_EPOLL**: Closes all timerfd handles, joins timer thread, closes epoll fd
      * - **SIGEV_THREAD**: Deletes all POSIX timers, removes from global map
      * 
      * @see TimerManager()
      */
     ~TimerManager();
 
     /**
      * @brief Create a new timer
      * 
      * @param callback Function to call when timer expires (must not be null)
      * @param delay_ms Delay in milliseconds before first expiration (must be > 0)
      * @param periodic true for repeating timer, false for one-shot timer
      * @param cancelled Optional pointer to atomic bool for external cancellation
      * 
      * @return TimerId Unique timer identifier (0 if creation failed)
      * 
      * @details
      * Creates and starts a new timer that will call the provided callback after
      * the specified delay. For periodic timers, the callback will be called
      * repeatedly at the specified interval until cancelled.
      * 
      * The callback will be executed on the SLLooper main thread, ensuring
      * thread safety with other event loop operations.
      * 
      * @par Cancellation Mechanism
      * If a cancelled pointer is provided, the timer will check this flag
      * before executing the callback. This allows for external cancellation
      * even after the timer has fired but before callback execution.
      * 
      * @par Backend Behavior
      * - **TIMERFD_EPOLL**: Creates timerfd, configures timing, adds to epoll
      * - **SIGEV_THREAD**: Creates POSIX timer with SIGEV_THREAD notification
      * 
      * @par Error Conditions
      * Returns 0 if:
      * - callback is null
      * - System timer creation fails (out of resources, invalid parameters)
      * - Backend-specific errors (epoll_ctl failure, timer_create failure)
      * 
      * @par Example
      * @code
      * // One-shot timer
      * TimerId id = manager.createTimer([]() {
      *     std::cout << "Timer fired once!" << std::endl;
      * }, 1000, false, nullptr);
      * 
      * // Periodic timer with cancellation
      * std::atomic<bool> cancelled(false);
      * TimerId periodicId = manager.createTimer([&]() {
      *     std::cout << "Periodic: " << std::chrono::steady_clock::now() << std::endl;
      * }, 500, true, &cancelled);
      * 
      * // Cancel after some time
      * std::this_thread::sleep_for(std::chrono::seconds(2));
      * cancelled = true;
      * @endcode
      * 
      * @warning Callback must not block for extended periods
      * @warning Do not capture TimerManager or Timer objects in callback to avoid cycles
      * 
      * @see cancelTimer()
      * @see restartTimer()
      */
     TimerId createTimer(std::function<void()> callback, uint64_t delay_ms, 
                        bool periodic, std::atomic<bool>* cancelled);
 
     /**
      * @brief Cancel an active timer
      * 
      * @param id Timer ID returned by createTimer()
      * 
      * @return true if timer was found and cancelled successfully
      * @return false if timer ID not found or already cancelled
      * 
      * @details
      * Immediately stops the specified timer and removes it from the system.
      * If the timer callback is currently executing, this method will wait
      * for it to complete before returning.
      * 
      * After cancellation, the timer ID becomes invalid and should not be used.
      * 
      * @par Thread Safety
      * This method is thread-safe and can be called from any thread, including
      * from within timer callbacks.
      * 
      * @par Backend Behavior
      * - **TIMERFD_EPOLL**: Removes from epoll, closes timerfd, removes from map
      * - **SIGEV_THREAD**: Calls timer_delete(), removes from global map
      * 
      * @par Example
      * @code
      * TimerId id = manager.createTimer(callback, 1000, false, nullptr);
      * 
      * // Cancel immediately
      * if (manager.cancelTimer(id)) {
      *     std::cout << "Timer cancelled successfully" << std::endl;
      * }
      * 
      * // Attempting to cancel again returns false
      * bool result = manager.cancelTimer(id);  // false
      * @endcode
      * 
      * @note Cancelling an invalid ID is safe and returns false
      * 
      * @see createTimer()
      * @see hasTimer()
      */
     bool cancelTimer(TimerId id);
 
     /**
      * @brief Check if a timer exists and is active
      * 
      * @param id Timer ID to check
      * 
      * @return true if timer exists and is active
      * @return false if timer ID not found, cancelled, or expired (for one-shot)
      * 
      * @details
      * Queries whether the specified timer ID corresponds to an active timer.
      * This is useful for checking timer status before performing operations.
      * 
      * @par Thread Safety
      * This method is thread-safe and provides a consistent snapshot of timer state.
      * 
      * @par Example
      * @code
      * TimerId id = manager.createTimer(callback, 5000, false, nullptr);
      * 
      * if (manager.hasTimer(id)) {
      *     std::cout << "Timer is still active" << std::endl;
      *     manager.cancelTimer(id);
      * }
      * @endcode
      * 
      * @note Result may become stale immediately after return due to concurrent operations
      * 
      * @see createTimer()
      * @see cancelTimer()
      * @see getActiveTimerCount()
      */
     bool hasTimer(TimerId id);
 
     /**
      * @brief Restart an existing timer with new delay
      * 
      * @param id Timer ID to restart
      * @param delay_ms New delay in milliseconds
      * 
      * @return true if timer was found and restarted successfully
      * @return false if timer ID not found
      * 
      * @details
      * Resets an existing timer to fire after the new specified delay.
      * The timer becomes a one-shot timer regardless of its original periodic setting.
      * If the timer was cancelled via external flag, the cancellation is reset.
      * 
      * This operation is more efficient than cancelling and creating a new timer
      * as it reuses the existing timer resources.
      * 
      * @par Thread Safety
      * This method is thread-safe and can be called from any thread.
      * 
      * @par Backend Behavior
      * - **TIMERFD_EPOLL**: Updates timerfd with new interval using timerfd_settime()
      * - **SIGEV_THREAD**: Updates POSIX timer with new interval using timer_settime()
      * 
      * @par Example
      * @code
      * TimerId id = manager.createTimer(callback, 1000, true, nullptr);
      * 
      * // Change to fire in 5 seconds (becomes one-shot)
      * if (manager.restartTimer(id, 5000)) {
      *     std::cout << "Timer restarted with 5s delay" << std::endl;
      * }
      * @endcode
      * 
      * @warning Restarted timer always becomes one-shot, losing periodic behavior
      * 
      * @see createTimer()
      * @see cancelTimer()
      */
     bool restartTimer(TimerId id, uint64_t delay_ms);
 
     /**
      * @brief Update the cancellation flag pointer for an existing timer
      * 
      * @param id Timer ID to update
      * @param newPtr New pointer to atomic bool cancellation flag
      * 
      * @details
      * Updates the cancellation flag pointer for an existing timer. This allows
      * changing the external cancellation mechanism after timer creation.
      * 
      * The new pointer will be checked before each callback execution.
      * Pass nullptr to disable external cancellation for the timer.
      * 
      * @par Thread Safety
      * This method is thread-safe. The pointer update is atomic.
      * 
      * @par Use Cases
      * - Changing ownership of cancellation control
      * - Disabling external cancellation (pass nullptr)
      * - Updating to new cancellation flag after object moves
      * 
      * @par Example
      * @code
      * std::atomic<bool> cancelled1(false);
      * TimerId id = manager.createTimer(callback, 1000, true, &cancelled1);
      * 
      * // Change to different cancellation flag
      * std::atomic<bool> cancelled2(false);
      * manager.updateCancelledPtr(id, &cancelled2);
      * 
      * // Disable external cancellation
      * manager.updateCancelledPtr(id, nullptr);
      * @endcode
      * 
      * @warning Ensure the new pointer remains valid for timer lifetime
      * @warning Old pointer may still be checked if callback is in progress
      * 
      * @see createTimer()
      */
     void updateCancelledPtr(TimerId id, std::atomic<bool>* newPtr);
 
     /**
      * @brief Get the number of currently active timers
      * 
      * @return size_t Number of active timers managed by this instance
      * 
      * @details
      * Returns the current count of active timers. This includes both one-shot
      * and periodic timers that have not yet been cancelled or expired.
      * 
      * @par Thread Safety
      * This method is thread-safe and returns a consistent snapshot at call time.
      * 
      * @par Performance
      * This is an O(1) operation that returns the size of the internal timer map.
      * 
      * @par Example
      * @code
      * std::cout << "Active timers: " << manager.getActiveTimerCount() << std::endl;
      * 
      * // Create some timers
      * auto id1 = manager.createTimer(callback, 1000, false, nullptr);
      * auto id2 = manager.createTimer(callback, 2000, true, nullptr);
      * 
      * std::cout << "After creation: " << manager.getActiveTimerCount() << std::endl;  // 2
      * 
      * manager.cancelTimer(id1);
      * std::cout << "After cancel: " << manager.getActiveTimerCount() << std::endl;    // 1
      * @endcode
      * 
      * @note Count may change immediately after return due to concurrent operations
      * 
      * @see hasTimer()
      * @see createTimer()
      * @see cancelTimer()
      */
     size_t getActiveTimerCount();
 
     /**
      * @brief Get the name of the currently compiled timer backend
      * 
      * @return const char* Backend name string ("TIMERFD_EPOLL" or "SIGEV_THREAD")
      * 
      * @details
      * Returns a string identifying which timer backend was selected at compile time.
      * This is useful for logging, debugging, and runtime feature detection.
      * 
      * @par Return Values
      * - "TIMERFD_EPOLL" - Linux timerfd + epoll backend
      * - "SIGEV_THREAD" - POSIX timer_create + sigev_thread backend
      * 
      * @par Example
      * @code
      * std::cout << "Timer backend: " << TimerManager::getBackendName() << std::endl;
      * 
      * if (strcmp(TimerManager::getBackendName(), "TIMERFD_EPOLL") == 0) {
      *     std::cout << "Using high-performance Linux backend" << std::endl;
      * }
      * @endcode
      * 
      * @note This is a static method and can be called without TimerManager instance
      * 
      * @see SLLooper::getTimerBackend()
      */
     static const char* getBackendName() {
 #ifdef TIMER_USE_TIMERFD_EPOLL
         return "TIMERFD_EPOLL";
 #else
         return "SIGEV_THREAD";
 #endif
     }
 
 private:
 #ifdef TIMER_USE_TIMERFD_EPOLL
     /**
      * @name TIMERFD_EPOLL Private Methods
      * @brief Private implementation methods for timerfd+epoll backend
      * @{
      */
     
     /**
      * @brief Main timer thread function for epoll event processing
      * 
      * @details
      * Runs in dedicated thread, processing timer events from epoll.
      * Continues until mRunning becomes false.
      */
     void timerThreadFunc();
     
     /**
      * @brief Create and configure a timerfd
      * 
      * @param delay_ms Initial delay in milliseconds  
      * @param periodic true for repeating timer
      * @return int timerfd file descriptor (-1 on error)
      */
     int createTimerFd(uint64_t delay_ms, bool periodic);
     
     /**
      * @brief Update existing timerfd timing configuration
      * 
      * @param fd timerfd file descriptor
      * @param delay_ms New delay in milliseconds
      * @param periodic true for repeating timer
      */
     void updateTimerFd(int fd, uint64_t delay_ms, bool periodic);
     
     /** @} */
 #else
     /**
      * @name SIGEV_THREAD Private Methods  
      * @brief Private implementation methods for sigev_thread backend
      * @{
      */
     
     /**
      * @brief Create POSIX timer with sigev_thread notification
      * 
      * @param id Timer ID for callback identification
      * @return timer_t POSIX timer handle (cast to -1 on error)
      */
     timer_t createSigevTimer(TimerId id);
     
     /**
      * @brief Update POSIX timer timing configuration
      * 
      * @param timer POSIX timer handle
      * @param delay_ms Delay in milliseconds
      * @param periodic true for repeating timer
      */
     void updateSigevTimer(timer_t timer, uint64_t delay_ms, bool periodic);
     
     /**
      * @brief Static callback function for POSIX timer notifications
      * 
      * @param sv sigval structure containing timer ID
      * 
      * @details
      * This function is called by the system when a POSIX timer expires.
      * It looks up the appropriate TimerManager and forwards the event.
      */
     static void sigevTimerCallback(sigval_t sv);
     
     /** @} */
 #endif
 
     /**
      * @name Common Private Methods
      * @brief Private methods shared by both backend implementations
      * @{
      */
     
     /**
      * @brief Handle timer expiration by posting callback to main thread
      * 
      * @param timerInfo Timer information structure
      * 
      * @details
      * Posts the timer callback to SLLooper main thread for execution.
      * Performs final cancellation checks before posting.
      */
     void handleTimerExpired(const TimerInfo& timerInfo);
     
     /**
      * @brief Clean up timer resources and remove from active set
      * 
      * @param id Timer ID to cleanup
      * 
      * @details
      * Performs backend-specific cleanup and removes timer from active map.
      * Called when timer expires (one-shot) or is cancelled.
      */
     void cleanupTimer(TimerId id);
     
     /** @} */
 };
 
 } // namespace swt