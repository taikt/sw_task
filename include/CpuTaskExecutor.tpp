/**
 * @file CpuTaskExecutor.tpp
 * @brief Template implementation for CpuTaskExecutor - async CPU-bound task execution with timeout support
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once
 #include "Promise.h"
 #include "SLLooper.h"
 #include "CpuTaskExecutor.h"
 #include <thread>
 #include <future>
 #include <string>
 
 namespace swt {
 
 /**
  * @brief Execute CPU-bound task asynchronously without timeout
  * @tparam Func Function type (auto-deduced)
  * @param resultLooper SLLooper instance for result callback execution
  * @param func CPU-intensive function to execute in separate thread
  * @return Promise<ReturnType> Promise for result retrieval and continuation chaining
  * 
  * Executes CPU-intensive tasks in a separate thread to avoid blocking the main
  * event loop. The function is executed asynchronously using std::async with
  * std::launch::async policy to guarantee separate thread execution. Results
  * are delivered back to the specified event loop thread for thread-safe
  * callback execution.
  * 
  * Key implementation details:
  * - **Separate thread**: Uses std::async with std::launch::async for guaranteed thread
  * - **Thread-safe results**: Results posted back to specified SLLooper thread
  * - **Exception safety**: Automatic exception capture and propagation
  * - **Void support**: Template specialization handles void-returning functions
  * - **Move semantics**: Efficient parameter and result transfer
  * - **Fire-and-forget**: std::future is stored but not waited on (Promise coordination)
  * 
  * Execution flow:
  * 1. Create promise for result coordination
  * 2. Launch async task in separate thread
  * 3. Execute function and capture result/exception
  * 4. Post result back to event loop thread via SLLooper
  * 5. Resolve promise in event loop thread context
  * 
  * @code{.cpp}
  * auto promise = CpuTaskExecutor::executeAsync(looper, []() {
  *     // CPU-intensive computation
  *     return fibonacci(45);  // This runs in separate thread
  * });
  * 
  * promise.then(looper, [](long result) {
  *     // This callback runs in event loop thread
  *     std::cout << "Fibonacci result: " << result << std::endl;
  * }).catch_error(looper, [](std::exception_ptr ex) {
  *     std::cerr << "Computation failed!" << std::endl;
  * });
  * @endcode
  * 
  * @note Function executes in separate thread - ensure thread safety
  * @note Result callbacks execute in specified SLLooper thread context
  * @note std::future is intentionally not waited on - Promise provides coordination
  * @warning Avoid capturing SLLooper or other event loop objects in func
  */
 template<typename Func>
 auto CpuTaskExecutor::executeAsync(std::shared_ptr<SLLooper> resultLooper,
                                  Func&& func) 
     -> Promise<decltype(func())> {
     
     using ReturnType = decltype(func());
     auto promise = resultLooper->createPromise<ReturnType>();
     
     // Launch async task - std::future stored to prevent immediate destruction
     // but not waited on since Promise provides coordination mechanism
     auto asyncFuture = std::async(std::launch::async, [promise, func = std::forward<Func>(func), resultLooper]() mutable {
         try {
             if constexpr (std::is_void_v<ReturnType>) {
                 // Handle void-returning function
                 func();
                 // Post success result back to event loop thread
                 resultLooper->post([promise]() mutable {
                     promise.set_value();
                 });
             } else {
                 // Handle value-returning function
                 auto result = func();
                 // Post result back to event loop thread with move semantics
                 resultLooper->post([promise, result = std::move(result)]() mutable {
                     promise.set_value(std::move(result));
                 });
             }
         } catch (...) {
             // Capture any exception thrown during execution
             auto exception = std::current_exception();
             // Post exception back to event loop thread
             resultLooper->post([promise, exception]() mutable {
                 promise.set_exception(exception);
             });
         }
     });
     
     // Suppress unused variable warning - future is intentionally not waited on
     // Promise mechanism provides the coordination, not std::future
     (void)asyncFuture;
     
     return promise;
 }
 
 /**
  * @brief Execute CPU-bound task asynchronously with timeout protection
  * @tparam Func Function type (auto-deduced)
  * @param resultLooper SLLooper instance for result callback execution
  * @param func CPU-intensive function to execute in separate thread
  * @param timeout Maximum execution time before task is considered failed
  * @return Promise<ReturnType> Promise for result retrieval and continuation chaining
  * 
  * Executes CPU-intensive tasks with timeout protection to prevent runaway
  * computations from hanging the application. Uses a two-level async approach:
  * an outer async task manages timeout detection, while an inner async task
  * executes the actual function. If the function doesn't complete within the
  * timeout, a CpuTaskTimeoutException is generated.
  * 
  * Key implementation Details:
  * - **Timeout protection**: std::future::wait_for with timeout detection
  * - **Two-level async**: Outer timeout management, inner task execution
  * - **Exception types**: CpuTaskTimeoutException for timeout vs function exceptions
  * - **Resource cleanup**: Timeout tasks are abandoned but system handles cleanup
  * - **Thread safety**: Same result posting mechanism as non-timeout version
  * 
  * Timeout behavior:
  * - **Within timeout**: Normal result/exception delivery via Promise
  * - **Timeout exceeded**: CpuTaskTimeoutException delivered via Promise
  * - **Background cleanup**: System handles cleanup of abandoned timeout tasks
  * 
  * @code{.cpp}
  * auto promise = CpuTaskExecutor::executeAsync(looper, []() {
  *     return longRunningComputation();  // May take a long time
  * }, 5000ms);  // 5 second timeout
  * 
  * promise.then(looper, [](auto result) {
  *     std::cout << "Completed in time: " << result << std::endl;
  * }).catch_error(looper, [](std::exception_ptr ex) {
  *     try {
  *         std::rethrow_exception(ex);
  *     } catch (const CpuTaskTimeoutException& timeout) {
  *         std::cerr << "Task timed out: " << timeout.what() << std::endl;
  *     } catch (const std::exception& other) {
  *         std::cerr << "Task failed: " << other.what() << std::endl;
  *     }
  * });
  * @endcode
  * 
  * @note Timeout detection uses std::future::wait_for for precision
  * @note Timed-out tasks continue running but results are discarded
  * @note CpuTaskTimeoutException provides timeout duration information
  * @warning Long-running tasks may continue executing after timeout
  */
 template<typename Func>
 auto CpuTaskExecutor::executeAsync(std::shared_ptr<SLLooper> resultLooper,
                                  Func&& func,
                                  std::chrono::milliseconds timeout) 
     -> Promise<decltype(func())> {
     
     using ReturnType = decltype(func());
     auto promise = resultLooper->createPromise<ReturnType>();
     
     // Launch timeout management task
     auto asyncFuture = std::async(std::launch::async, [promise, func = std::forward<Func>(func), resultLooper, timeout]() mutable {
         try {
             // Launch inner task for actual function execution
             auto taskFuture = std::async(std::launch::async, std::move(func));
             
             // Wait for completion or timeout
             if (taskFuture.wait_for(timeout) == std::future_status::timeout) {
                 // Timeout occurred - create timeout exception
                 auto timeoutException = std::make_exception_ptr(
                     CpuTaskTimeoutException("CPU task timeout after " + 
                         std::to_string(timeout.count()) + "ms"));
                 // Post timeout exception back to event loop thread
                 resultLooper->post([promise, timeoutException]() mutable {
                     promise.set_exception(timeoutException);
                 });
                 return;  // Exit timeout management task
             }
             
             // Task completed within timeout - retrieve result
             if constexpr (std::is_void_v<ReturnType>) {
                 // Handle void-returning function
                 taskFuture.get();  // May throw if function threw
                 // Post success result back to event loop thread
                 resultLooper->post([promise]() mutable {
                     promise.set_value();
                 });
             } else {
                 // Handle value-returning function
                 auto result = taskFuture.get();  // May throw if function threw
                 // Post result back to event loop thread
                 resultLooper->post([promise, result = std::move(result)]() mutable {
                     promise.set_value(std::move(result));
                 });
             }
             
         } catch (...) {
             // Capture any exception (from taskFuture.get() or other operations)
             auto exception = std::current_exception();
             // Post exception back to event loop thread
             resultLooper->post([promise, exception]() mutable {
                 promise.set_exception(exception);
             });
         }
     });
     
     // Suppress unused variable warning - future provides timeout management
     // but Promise provides the primary coordination mechanism
     (void)asyncFuture;
     
     return promise;
 }
 
 } // namespace swt