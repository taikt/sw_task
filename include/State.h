/**
 * @file State.h
 * @brief Promise/Future state management with template specialization for void types
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #ifndef STATE_HPP
 #define STATE_HPP
 
 #include <optional>
 #include <functional>
 #include <memory>
 #include <iostream>
 #include <variant>
 #include <exception>
 #include "Log.h"
 
 using namespace std;
 
 // Forward declaration
 class SLLooper;
 
 namespace kt {
 
 /**
  * @class State
  * @brief Thread-safe promise state management with continuation support
  * @tparam tValue Type of value stored in the promise state
  * 
  * State provides the core functionality for promise/future pattern implementation
  * with asynchronous callback execution via SLLooper integration. Key features:
  * 
  * - **Value storage**: Optional-based value storage with move semantics
  * - **Exception handling**: std::exception_ptr for type-safe error propagation
  * - **Continuation support**: Callback registration and execution
  * - **Thread safety**: Asynchronous execution via SLLooper message queue
  * - **Template specialization**: Special handling for void-returning promises
  * 
  * The State class acts as the shared state between Promise and Future objects,
  * allowing thread-safe communication of results and errors across thread boundaries.
  * 
  * @code{.cpp}
  * // Create state for integer promise
  * auto state = std::make_shared<State<int>>();
  * 
  * // Register continuation
  * state->setContinuation(looper, [](int value) {
  *     std::cout << "Received: " << value << std::endl;
  * });
  * 
  * // Register error handler
  * state->setErrorHandler(looper, [](std::exception_ptr ex) {
  *     try {
  *         std::rethrow_exception(ex);
  *     } catch (const std::exception& e) {
  *         std::cerr << "Error: " << e.what() << std::endl;
  *     }
  * });
  * 
  * // Set value (triggers continuation)
  * state->setValue(42);
  * @endcode
  * 
  * @note Thread-safe when used with SLLooper for callback execution
  * @warning Direct member access is not thread-safe - use provided methods
  * 
  * @see Promise, Future, SLLooper
  */
 template <typename tValue>
 class State 
 {
 public:
     /**
      * @brief Default constructor - initializes empty state
      * 
      * Creates a new State object with no value, no exception,
      * and no registered callbacks.
      */
     State() {}
     
     /**
      * @brief Destructor - cleanup state
      * 
      * Cleans up any stored value, exception, and callback functions.
      * Does not automatically execute pending callbacks.
      */
     ~State() {}
 
     /**
      * @brief Set the promise value and trigger continuation
      * @param value Value to store (moved into internal storage)
      * 
      * Sets the promise value using move semantics and immediately
      * executes any registered continuation callback if available.
      * This method can only be called once per State instance.
      * 
      * @note Implementation in State.tpp
      * @note Thread-safe execution via SLLooper
      */
     void setValue(tValue &&value);
     
     /**
      * @brief Set exception state and trigger error handler
      * @param exception Exception pointer to store
      * 
      * Sets the promise to failed state and immediately executes
      * any registered error handler if available. This method
      * can only be called once per State instance.
      * 
      * @note Implementation in State.tpp
      * @note Thread-safe execution via SLLooper
      */
     void setException(std::exception_ptr exception);
 
     /**
      * @brief Register continuation callback for successful resolution
      * @tparam F Function type for continuation callback
      * @param looper_ SLLooper instance for callback execution
      * @param continuation Callback function to execute when value is available
      * 
      * Registers a continuation callback that will be executed when the
      * promise resolves successfully. If the promise is already resolved,
      * the callback is executed immediately.
      * 
      * @note Implementation in State.tpp
      * @note Uses perfect forwarding to preserve function properties
      */
     template<typename F>
     void setContinuation(std::shared_ptr<SLLooper>& looper_, F&& continuation);
     
     /**
      * @brief Register error handler callback for promise rejection
      * @tparam F Function type for error handler callback
      * @param looper_ SLLooper instance for error handler execution
      * @param errorHandler Callback function to execute when exception occurs
      * 
      * Registers an error handler that will be executed when the promise
      * is rejected with an exception. If an exception is already set,
      * the error handler is executed immediately.
      * 
      * @note Implementation in State.tpp
      * @note Error handler receives std::exception_ptr parameter
      */
     template<typename F>
     void setErrorHandler(std::shared_ptr<SLLooper>& looper_, F&& errorHandler);
 
 private:
     /**
      * @brief Execute continuation callback asynchronously
      * @param value Value to pass to continuation callback
      * 
      * Posts the continuation callback to the associated SLLooper's
      * message queue for asynchronous execution in the correct thread context.
      * 
      * @note Implementation in State.tpp
      */
     void executeContination(tValue value);
     
     /**
      * @brief Execute error handler callback asynchronously
      * @param exception Exception pointer to pass to error handler
      * 
      * Posts the error handler callback to the associated SLLooper's
      * message queue for asynchronous execution.
      * 
      * @note Implementation in State.tpp
      */
     void executeErrorHandler(std::exception_ptr exception);
     
     std::optional<tValue> m_value;                           /**< Optional value storage with move semantics */
     std::exception_ptr m_exception;                         /**< Exception pointer for error state */
     std::function<void(tValue)> m_continuation;             /**< Continuation callback for successful resolution */
     std::function<void(std::exception_ptr)> m_errorHandler; /**< Error handler callback for exception cases */
     std::shared_ptr<SLLooper> m_looper;                     /**< SLLooper for continuation execution */
     std::shared_ptr<SLLooper> m_errorLooper;                /**< SLLooper for error handler execution */
 };
 
 // ========== Template Specialization for void type (std::monostate) ==========
 
 /**
  * @class State<std::monostate>
  * @brief Template specialization for void-returning promises
  * 
  * This specialization handles promises that don't return values (void functions)
  * by using std::monostate as the internal value type. This allows the promise
  * framework to work uniformly with both value-returning and void functions
  * while maintaining type safety.
  * 
  * Key differences from the general template:
  * - **Continuation signature**: void() instead of void(tValue)
  * - **Internal value type**: std::monostate instead of tValue
  * - **API consistency**: Same interface as general template
  * 
  * @code{.cpp}
  * // Create state for void promise
  * auto voidState = std::make_shared<State<std::monostate>>();
  * 
  * // Register continuation (no parameters)
  * voidState->setContinuation(looper, []() {
  *     std::cout << "Void promise completed!" << std::endl;
  * });
  * 
  * // Set completion (using monostate)
  * voidState->setValue(std::monostate{});
  * @endcode
  * 
  * @note Specialization maintains API consistency for void promises
  * @note std::monostate is a dummy type representing "no value"
  * 
  * @see State, Promise<void>, Future<void>
  */
 template <>
 class State<std::monostate>
 {
 public:
     /**
      * @brief Default constructor - initializes empty void state
      */
     State() {}
     
     /**
      * @brief Destructor - cleanup void state
      */
     ~State() {}
 
     /**
      * @brief Set completion state for void promise
      * @param value std::monostate value (represents completion)
      * 
      * Marks the void promise as completed and triggers any registered
      * continuation callback. Uses std::monostate to represent completion
      * without an actual value.
      * 
      * @note Implementation in State.tpp
      */
     void setValue(std::monostate &&value);
     
     /**
      * @brief Set exception state for void promise
      * @param exception Exception pointer to store
      * 
      * Identical behavior to general template - sets exception state
      * and triggers error handler if available.
      * 
      * @note Implementation in State.tpp
      */
     void setException(std::exception_ptr exception);
 
     /**
      * @brief Register continuation callback for void promise completion
      * @tparam F Function type for continuation callback
      * @param looper_ SLLooper instance for callback execution
      * @param continuation Callback function with void() signature
      * 
      * Registers a continuation callback that takes no parameters,
      * appropriate for void-returning promises.
      * 
      * @note Continuation signature is void() not void(std::monostate)
      * @note Implementation in State.tpp
      */
     template<typename F>
     void setContinuation(std::shared_ptr<SLLooper>& looper_, F&& continuation);
     
     /**
      * @brief Register error handler for void promise rejection
      * @tparam F Function type for error handler callback
      * @param looper_ SLLooper instance for error handler execution
      * @param errorHandler Callback function to execute when exception occurs
      * 
      * Identical behavior to general template - registers error handler
      * with std::exception_ptr parameter.
      * 
      * @note Implementation in State.tpp
      */
     template<typename F>
     void setErrorHandler(std::shared_ptr<SLLooper>& looper_, F&& errorHandler);
 
 private:
     /**
      * @brief Execute continuation callback for void promise
      * @param value std::monostate value (ignored in execution)
      * 
      * Posts the void continuation callback to SLLooper's message queue.
      * The monostate parameter is ignored since void continuations
      * take no parameters.
      * 
      * @note Implementation in State.tpp
      */
     void executeContination(std::monostate value);
     
     /**
      * @brief Execute error handler callback (identical to general template)
      * @param exception Exception pointer to pass to error handler
      * 
      * @note Implementation in State.tpp
      */
     void executeErrorHandler(std::exception_ptr exception);
     
     std::optional<std::monostate> m_value;                  /**< Optional monostate storage (represents completion) */
     std::exception_ptr m_exception;                         /**< Exception pointer for error state */
     std::function<void()> m_continuation;                   /**< Void continuation callback (no parameters) */
     std::function<void(std::exception_ptr)> m_errorHandler; /**< Error handler callback (same as general template) */
     std::shared_ptr<SLLooper> m_looper;                     /**< SLLooper for continuation execution */
     std::shared_ptr<SLLooper> m_errorLooper;                /**< SLLooper for error handler execution */
 };
 
 } // namespace kt
 
 // Include template implementations
 #include "State.tpp"
 
 #endif // STATE_HPP