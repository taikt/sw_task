/**
 * @file Awaitable.h
 * @brief Direct awaitable types for coroutine integration with SLLooper
 * @author Tran Anh Tai
 * @date 10/2025
 * @version 1.0.0
 * 
 * This file provides coroutine awaitable types that integrate with SLLooper
 * for asynchronous execution. Includes WorkAwaitable for background work,
 * PostAwaitable for main thread execution, and DelayAwaitable for timing operations.
 */

 #pragma once

 #include <coroutine>
 #include <exception>
 #include <optional>
 #include <memory>
 #include <functional>
 
 namespace swt {
 
 // Forward declaration
 class SLLooper;
 
 /**
  * @class WorkAwaitable
  * @brief Awaitable for executing work on background thread
  * @tparam T Return type of the background work function
  * 
  * WorkAwaitable executes a function on a background thread and suspends
  * the coroutine until the work completes. The result is then returned
  * to the coroutine when it resumes.
  * 
  * @code{.cpp}
  * auto result = co_await looper->awaitWork([]() -> int {
  *     // Heavy computation on background thread
  *     return 42;
  * });
  * @endcode
  */
 template<typename T>
 class WorkAwaitable {
 private:
     std::shared_ptr<SLLooper> mLooper;        ///< Event loop for execution
     std::function<T()> mFunc;                 ///< Function to execute
     mutable std::optional<T> mResult;         ///< Result storage
     mutable std::exception_ptr mException;    ///< Exception storage
     mutable bool mReady = false;              ///< Completion flag
 
 public:
     /**
      * @brief Construct WorkAwaitable with function
      * @tparam Func Function type (auto-deduced)
      * @param looper Shared pointer to SLLooper
      * @param func Function to execute on background thread
      */
     template<typename Func>
     WorkAwaitable(std::shared_ptr<SLLooper> looper, Func&& func)
         : mLooper(looper), mFunc(std::forward<Func>(func)) {}
 
     /**
      * @brief Check if result is ready (coroutine interface)
      * @return false - always suspend initially
      */
     bool await_ready() const noexcept { return mReady; }
     
     /**
      * @brief Suspend coroutine and execute work on background thread
      * @param handle Coroutine handle for resumption
      */
     void await_suspend(std::coroutine_handle<> handle) const noexcept;
 
     /**
      * @brief Resume coroutine and return result
      * @return Result from background work
      * @throws std::exception if work failed
      */
     auto await_resume() const -> T {
         if (mException) std::rethrow_exception(mException);
         if (mResult.has_value()) return std::move(mResult.value());
         throw std::runtime_error("Work not completed");
     }
 
     /**
      * @brief Get the SLLooper instance
      * @return Shared pointer to looper
      */
     std::shared_ptr<SLLooper> getLooper() const { return mLooper; }
     
     /**
      * @brief Get the work function
      * @return Function to execute
      */
     std::function<T()> getFunc() const { return mFunc; }
     
     /**
      * @brief Set successful result (internal use)
      * @param result Result value from work function
      */
     void setResult(T result) const { 
         mResult = result;
         mReady = true; 
     }
     
     /**
      * @brief Set exception result (internal use)
      * @param ex Exception pointer from work function
      */
     void setException(std::exception_ptr ex) const { 
         mException = ex; 
         mReady = true; 
     }
 };
 
 /**
  * @class WorkAwaitable<void>
  * @brief Specialization for void return type
  * 
  * Specialized version of WorkAwaitable for functions that return void.
  * Avoids std::optional<void> compilation issues.
  * 
  * @code{.cpp}
  * co_await looper->awaitWork([]() -> void {
  *     // Side-effect work on background thread
  *     std::cout << "Background work completed" << std::endl;
  * });
  * @endcode
  */
 template<>
 class WorkAwaitable<void> {
 private:
     std::shared_ptr<SLLooper> mLooper;        ///< Event loop for execution
     std::function<void()> mFunc;              ///< Function to execute
     mutable std::exception_ptr mException;    ///< Exception storage
     mutable bool mReady = false;              ///< Completion flag
 
 public:
     /**
      * @brief Construct WorkAwaitable with void function
      * @tparam Func Function type (auto-deduced)
      * @param looper Shared pointer to SLLooper
      * @param func Void function to execute on background thread
      */
     template<typename Func>
     WorkAwaitable(std::shared_ptr<SLLooper> looper, Func&& func)
         : mLooper(looper), mFunc(std::forward<Func>(func)) {}
 
     /**
      * @brief Check if work is ready (coroutine interface)
      * @return false - always suspend initially
      */
     bool await_ready() const noexcept { return mReady; }
     
     /**
      * @brief Suspend coroutine and execute work on background thread
      * @param handle Coroutine handle for resumption
      */
     void await_suspend(std::coroutine_handle<> handle) const noexcept;
 
     /**
      * @brief Resume coroutine after void work completion
      * @throws std::exception if work failed
      */
     void await_resume() const {
         if (mException) std::rethrow_exception(mException);
         if (!mReady) throw std::runtime_error("Work not completed");
     }
 
     /**
      * @brief Get the SLLooper instance
      * @return Shared pointer to looper
      */
     std::shared_ptr<SLLooper> getLooper() const { return mLooper; }
     
     /**
      * @brief Get the work function
      * @return Void function to execute
      */
     std::function<void()> getFunc() const { return mFunc; }
     
     /**
      * @brief Mark work as completed (internal use)
      */
     void setResult() const { mReady = true; }
     
     /**
      * @brief Set exception result (internal use)
      * @param ex Exception pointer from work function
      */
     void setException(std::exception_ptr ex) const { 
         mException = ex; 
         mReady = true; 
     }
 };
 
 /**
  * @class DelayAwaitable  
  * @brief Awaitable for delay operations
  * 
  * Provides coroutine-based delay functionality using SLLooper's timer system.
  * Suspends the coroutine for a specified duration.
  * 
  * @code{.cpp}
  * co_await looper->awaitDelay(1000); // Wait 1 second
  * @endcode
  */
 class DelayAwaitable {
 private:
     std::shared_ptr<SLLooper> mLooper;        ///< Event loop for timer
     int mDelayMs;                             ///< Delay duration in milliseconds
     mutable bool mReady = false;              ///< Timer completion flag
 
 public:
     /**
      * @brief Construct DelayAwaitable with delay duration
      * @param looper Shared pointer to SLLooper
      * @param delayMs Delay duration in milliseconds
      */
     DelayAwaitable(std::shared_ptr<SLLooper> looper, int delayMs)
         : mLooper(looper), mDelayMs(delayMs) {}
 
     /**
      * @brief Check if delay is ready (coroutine interface)
      * @return false - always suspend initially
      */
     bool await_ready() const noexcept { return mReady; }
     
     /**
      * @brief Suspend coroutine and start timer
      * @param handle Coroutine handle for resumption
      */
     void await_suspend(std::coroutine_handle<> handle) const noexcept;
     
     /**
      * @brief Resume coroutine after delay (no return value)
      */
     void await_resume() const noexcept {}
 
     /**
      * @brief Get delay duration
      * @return Delay in milliseconds
      */
     int getDelayMs() const { return mDelayMs; }
     
     /**
      * @brief Get the SLLooper instance
      * @return Shared pointer to looper
      */
     std::shared_ptr<SLLooper> getLooper() const { return mLooper; }
     
     /**
      * @brief Mark delay as completed (internal use)
      */
     void setReady() const { mReady = true; }
 };
 
 /**
  * @class PostAwaitable
  * @brief Awaitable for executing function on main thread
  * @tparam T Return type of the main thread function
  * 
  * PostAwaitable executes a function on the main event loop thread and
  * suspends the coroutine until completion. Useful for UI updates or
  * thread-safe operations.
  * 
  * @code{.cpp}
  * auto result = co_await looper->awaitPost([]() -> std::string {
  *     // Execute on main thread
  *     return "Main thread result";
  * });
  * @endcode
  */
 template<typename T>
 class PostAwaitable {
 private:
     std::shared_ptr<SLLooper> mLooper;        ///< Event loop for execution
     std::function<T()> mFunc;                 ///< Function to execute
     mutable std::optional<T> mResult;         ///< Result storage
     mutable std::exception_ptr mException;    ///< Exception storage
     mutable bool mReady = false;              ///< Completion flag
 
 public:
     /**
      * @brief Construct PostAwaitable with function
      * @tparam Func Function type (auto-deduced)
      * @param looper Shared pointer to SLLooper
      * @param func Function to execute on main thread
      */
     template<typename Func>
     PostAwaitable(std::shared_ptr<SLLooper> looper, Func&& func)
         : mLooper(looper), mFunc(std::forward<Func>(func)) {}
 
     /**
      * @brief Check if result is ready (coroutine interface)
      * @return false - always suspend initially
      */
     bool await_ready() const noexcept { return mReady; }
     
     /**
      * @brief Suspend coroutine and post to main thread
      * @param handle Coroutine handle for resumption
      */
     void await_suspend(std::coroutine_handle<> handle) const noexcept;
 
     /**
      * @brief Resume coroutine and return result
      * @return Result from main thread function
      * @throws std::exception if function failed
      */
     auto await_resume() const -> T {
         if (mException) std::rethrow_exception(mException);
         if (mResult.has_value()) return std::move(mResult.value());
         throw std::runtime_error("Post not completed");
     }
 
     /**
      * @brief Get the main thread function
      * @return Function to execute
      */
     std::function<T()> getFunc() const { return mFunc; }
     
     /**
      * @brief Get the SLLooper instance
      * @return Shared pointer to looper
      */
     std::shared_ptr<SLLooper> getLooper() const { return mLooper; }
     
     /**
      * @brief Set successful result (internal use)
      * @param result Result value from main thread function
      */
     void setResult(T result) const { 
         mResult = result;
         mReady = true; 
     }
     
     /**
      * @brief Set exception result (internal use)
      * @param ex Exception pointer from main thread function
      */
     void setException(std::exception_ptr ex) const { 
         mException = ex; 
         mReady = true; 
     }
 };
 
 /**
  * @class PostAwaitable<void>
  * @brief Specialization for void return type
  * 
  * Specialized version of PostAwaitable for functions that return void.
  * Executes side-effect operations on the main thread.
  * 
  * @code{.cpp}
  * co_await looper->awaitPost([]() -> void {
  *     // UI update or thread-safe operation
  *     std::cout << "Main thread operation completed" << std::endl;
  * });
  * @endcode
  */
 template<>
 class PostAwaitable<void> {
 private:
     std::shared_ptr<SLLooper> mLooper;        ///< Event loop for execution
     std::function<void()> mFunc;              ///< Function to execute
     mutable std::exception_ptr mException;    ///< Exception storage
     mutable bool mReady = false;              ///< Completion flag
 
 public:
     /**
      * @brief Construct PostAwaitable with void function
      * @tparam Func Function type (auto-deduced)
      * @param looper Shared pointer to SLLooper
      * @param func Void function to execute on main thread
      */
     template<typename Func>
     PostAwaitable(std::shared_ptr<SLLooper> looper, Func&& func)
         : mLooper(looper), mFunc(std::forward<Func>(func)) {}
 
     /**
      * @brief Check if operation is ready (coroutine interface)
      * @return false - always suspend initially
      */
     bool await_ready() const noexcept { return mReady; }
     
     /**
      * @brief Suspend coroutine and post to main thread
      * @param handle Coroutine handle for resumption
      */
     void await_suspend(std::coroutine_handle<> handle) const noexcept;
 
     /**
      * @brief Resume coroutine after void operation completion
      * @throws std::exception if operation failed
      */
     void await_resume() const {
         if (mException) std::rethrow_exception(mException);
     }
 
     /**
      * @brief Get the main thread function
      * @return Void function to execute
      */
     std::function<void()> getFunc() const { return mFunc; }
     
     /**
      * @brief Get the SLLooper instance
      * @return Shared pointer to looper
      */
     std::shared_ptr<SLLooper> getLooper() const { return mLooper; }
     
     /**
      * @brief Mark operation as completed (internal use)
      */
     void setResult() const { mReady = true; }
     
     /**
      * @brief Set exception result (internal use)
      * @param ex Exception pointer from main thread function
      */
     void setException(std::exception_ptr ex) const { 
         mException = ex; 
         mReady = true; 
     }
 };
 
 } // namespace swt