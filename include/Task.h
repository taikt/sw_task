/**
 * @file Task.h
 * @brief Coroutine-based Task implementation for asynchronous execution
 * @author Tran Anh Tai
 * @date 10/2025
 * @version 1.0.0
 */

 #pragma once

 #include <coroutine>
 #include <exception>
 #include <memory>
 #include <optional>
 #include <vector>
 #include <mutex>
 #include <atomic>
 #include <utility>
 
 namespace swt {
 
 // Forward declaration
 template<typename T>
 class Task;
 
 /**
  * @class TaskPromise
  * @brief Promise type for coroutine-based Task implementation
  * 
  * TaskPromise manages the coroutine state and provides continuation chaining
  * for proper asynchronous execution. It handles result storage, exception
  * propagation, and thread-safe continuation management.
  * 
  * @tparam T The return type of the coroutine
  * 
  * Key features:
  * - **Thread-safe completion**: Atomic flags and proper memory ordering
  * - **Continuation chaining**: Multiple coroutines can await the same task
  * - **Exception safety**: Proper exception propagation and resource cleanup
  * - **Memory efficient**: Minimal overhead with optimized storage
  * 
  * @note This class is not intended for direct use - it's managed by Task<T>
  */
 template<typename T>
 class TaskPromise {
 public:
     using Handle = std::coroutine_handle<TaskPromise<T>>;
 
     /**
      * @brief Create the Task object for this promise
      * @return Task<T> object associated with this promise
      */
     Task<T> get_return_object();
 
     /**
      * @brief Initial suspend behavior - always suspend to allow manual start
      * @return std::suspend_always to require manual task.start() call
      */
     std::suspend_always initial_suspend() noexcept {
         return {};
     }
 
     /**
      * @brief Final suspend behavior with continuation chaining
      * @return Custom awaiter that resumes all waiting continuations
      */
     auto final_suspend() noexcept {
         struct FinalAwaiter {
             TaskPromise* promise;
             
             bool await_ready() noexcept { return false; }
             
             /**
              * @brief Resume all waiting continuations when task completes
              * @param h Handle to the completing coroutine
              * @return noop_coroutine to prevent current coroutine resumption
              */
             std::coroutine_handle<> await_suspend(std::coroutine_handle<TaskPromise<T>> h) noexcept {
                 std::lock_guard<std::mutex> lock(promise->mContinuationMutex);
                 
                 // Resume all waiting continuations
                 for (auto& continuation : promise->mContinuations) {
                     continuation.resume();
                 }
                 promise->mContinuations.clear();
                 
                 return std::noop_coroutine();
             }
             
             void await_resume() noexcept {}
         };
         
         return FinalAwaiter{this};
     }
 
     /**
      * @brief Store the coroutine return value
      * @param value The value returned by co_return
      */
     void return_value(T value) {
         mResult = std::move(value);
         mResultReady.store(true, std::memory_order_release);
     }
 
     /**
      * @brief Handle unhandled exceptions in the coroutine
      */
     void unhandled_exception() {
         mException = std::current_exception();
         mResultReady.store(true, std::memory_order_release);
     }
 
     /**
      * @brief Get the stored result value
      * @return The result value
      * @throws std::exception_ptr if coroutine threw an exception
      * @throws std::runtime_error if task not completed
      */
     T getResult() {
         if (mException) {
             std::rethrow_exception(mException);
         }
         if (mResult.has_value()) {
             return mResult.value(); // Don't move, might be called multiple times
         }
         throw std::runtime_error("Task not completed");
     }
 
     /**
      * @brief Check if the task has completed
      * @return true if task completed (successfully or with exception)
      */
     bool isReady() const noexcept {
         return mResultReady.load(std::memory_order_acquire);
     }
 
     /**
      * @brief Add a continuation to be resumed when task completes
      * @param continuation Coroutine handle to resume
      * 
      * If the task is already completed, the continuation is resumed immediately.
      * Otherwise, it's added to the continuation list for later resumption.
      */
     void addContinuation(std::coroutine_handle<> continuation) {
         std::lock_guard<std::mutex> lock(mContinuationMutex);
         
         // If already completed, resume immediately
         if (mResultReady.load(std::memory_order_acquire)) {
             continuation.resume();
         } else {
             mContinuations.push_back(continuation);
         }
     }
 
 private:
     std::optional<T> mResult;                           ///< Stored result value
     std::exception_ptr mException;                      ///< Stored exception if any
     std::atomic<bool> mResultReady{false};             ///< Atomic completion flag
     
     std::mutex mContinuationMutex;                      ///< Mutex for continuation access
     std::vector<std::coroutine_handle<>> mContinuations; ///< Waiting continuations
 };
 
 /**
  * @class TaskPromise<void>
  * @brief Specialized promise for void-returning coroutines
  * 
  * This specialization handles coroutines that don't return a value,
  * using co_return; or implicit return at the end of the coroutine.
  */
 template<>
 class TaskPromise<void> {
 public:
     using Handle = std::coroutine_handle<TaskPromise<void>>;
 
     /**
      * @brief Create the Task<void> object for this promise
      * @return Task<void> object associated with this promise
      */
     Task<void> get_return_object();
 
     /**
      * @brief Initial suspend behavior - always suspend to allow manual start
      * @return std::suspend_always to require manual task.start() call
      */
     std::suspend_always initial_suspend() noexcept {
         return {};
     }
 
     /**
      * @brief Final suspend behavior with continuation chaining
      * @return Custom awaiter that resumes all waiting continuations
      */
     auto final_suspend() noexcept {
         struct FinalAwaiter {
             TaskPromise* promise;
             
             bool await_ready() noexcept { return false; }
             
             std::coroutine_handle<> await_suspend(std::coroutine_handle<TaskPromise<void>> h) noexcept {
                 std::lock_guard<std::mutex> lock(promise->mContinuationMutex);
                 
                 for (auto& continuation : promise->mContinuations) {
                     continuation.resume();
                 }
                 promise->mContinuations.clear();
                 
                 return std::noop_coroutine();
             }
             
             void await_resume() noexcept {}
         };
         
         return FinalAwaiter{this};
     }
 
     /**
      * @brief Handle void return from coroutine
      */
     void return_void() {
         mResultReady.store(true, std::memory_order_release);
     }
 
     /**
      * @brief Handle unhandled exceptions in the coroutine
      */
     void unhandled_exception() {
         mException = std::current_exception();
         mResultReady.store(true, std::memory_order_release);
     }
 
     /**
      * @brief Check for exceptions (void tasks have no return value)
      * @throws std::exception_ptr if coroutine threw an exception
      */
     void getResult() {
         if (mException) {
             std::rethrow_exception(mException);
         }
     }
 
     /**
      * @brief Check if the task has completed
      * @return true if task completed (successfully or with exception)
      */
     bool isReady() const noexcept {
         return mResultReady.load(std::memory_order_acquire);
     }
 
     /**
      * @brief Add a continuation to be resumed when task completes
      * @param continuation Coroutine handle to resume
      */
     void addContinuation(std::coroutine_handle<> continuation) {
         std::lock_guard<std::mutex> lock(mContinuationMutex);
         
         if (mResultReady.load(std::memory_order_acquire)) {
             continuation.resume();
         } else {
             mContinuations.push_back(continuation);
         }
     }
 
 private:
     std::exception_ptr mException;                      ///< Stored exception if any
     std::atomic<bool> mResultReady{false};             ///< Atomic completion flag
     std::mutex mContinuationMutex;                      ///< Mutex for continuation access
     std::vector<std::coroutine_handle<>> mContinuations; ///< Waiting continuations
 };
 
 /**
  * @class Task
  * @brief RAII wrapper for coroutines with proper continuation chaining
  * 
  * Task<T> provides a type-safe, move-only wrapper around C++20 coroutines
  * with support for proper asynchronous execution and continuation chaining.
  * 
  * @tparam T The return type of the coroutine
  * 
  * Key features:
  * - **RAII management**: Automatic coroutine handle cleanup
  * - **Move-only semantics**: Prevents accidental copying
  * - **Awaitable interface**: Can be co_awaited by other coroutines
  * - **Thread-safe**: Safe to await from multiple threads
  * - **Exception propagation**: Proper exception handling across await boundaries
  * 
  * @code{.cpp}
  * Task<int> computeAsync() {
  *     co_await std::suspend_always{};
  *     co_return 42;
  * }
  * 
  * Task<void> example() {
  *     int result = co_await computeAsync();
  *     std::cout << "Result: " << result << std::endl;
  * }
  * @endcode
  * 
  * @note Tasks must be started manually with start() and are lazy by default
  * @warning Task objects must not be copied - move-only for resource safety
  */
 template<typename T>
 class Task {
 public:
     using promise_type = TaskPromise<T>;
     using Handle = typename promise_type::Handle;
 
     /**
      * @brief Construct task with coroutine handle
      * @param handle Coroutine handle from promise.get_return_object()
      */
     explicit Task(Handle handle) : mHandle(handle) {}
 
     /**
      * @brief Destructor - automatically destroys coroutine handle
      */
     ~Task() {
         if (mHandle) {
             mHandle.destroy();
         }
     }
 
     // Move-only semantics
     Task(const Task&) = delete;
     Task& operator=(const Task&) = delete;
 
     /**
      * @brief Move constructor
      * @param other Task to move from
      */
     Task(Task&& other) noexcept : mHandle(std::exchange(other.mHandle, {})) {}
 
     /**
      * @brief Move assignment operator
      * @param other Task to move from
      * @return Reference to this task
      */
     Task& operator=(Task&& other) noexcept {
         if (this != &other) {
             if (mHandle) {
                 mHandle.destroy();
             }
             mHandle = std::exchange(other.mHandle, {});
         }
         return *this;
     }
 
     /**
      * @brief Start the coroutine execution
      * 
      * Tasks are lazy by default and must be explicitly started.
      * This resumes the coroutine from its initial suspend point.
      */
     void start() {
         if (mHandle && !mHandle.done()) {
             mHandle.resume();
         }
     }
 
     /**
      * @brief Check if the task has completed
      * @return true if coroutine has finished execution
      */
     bool done() const noexcept {
         return mHandle && mHandle.done();
     }
 
     /**
      * @brief Get the result value (non-blocking)
      * @return The result value
      * @throws std::runtime_error if task not completed or invalid handle
      * @throws std::exception_ptr if coroutine threw an exception
      * 
      * This method does not block. Use co_await or check done() first.
      */
     auto getResult() -> T requires (!std::is_void_v<T>) {
         if (!mHandle) {
             throw std::runtime_error("Invalid task handle");
         }
         
         if (!mHandle.promise().isReady()) {
             throw std::runtime_error("Task not completed - use co_await or check done() first");
         }
         
         return mHandle.promise().getResult();
     }
 
     /**
      * @brief Get the result for void tasks (non-blocking)
      * @throws std::runtime_error if task not completed or invalid handle
      * @throws std::exception_ptr if coroutine threw an exception
      */
     void getResult() requires std::is_void_v<T> {
         if (!mHandle) {
             throw std::runtime_error("Invalid task handle");
         }
         
         if (!mHandle.promise().isReady()) {
             throw std::runtime_error("Task not completed - use co_await or check done() first");
         }
         
         mHandle.promise().getResult();
     }
 
     /**
      * @brief Check if task is ready (awaitable interface)
      * @return true if task completed, false if still running
      */
     bool await_ready() const noexcept {
         return done();
     }
 
     /**
      * @brief Suspend awaiting coroutine until this task completes
      * @param waiter Handle to the coroutine that's awaiting this task
      * 
      * This method implements proper continuation chaining. The waiting
      * coroutine will be resumed when this task completes.
      */
     void await_suspend(std::coroutine_handle<> waiter) noexcept {
         if (!mHandle) {
             // Invalid handle, resume immediately
             waiter.resume();
             return;
         }
 
         // Add waiter as continuation
         mHandle.promise().addContinuation(waiter);
         
         // Start this task if not already started
         if (!mHandle.done()) {
             mHandle.resume();
         }
     }
 
     /**
      * @brief Get result when co_await completes (non-void version)
      * @return The result value
      * @throws std::exception_ptr if coroutine threw an exception
      */
     auto await_resume() -> T requires (!std::is_void_v<T>) {
         return getResult();
     }
 
     /**
      * @brief Complete co_await for void tasks
      * @throws std::exception_ptr if coroutine threw an exception
      */
     void await_resume() requires std::is_void_v<T> {
         getResult();
     }
 
     /**
      * @brief Check if task is ready without blocking
      * @return true if task completed, false if still running
      */
     bool isReady() const noexcept {
         return mHandle && mHandle.promise().isReady();
     }
 
 private:
     Handle mHandle; ///< Coroutine handle for this task
 };
 
 // Implementations for get_return_object
 template<typename T>
 inline Task<T> TaskPromise<T>::get_return_object() {
     return Task<T>{Handle::from_promise(*this)};
 }
 
 inline Task<void> TaskPromise<void>::get_return_object() {
     return Task<void>{Handle::from_promise(*this)};
 }
 
 } // namespace swt