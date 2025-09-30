/**
 * @file SLLooper.tpp
 * @brief Template implementation for SLLooper - async task, timer, and promise management
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once
 #include "SLLooper.h"
 #include "EventQueue.h"
 #include "Promise.h"
 #include "CpuTaskExecutor.h"
 
 // ========== DEBUG MACROS FOR CPU-BOUND DETECTION ==========
 
 /**
  * @def CPU_BOUND_DETECT_THRESHOLD_MS
  * @brief Threshold (milliseconds) for warning about CPU-bound tasks in debug mode
  * 
  * Tasks that execute longer than this threshold will generate warnings
  * to help identify operations that should be moved to CpuTaskExecutor.
  * Only active in DEBUG builds for performance reasons.
  */
 #ifdef DEBUG
 #define CPU_BOUND_DETECT_THRESHOLD_MS 3000
 
 /**
  * @def CPU_BOUND_DETECT_BEGIN
  * @brief Start timing measurement for CPU-bound task detection
  * 
  * Records the start time for execution duration measurement.
  * No-op in release builds for zero runtime cost.
  */
 #define CPU_BOUND_DETECT_BEGIN auto __cpu_task_start = std::chrono::steady_clock::now();
 
 /**
  * @def CPU_BOUND_DETECT_END_WITH_INFO(file, line, funcname)
  * @brief End timing measurement and warn if threshold exceeded
  * @param file Source file name for diagnostic output
  * @param line Source line number for diagnostic output  
  * @param funcname Function name for diagnostic output
  * 
  * Calculates execution duration and prints warning if it exceeds
  * CPU_BOUND_DETECT_THRESHOLD_MS. Includes source location information
  * for easy identification of problematic code.
  */
 #define CPU_BOUND_DETECT_END_WITH_INFO(file, line, funcname) \
     auto __cpu_task_end = std::chrono::steady_clock::now(); \
     auto __cpu_task_dur = std::chrono::duration_cast<std::chrono::milliseconds>(__cpu_task_end - __cpu_task_start); \
     if (__cpu_task_dur > std::chrono::milliseconds(CPU_BOUND_DETECT_THRESHOLD_MS)) { \
         std::cerr << "[Warning] CPU-bound task detected: " << __cpu_task_dur.count() << " ms at " \
                   << (file ? file : "unknown") << ":" << line << " (" << (funcname ? funcname : "unknown") << ")\n"; \
     }
 #else
 #define CPU_BOUND_DETECT_BEGIN
 #define CPU_BOUND_DETECT_END_WITH_INFO(file, line, funcname)
 #endif
 
 // ========== INTERNAL DEBUG API IMPLEMENTATIONS ==========
 
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
  * Internal implementation that wraps the function with CPU-bound detection
  * logic in debug builds. Uses std::apply for perfect argument forwarding
  * and handles both void and value-returning functions with constexpr if.
  * 
  * Key implementation details:
  * - **Argument capture**: Uses std::make_tuple with perfect forwarding
  * - **Lazy evaluation**: Arguments evaluated in execution thread context
  * - **Debug integration**: Timing measurement with source location tracking
  * - **Type safety**: constexpr if for void vs value-returning functions
  * - **Zero cost**: Complete no-op in release builds
  * 
  * @note For internal use by SLLooper::post() macro expansion
  * @note Debug overhead only in DEBUG builds
  * @note Uses std::apply for tuple argument unpacking
  */
 template<typename F, typename... Args>
 auto SLLooper::post_internal(F&& func, Args&&... args, const char* file, int line, const char* funcname)
     -> std::future<decltype(func(args...))>
 {
     using ReturnType = decltype(func(args...));
     return mEventQueue->enqueueFunction(
         [func = std::forward<F>(func), args_tuple = std::make_tuple(std::forward<Args>(args)...), file, line, funcname]() mutable {
             CPU_BOUND_DETECT_BEGIN
             if constexpr (std::is_void_v<ReturnType>) {
                 // Handle void-returning function
                 std::apply(func, args_tuple);
                 CPU_BOUND_DETECT_END_WITH_INFO(file, line, funcname)
             } else {
                 // Handle value-returning function
                 auto result = std::apply(func, args_tuple);
                 CPU_BOUND_DETECT_END_WITH_INFO(file, line, funcname)
                 return result;
             }
         }
     );
 }
 
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
  * Identical to post_internal but uses enqueueFunctionDelayed for timing.
  * 
  * @note For internal use by SLLooper::postDelayed() macro expansion
  * @note Same debug and performance characteristics as post_internal
  */
 template<typename F, typename... Args>
 auto SLLooper::postDelayed_internal(int64_t delayMs, F&& func, Args&&... args, const char* file, int line, const char* funcname)
     -> std::future<decltype(func(args...))>
 {
     using ReturnType = decltype(func(args...));
     return mEventQueue->enqueueFunctionDelayed(
         delayMs,
         [func = std::forward<F>(func), args_tuple = std::make_tuple(std::forward<Args>(args)...), file, line, funcname]() mutable {
             CPU_BOUND_DETECT_BEGIN
             if constexpr (std::is_void_v<ReturnType>) {
                 // Handle void-returning function
                 std::apply(func, args_tuple);
                 CPU_BOUND_DETECT_END_WITH_INFO(file, line, funcname)
             } else {
                 // Handle value-returning function
                 auto result = std::apply(func, args_tuple);
                 CPU_BOUND_DETECT_END_WITH_INFO(file, line, funcname)
                 return result;
             }
         }
     );
 }
 
 // ========== PUBLIC ASYNC API IMPLEMENTATIONS ==========
 
 /**
  * @brief Post function for immediate asynchronous execution
  * @tparam F Function type (auto-deduced)
  * @tparam Args Variadic argument types (auto-deduced)
  * @param func Callable object to execute asynchronously
  * @param args Arguments to forward to the function
  * @return std::future<ReturnType> Future containing the function result
  * 
  * Public API for immediate function posting. Uses std::bind for argument
  * binding, which is simpler than tuple-based approach but may have slightly
  * different semantics for perfect forwarding.
  * 
  * Implementation choice rationale:
  * - **std::bind**: Simpler implementation, standard argument binding
  * - **Perfect forwarding**: Preserves argument value categories
  * - **Direct delegation**: Leverages EventQueue template implementation
  * - **No debug overhead**: Pure release mode implementation
  * 
  * @code{.cpp}
  * // Lambda with capture
  * auto future1 = looper->post([x = 42](int y) { return x + y; }, 8);
  * 
  * // Free function with arguments
  * auto future2 = looper->post(std::sin, 3.14159);
  * 
  * // Member function call
  * MyClass obj;
  * auto future3 = looper->post(&MyClass::method, &obj, arg1, arg2);
  * @endcode
  * 
  * @note Thread-safe operation
  * @note Arguments bound immediately, evaluated later
  * @note Direct EventQueue delegation for optimal performance
  */
 template<typename F, typename... Args>
 auto SLLooper::post(F&& func, Args&&... args) -> std::future<decltype(func(args...))> {
     return mEventQueue->enqueueFunction(
         std::bind(std::forward<F>(func), std::forward<Args>(args)...)
     );
 }
 
 /**
  * @brief Post function for delayed asynchronous execution
  * @tparam F Function type (auto-deduced)
  * @tparam Args Variadic argument types (auto-deduced)
  * @param delayMs Delay in milliseconds before execution
  * @param func Callable object to execute asynchronously
  * @param args Arguments to forward to the function
  * @return std::future<ReturnType> Future containing the function result
  * 
  * Public API for delayed function posting. Uses std::bind for argument
  * binding with the same characteristics as immediate post().
  * 
  * @code{.cpp}
  * // Execute lambda after 1 second
  * auto future = looper->postDelayed(1000, []() { 
  *     return "Hello after delay!"; 
  * });
  * 
  * // Delayed function with arguments
  * auto future2 = looper->postDelayed(500, calculateSum, a, b, c);
  * @endcode
  * 
  * @note Delay measured from posting time, not execution time
  * @note Arguments bound immediately for consistent behavior
  */
 template<typename F, typename... Args>
 auto SLLooper::postDelayed(int64_t delayMs, F&& func, Args&&... args) -> std::future<decltype(func(args...))> {
     return mEventQueue->enqueueFunctionDelayed(
         delayMs,
         std::bind(std::forward<F>(func), std::forward<Args>(args)...)
     );
 }
 
 // ========== PROMISE API IMPLEMENTATIONS ==========
 
 /**
  * @brief Create a new promise object for manual result setting
  * @tparam T Value type for the promise
  * @return kt::Promise<T> New promise object
  * 
  * Creates a new promise object that can be resolved manually from any thread.
  * The promise callbacks will execute in this SLLooper's thread context when
  * the promise is resolved or rejected.
  * 
  * Implementation note: Currently creates a simple Promise<T> object rather
  * than using EventQueue::enqueuePromise(). This provides more direct control
  * over promise lifecycle and callback execution context.
  * 
  * @code{.cpp}
  * auto promise = looper->createPromise<int>();
  * 
  * // Set up continuation
  * promise.then(looper, [](int result) {
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
  * @note Promise can be resolved from any thread
  * @note Callbacks execute in this looper's thread context
  * @note Each promise can only be resolved once
  */
 template<typename T>
 kt::Promise<T> SLLooper::createPromise() {
     // Direct Promise creation - callbacks will use this looper's context
     // when promise.then(shared_from_this(), callback) is called
     return kt::Promise<T>{};
 }
 
 // ========== CPU-BOUND TASK API IMPLEMENTATIONS ==========
 
 /**
  * @brief Execute CPU-intensive task asynchronously
  * @tparam Func Function type (auto-deduced)
  * @param func CPU-intensive function to execute
  * @return kt::Promise<ReturnType> Promise for result retrieval and chaining
  * 
  * Delegates CPU-bound task execution to CpuTaskExecutor, which handles
  * separate thread execution and result delivery back to this event loop.
  * This ensures the main event loop remains responsive during CPU-intensive
  * operations.
  * 
  * Key benefits:
  * - **Non-blocking**: Main event loop continues processing other tasks
  * - **Thread isolation**: CPU task executes in dedicated thread
  * - **Result integration**: Results delivered via Promise in event loop context
  * - **Exception safety**: All exceptions captured and propagated properly
  * 
  * @code{.cpp}
  * auto promise = looper->postWork([]() {
  *     // CPU-intensive computation runs in separate thread
  *     return fibonacci(45);
  * });
  * 
  * promise.then(shared_from_this(), [](long result) {
  *     // This callback runs in main event loop thread
  *     std::cout << "Computation result: " << result << std::endl;
  * }).catch_error(shared_from_this(), [](std::exception_ptr ex) {
  *     std::cerr << "Computation failed!" << std::endl;
  * });
  * @endcode
  * 
  * @note Uses shared_from_this() to ensure SLLooper lifetime during async operation
  * @note Function executes in separate thread - ensure thread safety
  * @note Preferred over regular post() for CPU-intensive operations
  */
 template<typename Func>
 auto SLLooper::postWork(Func&& func) -> kt::Promise<decltype(func())> {
     return kt::CpuTaskExecutor::executeAsync(shared_from_this(), std::forward<Func>(func));
 }
 
 /**
  * @brief Execute CPU-intensive task with timeout protection
  * @tparam Func Function type (auto-deduced)
  * @param func CPU-intensive function to execute
  * @param timeout Maximum execution time
  * @return kt::Promise<ReturnType> Promise for result retrieval
  * 
  * Delegates CPU-bound task execution with timeout protection to CpuTaskExecutor.
  * If the task doesn't complete within the timeout, the promise is rejected
  * with a CpuTaskTimeoutException.
  * 
  * @code{.cpp}
  * auto promise = looper->postWork([]() {
  *     return longRunningComputation();
  * }, 5000ms);  // 5 second timeout
  * 
  * promise.then(shared_from_this(), [](auto result) {
  *     std::cout << "Completed in time: " << result << std::endl;
  * }).catch_error(shared_from_this(), [](std::exception_ptr ex) {
  *     try {
  *         std::rethrow_exception(ex);
  *     } catch (const kt::CpuTaskTimeoutException& timeout) {
  *         std::cerr << "Task timed out!" << std::endl;
  *     }
  * });
  * @endcode
  * 
  * @note Timeout is enforced by CpuTaskExecutor's timeout mechanism
  * @note Timed-out tasks may continue running but results are discarded
  */
 template<typename Func>
 auto SLLooper::postWork(Func&& func, std::chrono::milliseconds timeout) 
     -> kt::Promise<decltype(func())> {
     return kt::CpuTaskExecutor::executeAsync(shared_from_this(), std::forward<Func>(func), timeout);
 }
 
 // ========== TIMER API IMPLEMENTATIONS (Chrono Support) ==========
 
 /**
  * @brief Add one-shot timer with chrono duration
  * @tparam Rep Duration representation type (e.g., int, long)
  * @tparam Period Duration period type (e.g., std::milli, std::micro)
  * @param callback Function to call when timer expires
  * @param delay Chrono duration (e.g., 500ms, 1s, 100us)
  * @return Timer RAII object for timer management
  * 
  * Template wrapper that converts chrono durations to milliseconds for
  * internal timer management. Supports any chrono duration type and
  * automatically converts to the internal millisecond representation.
  * 
  * Conversion details:
  * - **Duration casting**: Uses std::chrono::duration_cast for precision
  * - **Millisecond resolution**: Internal timer system uses millisecond granularity
  * - **Truncation behavior**: Sub-millisecond durations truncated to 0ms
  * - **Overflow protection**: Large durations may overflow - consider reasonable limits
  * 
  * @code{.cpp}
  * using namespace std::chrono_literals;
  * 
  * // Various duration types supported
  * auto timer1 = looper->addTimer(callback, 500ms);      // 500 milliseconds
  * auto timer2 = looper->addTimer(callback, 1s);         // 1 second  
  * auto timer3 = looper->addTimer(callback, 2min);       // 2 minutes
  * auto timer4 = looper->addTimer(callback, 1500us);     // 1.5ms (truncated to 1ms)
  * @endcode
  * 
  * @note Delegates to millisecond-based addTimer() after conversion
  * @note Sub-millisecond durations may be truncated
  * @note Timer precision limited by underlying system timer resolution
  */
 template<typename Rep, typename Period>
 Timer SLLooper::addTimer(std::function<void()> callback, 
                         const std::chrono::duration<Rep, Period>& delay) {
     auto delay_ms = std::chrono::duration_cast<std::chrono::milliseconds>(delay).count();
     return addTimer(std::move(callback), static_cast<uint64_t>(delay_ms));
 }
 
 /**
  * @brief Add periodic timer with chrono interval
  * @tparam Rep Duration representation type
  * @tparam Period Duration period type  
  * @param callback Function to call on each timer expiration
  * @param interval Chrono interval between expirations
  * @return Timer RAII object for timer management
  * 
  * Template wrapper for periodic timers with chrono duration support.
  * Creates a repeating timer that fires at the specified interval until
  * cancelled or the Timer object is destroyed.
  * 
  * @code{.cpp}
  * using namespace std::chrono_literals;
  * 
  * // Heartbeat every 30 seconds
  * auto heartbeat = looper->addPeriodicTimer([]() {
  *     sendHeartbeat();
  * }, 30s);
  * 
  * // Status check every 100 milliseconds  
  * auto statusCheck = looper->addPeriodicTimer([]() {
  *     checkSystemStatus();
  * }, 100ms);
  * @endcode
  * 
  * @note Delegates to millisecond-based addPeriodicTimer() after conversion
  * @note Same conversion characteristics as one-shot timer version
  * @note Timer continues until cancelled or Timer object destroyed
  */
 template<typename Rep, typename Period>
 Timer SLLooper::addPeriodicTimer(std::function<void()> callback,
                                 const std::chrono::duration<Rep, Period>& interval) {
     auto interval_ms = std::chrono::duration_cast<std::chrono::milliseconds>(interval).count();
     return addPeriodicTimer(std::move(callback), static_cast<uint64_t>(interval_ms));
 }
 
 // ========== CONVENIENCE METHOD IMPLEMENTATIONS ==========
 
 /**
  * @brief Post function with timeout, returns Timer for cancellation
  * @tparam Function Function type (auto-deduced)
  * @param func Function to execute after timeout
  * @param timeout_ms Timeout in milliseconds
  * @return Timer object for cancellation control
  * 
  * Convenience method that combines delayed execution with timer functionality.
  * Only enabled for void-returning functions using SFINAE template constraints.
  * Provides a Timer object for cancellation control, unlike postDelayed which
  * returns a future.
  * 
  * SFINAE constraint rationale:
  * - **Void-only**: Timer callbacks don't return values, so function must be void
  * - **Type safety**: Prevents accidental use with value-returning functions
  * - **API consistency**: Maintains timer callback signature expectations
  * 
  * Use cases:
  * - **Timeout operations**: Cancel pending operations after delay
  * - **Periodic tasks**: Combined with periodic timer for complex patterns
  * - **UI timeouts**: Auto-hide notifications, tooltips, etc.
  * - **Network timeouts**: Cleanup connections after inactivity
  * 
  * @code{.cpp}
  * // Auto-hide notification after 5 seconds
  * Timer hideTimer = looper->postWithTimeout([]() {
  *     hideNotification();
  * }, 5000);
  * 
  * // User interaction cancels the auto-hide
  * if (userInteracted) {
  *     hideTimer.cancel();  // Notification stays visible
  * }
  * 
  * // Timeout for network operation
  * Timer networkTimeout = looper->postWithTimeout([]() {
  *     cancelNetworkOperation();
  *     showTimeoutError();
  * }, 10000);
  * 
  * // Success response cancels timeout
  * onNetworkSuccess([&networkTimeout]() {
  *     networkTimeout.cancel();
  * });
  * @endcode
  * 
  * @note SFINAE restricts to void-returning functions only
  * @note Returns Timer for cancellation, not future for result
  * @note Function captured by value for safe timer execution
  * @warning Function must not throw - timer callbacks should be exception-safe
  */
 template<typename Function>
 auto SLLooper::postWithTimeout(Function&& func, uint64_t timeout_ms) 
     -> std::enable_if_t<std::is_void_v<std::invoke_result_t<Function>>, Timer> {
     
     return addTimer([func = std::forward<Function>(func)]() {
         func();
     }, timeout_ms);
 }