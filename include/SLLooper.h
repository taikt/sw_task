/**
 * @file SLLooper.h
 * @brief Main event loop coordinator for asynchronous task management and timer operations
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once
 #include <memory>
 #include <thread>
 #include <future>
 #include <atomic>
 #include <chrono> 
 #include <type_traits> 
 #include "Timer.h"
 #include "EventQueue.h"
 
 namespace swt {
 class TimerManager;
 template<typename T> class Promise;
 
 /**
  * @class SLLooper
  * @brief Central event loop coordinator providing asynchronous task execution and timer management
  * 
  * SLLooper is the core component of the SW Task Framework that coordinates:
  * - **Message queue processing**: Unified queue for messages and function tasks
  * - **Timer management**: High-performance Linux timerfd-based timers
  * - **Asynchronous execution**: Function posting with futures and promises
  * - **CPU-bound task handling**: Specialized executor for long-running tasks
  * - **Thread safety**: Safe cross-thread communication via message passing
  * 
  * The SLLooper follows the event loop pattern, continuously processing queued
  * tasks and timer events in a single thread while providing thread-safe APIs
  * for task submission from other threads.
  * 
  * Key design principles:
  * - **Non-blocking operations**: All APIs return immediately with futures/promises
  * - **RAII resource management**: Automatic cleanup of timers and tasks
  * - **Boost-style timer API**: Familiar interface for timer operations
  * - **Template-based**: Type-safe function posting with perfect forwarding
  * 
  * @code{.cpp}
  * // Create event loop
  * auto looper = std::make_shared<SLLooper>();
  * 
  * // Post async function
  * auto future = looper->post([](int x) { return x * 2; }, 21);
  * int result = future.get(); // result = 42
  * 
  * // Add timer
  * auto timer = looper->addTimer([]() { 
  *     std::cout << "Timer fired!" << std::endl; 
  * }, 1000ms);
  * 
  * // CPU-bound task with promise
  * auto promise = looper->postWork([]() { 
  *     return heavyComputation(); 
  * });
  * 
  * promise.then([](auto result) {
  *     std::cout << "Work completed: " << result << std::endl;
  * });
  * @endcode
  * 
  * @note Thread-safe for task submission, but not for direct member access
  * @warning Must be created as shared_ptr due to enable_shared_from_this usage
  * 
  * @see \ref swt::EventQueue "EventQueue", \ref swt::TimerManager "TimerManager", \ref swt::Timer "Timer", \ref swt::Promise "Promise"
  */
 class SLLooper : public std::enable_shared_from_this<SLLooper>
 {
 public:
     /**
      * @brief Constructor - initializes message queue and starts event loop
      * 
      * Creates EventQueue, starts the main event loop thread, and initializes
      * internal state. TimerManager is created lazily on first timer request.
      * 
      * @note Must be created as shared_ptr: auto looper = std::make_shared<SLLooper>()
      */
     SLLooper();
     
     /**
      * @brief Destructor - stops event loop and cleanup resources
      * 
      * Performs orderly shutdown:
      * 1. Stop the main event loop thread
      * 2. Cancel all active timers via TimerManager
      * 3. Cleanup message queue and other resources
      */
     ~SLLooper();
     
     /**
      * @brief Get access to the underlying event queue
      * @return std::shared_ptr<EventQueue> Shared pointer to the event queue
      * 
      * Provides direct access to the underlying \ref swt::EventQueue "EventQueue" for advanced operations
      * that require direct queue manipulation or integration with legacy message-based code.
      * Most users should prefer the high-level \ref swt::SLLooper::post "post()" methods instead.
      * 
      * Use cases for direct queue access:
      * - **Legacy integration**: Working with existing message-based code
      * - **Advanced operations**: Custom message handling or queue manipulation
      * - **Performance optimization**: Bypassing SLLooper abstractions when needed
      * - **Testing**: Direct queue state inspection for unit tests
      * 
      * @code{.cpp}
      * auto looper = std::make_shared<SLLooper>();
      * auto queue = looper->getEventQueue();
      * 
      * // Direct message enqueueing (legacy style)
      * auto message = std::make_shared<Message>();
      * queue->enqueueMessage(message, uptimeMicros() + 1000000);
      * 
      * // Direct function enqueueing (bypassing SLLooper)
      * auto future = queue->enqueueFunction([]() { return 42; });
      * @endcode
      * 
      * @note Use with caution - direct queue access bypasses SLLooper's abstractions
      * @note Prefer SLLooper's post() methods for most use cases
      * @note Thread-safe operation (EventQueue is thread-safe)
      * @warning Direct queue manipulation may interfere with SLLooper's internal operations
      * 
      * @see \ref swt::EventQueue "EventQueue", \ref swt::SLLooper::post "post()", \ref swt::SLLooper::postDelayed "postDelayed()"
      */
     std::shared_ptr<EventQueue> getEventQueue();
 
     // ========== INTERNAL DEBUG API ==========
     
     /**
      * @brief Internal post function with debug info for CPU-bound detection
      * @tparam F Function type (auto-deduced)
      * @tparam Args Variadic argument types (auto-deduced)
      * @param func Function to execute asynchronously
      * @param args Arguments to forward to the function
      * @param file Source file name (for debug output)
      * @param line Source line number (for debug output)
      * @param funcname Function name (for debug output)
      * @return std::future<ReturnType> Future containing the function result
      * 
      * Internal implementation that adds CPU-bound task detection in debug builds.
      * Warns if task execution exceeds CPU_BOUND_DETECT_THRESHOLD_MS (3000ms).
      * 
      * @note For internal use - prefer \ref swt::SLLooper::post "post()" for normal usage
      * @note Implementation in SLLooper.tpp
      */
     template<typename F, typename... Args>
     auto post_internal(F&& func, Args&&... args, const char* file, int line, const char* funcname)
         -> std::future<decltype(func(args...))>;
 
     /**
      * @brief Internal postDelayed function with debug info for CPU-bound detection
      * @tparam F Function type (auto-deduced)
      * @tparam Args Variadic argument types (auto-deduced)
      * @param delayMs Delay in milliseconds before execution
      * @param func Function to execute asynchronously
      * @param args Arguments to forward to the function
      * @param file Source file name (for debug output)
      * @param line Source line number (for debug output)
      * @param funcname Function name (for debug output)
      * @return std::future<ReturnType> Future containing the function result
      * 
      * Internal implementation for delayed execution with CPU-bound detection.
      * 
      * @note For internal use - prefer \ref swt::SLLooper::postDelayed "postDelayed()" for normal usage
      * @note Implementation in SLLooper.tpp
      */
     template<typename F, typename... Args>
     auto postDelayed_internal(int64_t delayMs, F&& func, Args&&... args, const char* file, int line, const char* funcname)
         -> std::future<decltype(func(args...))>;
 
     // ========== PUBLIC ASYNC API ==========
 
     /**
      * @brief Post function for immediate asynchronous execution
      * @tparam F Function type (auto-deduced)
      * @tparam Args Variadic argument types (auto-deduced)
      * @param func Callable object to execute asynchronously
      * @param args Arguments to forward to the function
      * @return std::future<ReturnType> Future containing the function result
      * 
      * Posts a function to the message queue for immediate asynchronous execution
      * in the event loop thread. Uses perfect forwarding to preserve argument types
      * and supports both callable objects and member functions.
      * 
      * @code{.cpp}
      * // Lambda with capture
      * auto future1 = looper->post([x = 42](int y) { return x + y; }, 8);
      * 
      * // Free function
      * auto future2 = looper->post(std::sin, 3.14159);
      * 
      * // Member function
      * MyClass obj;
      * auto future3 = looper->post(&MyClass::method, &obj, arg1, arg2);
      * @endcode
      * 
      * @note Thread-safe operation
      * @note Function executes in event loop thread context
      * @note Implementation in SLLooper.tpp
      * @see \ref swt::SLLooper::postDelayed "postDelayed()", \ref swt::SLLooper::postWithTimeout "postWithTimeout()"
      */
     template<typename F, typename... Args>
     auto post(F&& func, Args&&... args) -> std::future<decltype(func(args...))>;
     
     /**
      * @brief Post function for delayed asynchronous execution
      * @tparam F Function type (auto-deduced)
      * @tparam Args Variadic argument types (auto-deduced)
      * @param delayMs Delay in milliseconds before execution
      * @param func Callable object to execute asynchronously
      * @param args Arguments to forward to the function
      * @return std::future<ReturnType> Future containing the function result
      * 
      * Posts a function to the message queue for delayed asynchronous execution.
      * The function will be executed after the specified delay in the event loop thread.
      * 
      * @code{.cpp}
      * // Execute after 1 second
      * auto future = looper->postDelayed(1000, []() { 
      *     return "Hello after delay!"; 
      * });
      * 
      * // Get result (blocks until execution completes)
      * std::string result = future.get();
      * @endcode
      * 
      * @note Thread-safe operation
      * @note Delay is measured from when the function is posted
      * @note Implementation in SLLooper.tpp
      * @see \ref swt::SLLooper::post "post()"
      */
     template<typename F, typename... Args>
     auto postDelayed(int64_t delayMs, F&& func, Args&&... args) -> std::future<decltype(func(args...))>;
     
     // ========== PROMISE API ==========
 
     /**
      * @brief Create a new promise object for manual result setting
      * @tparam T Value type for the promise
      * @return swt::Promise<T> New promise object
      * 
      * Creates a new promise object that can be resolved manually from any thread.
      * Useful for integrating with callback-based APIs or complex async workflows.
      * 
      * @code{.cpp}
      * auto promise = looper->createPromise<int>();
      * 
      * // Set up continuation
      * promise.then([](int result) {
      *     std::cout << "Promise resolved with: " << result << std::endl;
      * });
      * 
      * // Resolve from another thread
      * std::thread([promise]() mutable {
      *     std::this_thread::sleep_for(1s);
      *     promise.set_value(42);
      * }).detach();
      * @endcode
      * 
      * @note Implementation in SLLooper.tpp
      * @see \ref swt::Promise "Promise", \ref swt::Future "Future"
      */
     template<typename T>
     swt::Promise<T> createPromise();
     
     // ========== CPU-BOUND TASK API ==========
 
     /**
      * @brief Execute CPU-intensive task asynchronously
      * @tparam Func Function type (auto-deduced)
      * @param func CPU-intensive function to execute
      * @return swt::Promise<ReturnType> Promise for result retrieval and chaining
      * 
      * Executes CPU-intensive tasks using a dedicated thread pool to avoid
      * blocking the main event loop. Returns a promise for result handling
      * and continuation chaining.
      * 
      * @code{.cpp}
      * auto promise = looper->postWork([]() {
      *     // CPU-intensive computation
      *     return fibonacci(45);
      * });
      * 
      * promise.then([](long result) {
      *     std::cout << "Computation result: " << result << std::endl;
      * }).catch_error([](std::exception_ptr ex) {
      *     std::cerr << "Computation failed!" << std::endl;
      * });
      * @endcode
      * 
      * @note Uses separate thread pool, doesn't block event loop
      * @note Implementation in SLLooper.tpp
      * @see \ref swt::CpuTaskExecutor "CpuTaskExecutor", \ref swt::Promise "Promise"
      */
     template<typename Func>
     auto postWork(Func&& func) -> swt::Promise<decltype(func())>;
 
     /**
      * @brief Execute CPU-intensive task with timeout
      * @tparam Func Function type (auto-deduced)
      * @param func CPU-intensive function to execute
      * @param timeout Maximum execution time
      * @return swt::Promise<ReturnType> Promise for result retrieval
      * 
      * Executes CPU-intensive tasks with a timeout limit. If the task
      * doesn't complete within the timeout, the promise is rejected
      * with a timeout exception.
      * 
      * @code{.cpp}
      * auto promise = looper->postWork([]() {
      *     return longRunningComputation();
      * }, 5000ms);
      * 
      * promise.then([](auto result) {
      *     std::cout << "Completed in time: " << result << std::endl;
      * }).catch_error([](std::exception_ptr ex) {
      *     // Handle timeout or other errors
      * });
      * @endcode
      * 
      * @note Implementation in SLLooper.tpp
      * @see \ref swt::CpuTaskExecutor "CpuTaskExecutor", \ref swt::Promise "Promise"
      */
     template<typename Func>
     auto postWork(Func&& func, std::chrono::milliseconds timeout) 
         -> swt::Promise<decltype(func())>;
 
     /**
      * @brief Run one iteration of the event loop
      * @return true if loop should continue, false to exit
      * 
      * Processes one batch of messages and timer events. Used internally
      * by the event loop thread. Can be called manually for custom loop control.
      * 
      * @note Not typically called by user code
      */
     bool loop();
 
     /**
      * @brief Request event loop to exit
      * 
      * Signals the event loop to stop processing and exit gracefully.
      * The loop will finish processing current messages before stopping.
      */
     void exit();
 
     // ========== TIMER API (Boost-style) ==========
 
     /**
      * @brief Add one-shot timer with millisecond precision
      * @param callback Function to call when timer expires
      * @param delay_ms Delay in milliseconds before timer expiration
      * @return Timer RAII object for timer management
      * 
      * Creates a one-shot timer that fires once after the specified delay.
      * The callback executes in the event loop thread context.
      * 
      * @code{.cpp}
      * Timer timer = looper->addTimer([]() {
      *     std::cout << "Timer fired after 1 second!" << std::endl;
      * }, 1000);
      * 
      * // Timer can be cancelled
      * if (shouldCancel) {
      *     timer.cancel();
      * }
      * @endcode
      * 
      * @note Timer automatically cancels when Timer object is destroyed
      * @see \ref swt::Timer "Timer", \ref swt::TimerManager "TimerManager"
      */
     Timer addTimer(std::function<void()> callback, uint64_t delay_ms);
 
     /**
      * @brief Add one-shot timer with chrono duration
      * @tparam Rep Duration representation type (e.g., int, long)
      * @tparam Period Duration period type (e.g., std::milli, std::micro)
      * @param callback Function to call when timer expires
      * @param delay Chrono duration (e.g., 500ms, 1s, 100us)
      * @return Timer RAII object for timer management
      * 
      * Creates a one-shot timer using chrono duration types for type-safe
      * timing specifications. Supports various duration types automatically.
      * 
      * @code{.cpp}
      * using namespace std::chrono_literals;
      * 
      * // Various duration types
      * auto timer1 = looper->addTimer(callback, 500ms);
      * auto timer2 = looper->addTimer(callback, 1s);
      * auto timer3 = looper->addTimer(callback, 2min);
      * @endcode
      * 
      * @note Implementation in SLLooper.tpp
      * @note Duration is converted to milliseconds internally
      * @see \ref swt::Timer "Timer"
      */
     template<typename Rep, typename Period>
     Timer addTimer(std::function<void()> callback, 
                    const std::chrono::duration<Rep, Period>& delay);
 
     /**
      * @brief Add periodic timer with millisecond interval
      * @param callback Function to call on each timer expiration
      * @param interval_ms Interval in milliseconds between expirations
      * @return Timer RAII object for timer management
      * 
      * Creates a periodic timer that fires repeatedly at the specified interval.
      * The callback executes in the event loop thread context on each expiration.
      * 
      * @code{.cpp}
      * Timer periodicTimer = looper->addPeriodicTimer([]() {
      *     std::cout << "Periodic tick!" << std::endl;
      * }, 1000);  // Every second
      * 
      * // Stop after some time
      * looper->postDelayed(10000, [&periodicTimer]() {
      *     periodicTimer.cancel();
      * });
      * @endcode
      * 
      * @note Timer continues until cancelled or Timer object is destroyed
      * @see \ref swt::Timer "Timer"
      */
     Timer addPeriodicTimer(std::function<void()> callback, uint64_t interval_ms);
 
     /**
      * @brief Add periodic timer with chrono interval
      * @tparam Rep Duration representation type
      * @tparam Period Duration period type
      * @param callback Function to call on each timer expiration
      * @param interval Chrono interval between expirations
      * @return Timer RAII object for timer management
      * 
      * Creates a periodic timer using chrono duration types for type-safe
      * interval specifications.
      * 
      * @code{.cpp}
      * using namespace std::chrono_literals;
      * 
      * auto heartbeat = looper->addPeriodicTimer([]() {
      *     sendHeartbeat();
      * }, 30s);  // Every 30 seconds
      * @endcode
      * 
      * @note Implementation in SLLooper.tpp
      * @see \ref swt::Timer "Timer"
      */
     template<typename Rep, typename Period>
     Timer addPeriodicTimer(std::function<void()> callback,
                           const std::chrono::duration<Rep, Period>& interval);
 
     // ========== CONVENIENCE METHODS ==========
 
     /**
      * @brief Post function with timeout, returns Timer for cancellation
      * @tparam Function Function type (auto-deduced)
      * @param func Function to execute after timeout
      * @param timeout_ms Timeout in milliseconds
      * @return Timer Timer object for cancellation
      * 
      * Convenience method that combines postDelayed with timer functionality.
      * Only enabled for void-returning functions using SFINAE.
      * 
      * @code{.cpp}
      * Timer timeoutTimer = looper->postWithTimeout([]() {
      *     std::cout << "Timeout occurred!" << std::endl;
      * }, 5000);
      * 
      * // Can cancel before timeout
      * if (conditionMet) {
      *     timeoutTimer.cancel();
      * }
      * @endcode
      * 
      * @note Only works with void-returning functions
      * @note Implementation in SLLooper.tpp
      * @see \ref swt::Timer "Timer"
      */
     template<typename Function>
     auto postWithTimeout(Function&& func, uint64_t timeout_ms) 
         -> std::enable_if_t<std::is_void_v<std::invoke_result_t<Function>>, Timer>;
         
     // ========== INTERNAL TIMER API ==========
 
     /**
      * @brief Internal timer creation method
      * @param callback Function to call when timer expires
      * @param delay_ms Delay in milliseconds
      * @param periodic true for repeating timer, false for one-shot
      * @param cancelled Pointer to atomic cancellation flag
      * @return TimerId Unique timer identifier
      * 
      * Low-level timer creation used internally by Timer objects.
      * Creates TimerManager lazily if not already initialized.
      * 
      * @note For internal use by Timer class
      * @see \ref swt::TimerManager "TimerManager"
      */
     TimerId createTimerInternal(std::function<void()> callback, uint64_t delay_ms, 
                                bool periodic, std::atomic<bool>* cancelled);
 
     /**
      * @brief Internal timer cancellation method
      * @param id Timer identifier to cancel
      * @return true if timer was found and cancelled
      * 
      * @note For internal use by Timer class
      * @see \ref swt::TimerManager "TimerManager"
      */
     bool cancelTimerInternal(TimerId id);
 
     /**
      * @brief Check if timer exists in internal management
      * @param id Timer identifier to check
      * @return true if timer exists and is active
      * 
      * @note For internal use by Timer class
      * @see \ref swt::TimerManager "TimerManager"
      */
     bool hasTimerInternal(TimerId id);
 
     /**
      * @brief Restart existing timer with new delay
      * @param id Timer identifier to restart
      * @param delay_ms New delay in milliseconds
      * @return true if timer was found and restarted
      * 
      * @note For internal use by Timer class
      * @see \ref swt::TimerManager "TimerManager"
      */
     bool restartTimerInternal(TimerId id, uint64_t delay_ms);
 
     /**
      * @brief Get count of active timers
      * @return Number of currently active timers
      * 
      * Useful for debugging and monitoring timer usage.
      * @see \ref swt::TimerManager "TimerManager"
      */
     size_t getActiveTimerCount();
 
     /**
      * @brief Initialize TimerManager lazily
      * 
      * Creates TimerManager instance on first timer request.
      * Uses lazy initialization to avoid unnecessary resources.
      * 
      * @note Called automatically when first timer is created
      * @see \ref swt::TimerManager "TimerManager"
      */
     void initializeTimerManager();
 
     /**
      * @brief Update timer cancellation pointer for moved Timer objects
      * @param id Timer identifier
      * @param newPtr New pointer to atomic cancellation flag
      * 
      * Called when Timer objects are moved to update internal
      * pointer references for proper cancellation handling.
      * 
      * @note For internal use by Timer move operations
      * @see \ref swt::Timer "Timer"
      */
     void updateTimerCancelledPtr(TimerId id, std::atomic<bool>* newPtr);
 
 private:
     std::shared_ptr<EventQueue> mEventQueue;     /**< Message queue for async task processing */
     std::atomic<bool> mStarted;                      /**< Event loop started flag */
     std::thread t1;                                  /**< Main event loop thread */
     std::unique_ptr<TimerManager> mTimerManager;     /**< Timer management (lazy initialization) */
 };
 } // namespace swt
 
 // Include template implementations
 #include "SLLooper.tpp"