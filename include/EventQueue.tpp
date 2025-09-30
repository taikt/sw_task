/**
 * @file EventQueue.tpp
 * @brief Template implementation for EventQueue - type-safe function enqueuing with futures
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once
 #include "EventQueue.h" 
  
 namespace swt {
 /**
  * @brief Enqueue function for immediate asynchronous execution
  * @tparam F Function type (auto-deduced)
  * @tparam Args Variadic argument types (auto-deduced)
  * @param func Callable object to execute asynchronously
  * @param args Arguments to forward to the function
  * @return std::future<ReturnType> Future containing the function result
  * 
  * Enqueues a function for immediate execution in the event loop thread.
  * Uses perfect forwarding to preserve argument types and std::packaged_task
  * to provide future-based result retrieval. The function is wrapped in a
  * void task for uniform queue handling while preserving the original
  * return type through the future mechanism.
  * 
  * Key implementation details:
  * - **Perfect forwarding**: Preserves value categories and reference types
  * - **Type erasure**: Wraps typed task in void task for uniform storage
  * - **Thread safety**: Protected by mutex during queue insertion
  * - **Immediate execution**: Inserted at front of queue (whenUs = 0)
  * - **Always notify**: Signals condition variable regardless of queue state
  * 
  * @code{.cpp}
  * // Enqueue lambda with capture
  * auto future1 = queue.enqueueFunction([x = 42](int y) { return x + y; }, 8);
  * 
  * // Enqueue free function
  * auto future2 = queue.enqueueFunction(std::sin, 3.14159);
  * 
  * // Get results
  * int result1 = future1.get();      // 50
  * double result2 = future2.get();   // ~0.0
  * @endcode
  * 
  * @note Thread-safe operation with automatic notification
  * @note Function executes in event loop thread context
  * @warning Future must be retrieved before function object destruction
  * 
  * @see \ref swt::EventQueue "EventQueue", \ref swt::Promise "Promise"
  */
 template<typename F, typename... Args>
 auto EventQueue::enqueueFunction(F&& func, Args&&... args) -> std::future<decltype(func(args...))> {
     using ReturnType = decltype(func(args...));
     
     // Bind arguments to function using perfect forwarding
     auto boundTask = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
     
     // Create packaged_task for typed result handling
     auto packagedTask = std::packaged_task<ReturnType()>(boundTask);
     auto future = packagedTask.get_future();
     
     // Wrap in void task for type-erased queue storage
     auto voidTask = std::packaged_task<void()>([task = std::move(packagedTask)]() mutable {
         task();
     });
     
     {
         std::lock_guard<std::mutex> lock(iMutex);
         int64_t whenUs = 0;  // IMMEDIATE EXECUTION - front of queue
         
         // Insert at beginning for immediate execution (highest priority)
         mQueue.emplace_front(std::move(voidTask), whenUs);
     }
     
     // ALWAYS notify condition variable to wake up event loop thread
     mQueueChanged.notify_one();
         
     return future;
 }
 
 /**
  * @brief Enqueue function for delayed asynchronous execution
  * @tparam F Function type (auto-deduced)
  * @tparam Args Variadic argument types (auto-deduced)
  * @param delayMs Delay in milliseconds before execution
  * @param func Callable object to execute asynchronously
  * @param args Arguments to forward to the function
  * @return std::future<ReturnType> Future containing the function result
  * 
  * Enqueues a function for delayed execution in the event loop thread.
  * The function will be executed after the specified delay, maintaining
  * proper ordering with other delayed tasks. Uses binary search insertion
  * to maintain chronological order in the queue without full sorting.
  * 
  * Key implementation details:
  * - **Timed execution**: Calculates absolute execution time from current uptime
  * - **Ordered insertion**: Uses std::upper_bound for efficient sorted insertion
  * - **Binary search**: O(log n) insertion instead of O(n log n) full sort
  * - **Conditional notification**: Only notifies if event loop is started
  * - **Microsecond precision**: Internal timing uses microseconds for accuracy
  * 
  * @code{.cpp}
  * // Execute function after 1 second delay
  * auto future = queue.enqueueFunctionDelayed(1000, []() { 
  *     return "Hello after delay!"; 
  * });
  * 
  * // Execute with arguments after 500ms delay
  * auto future2 = queue.enqueueFunctionDelayed(500, 
  *     [](int x, int y) { return x * y; }, 6, 7);
  * 
  * // Get results (will block until execution time)
  * std::string result1 = future.get();   // "Hello after delay!"
  * int result2 = future2.get();          // 42
  * @endcode
  * 
  * @note Thread-safe operation with efficient ordered insertion
  * @note Delay is measured from when function is enqueued
  * @note Only notifies condition variable if event loop is started
  * 
  * @see \ref swt::EventQueue "EventQueue", \ref swt::Promise "Promise"
  */
 template<typename F, typename... Args>
 auto EventQueue::enqueueFunctionDelayed(int64_t delayMs, F&& func, Args&&... args) -> std::future<decltype(func(args...))> {
     using ReturnType = decltype(func(args...));
     
     // Bind arguments to function using perfect forwarding
     auto boundTask = std::bind(std::forward<F>(func), std::forward<Args>(args)...);
     
     // Create packaged_task for typed result handling
     auto packagedTask = std::packaged_task<ReturnType()>(boundTask);
     auto future = packagedTask.get_future();
     
     // Wrap in void task for type-erased queue storage
     auto voidTask = std::packaged_task<void()>([task = std::move(packagedTask)]() mutable {
         task();
     });
     
     {
         std::lock_guard<std::mutex> lock(iMutex);
         
         // Calculate absolute execution time in microseconds
         int64_t whenUs = uptimeMicros() + (delayMs * 1000);
         
         // Insert at correct chronological position using binary search
         auto insertPos = std::upper_bound(mQueue.begin(), mQueue.end(), whenUs,
             [](int64_t time, const QueueItem& item) {
                 return time < item.whenUs;
             });
         
         // Insert at calculated position maintaining sorted order
         mQueue.emplace(insertPos, std::move(voidTask), whenUs);
     }
     
     // Only notify if event loop is started to avoid spurious wakeups
     if (mStarted) {
         mQueueChanged.notify_one();
     }
     
     return future;
 }
 
 /**
  * @brief Create and enqueue promise for manual resolution
  * @tparam T Value type for the promise
  * @return swt::Promise<T> New promise object for manual control
  * 
  * Creates a new promise that can be resolved manually from any thread.
  * The promise callbacks will execute in the event loop thread when
  * the promise is resolved or rejected.
  * 
  * @see \ref swt::Promise "Promise"
  */
 template<typename T>
 Promise<T> EventQueue::enqueuePromise() {
     return Promise<T>();
 }
 
 } // namespace swt