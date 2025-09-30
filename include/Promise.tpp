/**
 * @file Promise.tpp
 * @brief Template implementation for Promise class - asynchronous result handling with continuation chaining
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #ifndef PROMISE_TPP
 #define PROMISE_TPP
 
 #include "Promise.h"
 #include "State.h"
 
 namespace swt {
 
 // ========== Template implementation for Promise<tValue> ==========
 
 /**
  * @brief Default constructor - creates promise with shared state
  * @tparam tValue Value type for promise result
  * 
  * Initializes a new promise with a shared State object for
  * communication between promise and future/continuation handlers.
  * 
  * @see \ref swt::Promise "Promise", \ref swt::State "State"
  */
 template <typename tValue>
 Promise<tValue>::Promise() : m_state(std::make_shared<State<tValue>>()) {}
 
 /**
  * @brief Set the promise value and trigger continuation chain
  * @tparam tValue Value type being set
  * @param value Value to set (moved into promise state)
  * 
  * Resolves the promise with the provided value and triggers any
  * registered continuation callbacks. This can only be called once
  * per promise instance.
  * 
  * @code{.cpp}
  * Promise<int> promise;
  * promise.set_value(42);  // Resolves promise and triggers continuations
  * @endcode
  * 
  * @see \ref swt::Promise::set_value "set_value()"
  */
 template <typename tValue>
 void Promise<tValue>::set_value(tValue value) {
     m_state->setValue(std::move(value));
 }
 
 /**
  * @brief Set exception state and trigger error handlers
  * @tparam tValue Value type (not used for exception)
  * @param exception Exception pointer to set
  * 
  * Rejects the promise with the provided exception and triggers any
  * registered error handlers. This can only be called once per promise.
  * 
  * @code{.cpp}
  * Promise<int> promise;
  * try {
  *     // Some operation that may fail
  * } catch (...) {
  *     promise.set_exception(std::current_exception());
  * }
  * @endcode
  * 
  * @see \ref swt::Promise::set_exception "set_exception()"
  */
 template <typename tValue>
 void Promise<tValue>::set_exception(std::exception_ptr exception) {
     m_state->setException(exception);
 }
 
 /**
  * @brief Chain continuation callback for promise resolution
  * @tparam tValue Current promise value type
  * @tparam F Function type for continuation callback
  * @param looper_ SLLooper instance for callback execution
  * @param func Continuation function to execute when promise resolves
  * @return Promise<ReturnType> New promise for further chaining
  * 
  * Registers a continuation callback that executes when the promise resolves
  * successfully. The continuation function receives the resolved value and
  * can return a new value, creating a chain of promise transformations.
  * 
  * Key features:
  * - **Type transformation**: Return type can differ from input type
  * - **Exception handling**: Automatic exception propagation in continuation chain
  * - **Void support**: Handles both value-returning and void continuations
  * - **Asynchronous execution**: Callbacks execute via SLLooper message queue
  * 
  * @code{.cpp}
  * promise.then(looper, [](int x) { 
  *     return x * 2; 
  * }).then(looper, [](int doubled) {
  *     std::cout << "Result: " << doubled << std::endl;
  * });
  * @endcode
  * 
  * @see \ref swt::Promise::then "then()", \ref swt::SLLooper "SLLooper"
  */
 template <typename tValue>
 template <typename F>
 auto Promise<tValue>::then(std::shared_ptr<SLLooper>& looper_, F func) -> Promise<std::invoke_result_t<F, tValue>> {
     using ReturnType = std::invoke_result_t<F, tValue>;
     Promise<ReturnType> nextPromise;
     
     // Set continuation that handles success case
     m_state->setContinuation(looper_, [func = std::move(func), nextPromise](tValue value) mutable {
         try {
             if constexpr (std::is_void_v<ReturnType>) {
                 // Handle void-returning continuation
                 func(std::move(value));
                 nextPromise.set_value();
             } else {
                 // Handle value-returning continuation
                 auto result = func(std::move(value));
                 nextPromise.set_value(std::move(result));
             }
         } catch (...) {
             // Automatic exception propagation
             nextPromise.set_exception(std::current_exception());
         }
     });
     
     // Set error handler that propagates exception through chain
     m_state->setErrorHandler(looper_, [nextPromise](std::exception_ptr exception) mutable {
         nextPromise.set_exception(exception);
     });
     
     return nextPromise;
 }
 
 /**
  * @brief Chain error handler for promise rejection
  * @tparam tValue Current promise value type
  * @tparam F Function type for error handler callback
  * @param looper_ SLLooper instance for callback execution
  * @param func Error handler function to execute when promise is rejected
  * @return Promise<tValue> New promise for further chaining
  * 
  * Registers an error handler that executes when the promise is rejected.
  * The error handler can recover from the error by returning a value, or
  * let the error propagate by throwing or not handling it.
  * 
  * @code{.cpp}
  * promise.catchError(looper, [](std::exception_ptr ex) -> int {
  *     try {
  *         std::rethrow_exception(ex);
  *     } catch (const MyException& e) {
  *         return 42;  // Recovery value
  *     }
  *     // Other exceptions will be re-thrown
  * });
  * @endcode
  * 
  * @see \ref swt::Promise::catchError "catchError()", \ref swt::SLLooper "SLLooper"
  */
 template <typename tValue>
 template <typename F>
 auto Promise<tValue>::catchError(std::shared_ptr<SLLooper>& looper_, F func) -> Promise<tValue> {
     Promise<tValue> nextPromise;
     
     // Set continuation that propagates success value
     m_state->setContinuation(looper_, [nextPromise](tValue value) mutable {
         nextPromise.set_value(std::move(value));
     });
     
     // Set error handler that calls the catch function
     m_state->setErrorHandler(looper_, [func = std::move(func), nextPromise](std::exception_ptr exception) mutable {
         try {
             if constexpr (std::is_void_v<std::invoke_result_t<F, std::exception_ptr>>) {
                 // Void error handler - propagate original exception
                 func(exception);
                 nextPromise.set_exception(exception);
             } else {
                 // Value-returning error handler - use result as recovery value
                 auto result = func(exception);
                 nextPromise.set_value(std::move(result));
             }
         } catch (...) {
             // Error handler threw - propagate new exception
             nextPromise.set_exception(std::current_exception());
         }
     });
     
     return nextPromise;
 }
 
 // ========== Template specialization for Promise<void> ==========
 
 /**
  * @brief Chain continuation callback for void promise resolution
  * @tparam F Function type for continuation callback
  * @param looper_ SLLooper instance for callback execution
  * @param func Continuation function (takes no parameters)
  * @return Promise<ReturnType> New promise for further chaining
  * 
  * Specialization for void promises that handles continuation chaining
  * when the current promise doesn't produce a value. The continuation
  * function takes no parameters and can return any type.
  * 
  * @code{.cpp}
  * voidPromise.then(looper, []() { 
  *     return "Void promise completed!"; 
  * }).then(looper, [](const std::string& msg) {
  *     std::cout << msg << std::endl;
  * });
  * @endcode
  * 
  * @see \ref swt::Promise::then "then()", \ref swt::SLLooper "SLLooper"
  */
 template <typename F>
 auto Promise<void>::then(std::shared_ptr<SLLooper>& looper_, F func) -> Promise<std::invoke_result_t<F>> {
     using ReturnType = std::invoke_result_t<F>;
     Promise<ReturnType> nextPromise;
     
     // Set continuation that handles success case for void promise
     m_state->setContinuation(looper_, [func = std::move(func), nextPromise]() mutable {
         try {
             if constexpr (std::is_void_v<ReturnType>) {
                 // Void -> Void continuation
                 func();
                 nextPromise.set_value();
             } else {
                 // Void -> Value continuation
                 auto result = func();
                 nextPromise.set_value(std::move(result));
             }
         } catch (...) {
             // Automatic exception propagation
             nextPromise.set_exception(std::current_exception());
         }
     });
     
     // Set error handler that propagates exception through chain
     m_state->setErrorHandler(looper_, [nextPromise](std::exception_ptr exception) mutable {
         nextPromise.set_exception(exception);
     });
     
     return nextPromise;
 }
 
 /**
  * @brief Chain error handler for void promise rejection
  * @tparam F Function type for error handler callback
  * @param looper_ SLLooper instance for callback execution
  * @param func Error handler function to execute when promise is rejected
  * @return Promise<void> New void promise for further chaining
  * 
  * Specialization for void promises that handles error recovery.
  * If the error handler executes successfully (without throwing),
  * the promise chain continues with success state.
  * 
  * @code{.cpp}
  * voidPromise.catchError(looper, [](std::exception_ptr ex) {
  *     std::cerr << "Error handled, continuing..." << std::endl;
  *     // Not throwing means error is handled
  * }).then(looper, []() {
  *     std::cout << "Promise chain continues!" << std::endl;
  * });
  * @endcode
  * 
  * @see \ref swt::Promise::catchError "catchError()", \ref swt::SLLooper "SLLooper"
  */
 template <typename F>
 auto Promise<void>::catchError(std::shared_ptr<SLLooper>& looper_, F func) -> Promise<void> {
     Promise<void> nextPromise;
     
     // Set continuation that propagates success (no value)
     m_state->setContinuation(looper_, [nextPromise]() mutable {
         nextPromise.set_value();
     });
     
     // Set error handler that calls the catch function
     m_state->setErrorHandler(looper_, [func = std::move(func), nextPromise](std::exception_ptr exception) mutable {
         try {
             // Call error handler
             func(exception);
             // If no exception thrown, error is handled - continue with success
             nextPromise.set_value();
         } catch (...) {
             // Error handler threw - propagate new exception
             nextPromise.set_exception(std::current_exception());
         }
     });
     
     return nextPromise;
 }
 
 } // namespace swt
 
 #endif // PROMISE_TPP