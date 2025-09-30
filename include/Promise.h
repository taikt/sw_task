/**
 * @file Promise.h
 * @brief Promise class for asynchronous result handling with continuation chaining and type safety
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #ifndef PROMISE_HPP
 #define PROMISE_HPP
 
 #include <type_traits>
 #include <memory>
 #include <functional>
 #include <variant>
 #include <exception>
 #include "Log.h"
 
 // Forward declarations
 class SLLooper;
 
 namespace kt {
     template <typename tValue> class State;
 }
 
 namespace kt {
 
 /**
  * @class Promise
  * @brief Type-safe promise for asynchronous result handling with continuation chaining
  * @tparam tValue Type of value that the promise will resolve to
  * 
  * Promise provides a modern C++ implementation of the promise/future pattern with:
  * - **Type-safe continuation chaining**: Automatic type deduction for chained operations
  * - **Exception propagation**: Automatic error handling through the promise chain
  * - **Asynchronous execution**: Integration with SLLooper for thread-safe callback execution
  * - **Move semantics**: Efficient value transfer without unnecessary copies
  * - **Template specialization**: Special handling for void-returning promises
  * 
  * Key features:
  * - **Continuation chaining**: then() method for sequential async operations
  * - **Error handling**: catchError() method for exception recovery
  * - **Thread safety**: Callbacks execute in specified SLLooper thread context
  * - **Type transformation**: Support for changing value types through the chain
  * 
  * The Promise class works in conjunction with State objects to manage the
  * asynchronous state and provide thread-safe communication between producers
  * and consumers of asynchronous results.
  * 
  * @code{.cpp}
  * // Create and resolve promise
  * Promise<int> promise;
  * promise.set_value(42);
  * 
  * // Chain operations with type transformation
  * promise.then(looper, [](int x) {
  *     return std::to_string(x * 2);  // int -> string
  * }).then(looper, [](const std::string& s) {
  *     std::cout << "Result: " << s << std::endl;
  * }).catchError(looper, [](std::exception_ptr ex) {
  *     std::cerr << "Error in promise chain!" << std::endl;
  * });
  * @endcode
  * 
  * @note Thread-safe when used with SLLooper for callback execution
  * @warning Promise can only be resolved once with either value or exception
  * 
  * @see State, SLLooper, Future
  */
 template <typename tValue>
 class Promise
 {
 public:
     /**
      * @brief Default constructor - creates promise with shared state
      * 
      * Initializes a new promise with a shared State object for
      * communication with continuation handlers and error handlers.
      * 
      * @note Implementation in Promise.tpp
      */
     Promise();
 
     /**
      * @brief Resolve promise with a value
      * @param value Value to resolve the promise with (moved)
      * 
      * Sets the promise value and triggers any registered continuation
      * callbacks. This method can only be called once per promise instance.
      * Subsequent calls are ignored.
      * 
      * @note Implementation in Promise.tpp
      * @note Thread-safe operation
      * 
      * @code{.cpp}
      * Promise<int> promise;
      * promise.set_value(42);  // Resolves and triggers continuations
      * @endcode
      */
     void set_value(tValue value);
     
     /**
      * @brief Reject promise with an exception
      * @param exception Exception pointer to reject the promise with
      * 
      * Sets the promise to failed state and triggers any registered
      * error handlers. This method can only be called once per promise.
      * 
      * @note Implementation in Promise.tpp
      * @note Thread-safe operation
      * 
      * @code{.cpp}
      * Promise<int> promise;
      * try {
      *     // Some operation that may fail
      * } catch (...) {
      *     promise.set_exception(std::current_exception());
      * }
      * @endcode
      */
     void set_exception(std::exception_ptr exception);
 
     /**
      * @brief Chain continuation callback for promise resolution
      * @tparam F Function type for continuation callback (auto-deduced)
      * @param looper_ SLLooper instance for callback execution thread context
      * @param func Continuation function to execute when promise resolves
      * @return Promise<ReturnType> New promise for further chaining
      * 
      * Registers a continuation callback that executes when the promise resolves
      * successfully. The continuation function receives the resolved value and
      * can return a new value of any type, enabling type transformation chains.
      * 
      * Key capabilities:
      * - **Type transformation**: Return type can differ from input type
      * - **Exception safety**: Automatic exception propagation through chain
      * - **Asynchronous execution**: Callbacks run in SLLooper thread context
      * - **Chaining**: Returns new promise for further then()/catchError() calls
      * 
      * @note Implementation in Promise.tpp
      * @note Thread-safe registration and execution
      * 
      * @code{.cpp}
      * promise.then(looper, [](int x) { 
      *     return x * 2; 
      * }).then(looper, [](int doubled) -> std::string {
      *     return "Result: " + std::to_string(doubled);
      * }).then(looper, [](const std::string& msg) {
      *     std::cout << msg << std::endl;
      * });
      * @endcode
      */
     template <typename F>
     auto then(std::shared_ptr<SLLooper>& looper_, F func) -> Promise<std::invoke_result_t<F, tValue>>;
 
     /**
      * @brief Chain error handler for exception recovery
      * @tparam F Function type for error handler callback (auto-deduced)
      * @param looper_ SLLooper instance for callback execution thread context
      * @param func Error handler function that receives std::exception_ptr
      * @return Promise<tValue> New promise continuing the chain
      * 
      * Registers an error handler that executes when the promise is rejected.
      * The error handler can recover from errors by returning a value, or
      * let errors propagate by throwing or returning nothing.
      * 
      * Error recovery patterns:
      * - **Recovery**: Return a valid tValue to continue with success
      * - **Propagation**: Throw or re-throw to propagate error down chain
      * - **Logging**: Log error and return default value for graceful degradation
      * 
      * @note Implementation in Promise.tpp
      * @note Thread-safe registration and execution
      * 
      * @code{.cpp}
      * promise.catchError(looper, [](std::exception_ptr ex) -> int {
      *     try {
      *         std::rethrow_exception(ex);
      *     } catch (const NetworkException& e) {
      *         return -1;  // Recovery value for network errors
      *     } catch (const TimeoutException& e) {
      *         return -2;  // Recovery value for timeout errors
      *     }
      *     // Other exceptions will be re-thrown
      * });
      * @endcode
      */
     template <typename F>
     auto catchError(std::shared_ptr<SLLooper>& looper_, F func) -> Promise<tValue>;
 
     /**
      * @brief Function call operator for convenient promise resolution
      * @param value Value to resolve the promise with
      * 
      * Convenience operator that allows using the promise as a callable
      * object for resolution. Equivalent to calling set_value(value).
      * 
      * @code{.cpp}
      * Promise<int> promise;
      * promise(42);  // Same as promise.set_value(42)
      * @endcode
      */
     void operator()(tValue value) {
         set_value(std::move(value));
     }
 
 private:
     std::shared_ptr<State<tValue>> m_state;  /**< Shared state for async communication */
 };
 
 // ========== Template Specialization for void type ==========
 
 /**
  * @class Promise<void>
  * @brief Template specialization for void-returning promises
  * 
  * This specialization handles promises that represent completion of operations
  * without returning values. Uses std::monostate internally to represent the
  * "no value" state while maintaining the same API as value-returning promises.
  * 
  * Key differences from general template:
  * - **set_value()**: Takes no parameters (represents completion)
  * - **Continuation signature**: Functions take no parameters
  * - **Internal state**: Uses State<std::monostate> instead of State<tValue>
  * - **API consistency**: Same chaining and error handling patterns
  * 
  * @code{.cpp}
  * // Create and resolve void promise
  * Promise<void> voidPromise;
  * voidPromise.set_value();  // Mark as completed
  * 
  * // Chain operations from void promise
  * voidPromise.then(looper, []() {
  *     std::cout << "Void operation completed!" << std::endl;
  *     return 42;  // Can return value from void continuation
  * }).then(looper, [](int x) {
  *     std::cout << "Got value: " << x << std::endl;
  * });
  * @endcode
  * 
  * @note Specialization maintains API consistency with value-returning promises
  * @note std::monostate used internally to represent completion state
  * 
  * @see Promise, State<std::monostate>
  */
 template <>
 class Promise<void>
 {
 public:
     /**
      * @brief Default constructor for void promise
      * 
      * Creates a void promise using State<std::monostate> for internal
      * state management while providing a clean void interface.
      * 
      * @note Implementation in Promise.tpp
      */
     Promise();
 
     /**
      * @brief Mark void promise as completed
      * 
      * Resolves the void promise by marking it as completed and
      * triggering any registered continuation callbacks. No value
      * is provided since this represents operation completion.
      * 
      * @note Implementation in Promise.tpp
      * @note Thread-safe operation
      * 
      * @code{.cpp}
      * Promise<void> promise;
      * promise.set_value();  // Mark as completed
      * @endcode
      */
     void set_value();
     
     /**
      * @brief Reject void promise with an exception
      * @param exception Exception pointer to reject the promise with
      * 
      * Identical behavior to general template - sets promise to failed
      * state and triggers error handlers.
      * 
      * @note Implementation in Promise.tpp
      * @note Thread-safe operation
      */
     void set_exception(std::exception_ptr exception);
 
     /**
      * @brief Chain continuation callback for void promise completion
      * @tparam F Function type for continuation callback (auto-deduced)
      * @param looper_ SLLooper instance for callback execution
      * @param func Continuation function (takes no parameters)
      * @return Promise<ReturnType> New promise for further chaining
      * 
      * Registers a continuation callback that executes when the void promise
      * completes. The continuation function takes no parameters since void
      * promises don't produce values, but can return any type.
      * 
      * @note Implementation in Promise.tpp
      * @note Continuation signature is F() not F(void)
      * 
      * @code{.cpp}
      * voidPromise.then(looper, []() { 
      *     return "Void operation completed!"; 
      * }).then(looper, [](const std::string& msg) {
      *     std::cout << msg << std::endl;
      * });
      * @endcode
      */
     template <typename F>
     auto then(std::shared_ptr<SLLooper>& looper_, F func) -> Promise<std::invoke_result_t<F>>;
 
     /**
      * @brief Chain error handler for void promise rejection
      * @tparam F Function type for error handler callback (auto-deduced)
      * @param looper_ SLLooper instance for callback execution
      * @param func Error handler function that receives std::exception_ptr
      * @return Promise<void> New void promise for further chaining
      * 
      * Registers an error handler for void promise rejection. If the error
      * handler executes successfully (without throwing), the promise chain
      * continues with success state.
      * 
      * @note Implementation in Promise.tpp
      * @note Error handler signature same as general template
      * 
      * @code{.cpp}
      * voidPromise.catchError(looper, [](std::exception_ptr ex) {
      *     std::cerr << "Void operation failed, but continuing..." << std::endl;
      *     // Not throwing means error is handled
      * }).then(looper, []() {
      *     std::cout << "Promise chain continues!" << std::endl;
      * });
      * @endcode
      */
     template <typename F>
     auto catchError(std::shared_ptr<SLLooper>& looper_, F func) -> Promise<void>;
 
     /**
      * @brief Function call operator for convenient void promise resolution
      * 
      * Convenience operator that allows using the void promise as a callable
      * object for completion marking. Equivalent to calling set_value().
      * 
      * @code{.cpp}
      * Promise<void> promise;
      * promise();  // Same as promise.set_value()
      * @endcode
      */
     void operator()() {
         set_value();
     }
 
 private:
     std::shared_ptr<State<std::monostate>> m_state;  /**< Shared state using monostate for void semantics */
 };
 
 } // namespace kt
 
 // Include template implementations
 #include "Promise.tpp"
 
 #endif // PROMISE_HPP