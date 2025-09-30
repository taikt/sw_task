/**
 * @file State.tpp
 * @brief Template implementation for State class - Promise/Future state management
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #ifndef STATE_TPP
 #define STATE_TPP
 
 #include "State.h"
 #include "SLLooper.h"
 
 namespace swt {
 
 /**
  * @brief Set the promise value and execute continuation if available
  * @tparam tValue Value type being stored in the promise
  * @param value Value to store (moved into internal storage)
  * 
  * Sets the promise value using move semantics and immediately executes
  * any registered continuation callback if both continuation and looper
  * are available. This provides immediate callback execution for already-
  * resolved promises.
  * 
  * @note Uses perfect forwarding to avoid unnecessary copies
  * @note Thread-safe when used with SLLooper's message queue
  * 
  * @code{.cpp}
  * auto state = std::make_shared<State<int>>();
  * state->setValue(42);  // Moves value and executes continuation
  * @endcode
  * 
  * @see \ref swt::State::setValue "setValue"
  */
 template <typename tValue>
 void State<tValue>::setValue(tValue &&value) {
     m_value = std::move(value);
     if (m_continuation && m_looper) {
         executeContination(std::move(*m_value));
     } else {
         // std::cout << "m_continuation or m_looper is empty\n";
     }
 }
 
 /**
  * @brief Set exception state and execute error handler if available
  * @tparam tValue Value type being stored in the promise
  * @param exception Exception pointer to store
  * 
  * Sets the promise to failed state with the provided exception and
  * immediately executes any registered error handler if both error
  * handler and looper are available.
  * 
  * @note Exception is stored as std::exception_ptr for type erasure
  * @note Thread-safe execution via SLLooper message queue
  * 
  * @code{.cpp}
  * auto state = std::make_shared<State<int>>();
  * try {
  *     // Some operation that may throw
  * } catch (...) {
  *     state->setException(std::current_exception());
  * }
  * @endcode
  * 
  * @see \ref swt::State::setException "setException"
  */
 template <typename tValue>
 void State<tValue>::setException(std::exception_ptr exception) {
     m_exception = exception;
     if (m_errorHandler && m_errorLooper) {
         executeErrorHandler(m_exception);
     } else {
         std::cout << "m_errorHandler or m_errorLooper is empty\n";
     }
 }
 
 /**
  * @brief Register continuation callback for promise resolution
  * @tparam tValue Value type being stored in the promise
  * @tparam F Function type for continuation callback
  * @param looper_ SLLooper instance for callback execution
  * @param continuation Callback function to execute when value is available
  * 
  * Registers a continuation callback that will be executed when the promise
  * resolves successfully. If the promise is already resolved, the callback
  * is executed immediately. If an exception is already set, it's propagated
  * to the error handler.
  * 
  * @note Uses perfect forwarding to preserve function object properties
  * @note Handles race conditions by checking existing state
  * 
  * @code{.cpp}
  * state->setContinuation(looper, [](int result) {
  *     std::cout << "Promise resolved with: " << result << std::endl;
  * });
  * @endcode
  * 
  * @see \ref swt::State::setContinuation "setContinuation", \ref swt::SLLooper "SLLooper"
  */
 template <typename tValue>
 template<typename F>
 void State<tValue>::setContinuation(std::shared_ptr<SLLooper>& looper_, F&& continuation) {
     m_looper = looper_;
     m_continuation = std::forward<F>(continuation);
     
     // If value is already set, execute immediately
     if (m_value.has_value()) {
         executeContination(*m_value);
     } else if (m_exception) {
         // If exception is already set, propagate to error handler
         if (m_errorHandler && m_errorLooper) {
             executeErrorHandler(m_exception);
         }
     }
 }
 
 /**
  * @brief Register error handler for promise rejection
  * @tparam tValue Value type being stored in the promise
  * @tparam F Function type for error handler callback
  * @param looper_ SLLooper instance for error handler execution
  * @param errorHandler Callback function to execute when exception occurs
  * 
  * Registers an error handler that will be executed when the promise
  * is rejected with an exception. If an exception is already set,
  * the error handler is executed immediately.
  * 
  * @note Error handler receives std::exception_ptr for type safety
  * @note Handles immediate execution for already-failed promises
  * 
  * @code{.cpp}
  * state->setErrorHandler(looper, [](std::exception_ptr ex) {
  *     try {
  *         std::rethrow_exception(ex);
  *     } catch (const std::exception& e) {
  *         std::cerr << "Promise failed: " << e.what() << std::endl;
  *     }
  * });
  * @endcode
  * 
  * @see \ref swt::State::setErrorHandler "setErrorHandler", \ref swt::SLLooper "SLLooper"
  */
 template <typename tValue>
 template<typename F>
 void State<tValue>::setErrorHandler(std::shared_ptr<SLLooper>& looper_, F&& errorHandler) {
     m_errorLooper = looper_;
     m_errorHandler = std::forward<F>(errorHandler);
     
     // If exception is already set, execute immediately
     if (m_exception) {
         executeErrorHandler(m_exception);
     }
 }
 
 /**
  * @brief Execute continuation callback asynchronously via SLLooper
  * @tparam tValue Value type being passed to continuation
  * @param value Value to pass to continuation callback
  * 
  * Posts the continuation callback to the associated SLLooper's message
  * queue for asynchronous execution. This ensures the callback runs in
  * the correct thread context and maintains thread safety.
  * 
  * @note Uses lambda capture with move semantics for efficiency
  * @note Private method called internally by setValue() and setContinuation()
  * 
  * @see \ref swt::SLLooper "SLLooper"
  */
 template <typename tValue>
 void State<tValue>::executeContination(tValue value) {
     if (m_looper && m_continuation) {
         m_looper->post([continuation = m_continuation, value = std::move(value)]() mutable {
             continuation(std::move(value));
         });
     }
 }
 
 /**
  * @brief Execute error handler callback asynchronously via SLLooper
  * @tparam tValue Value type (not used in error handling)
  * @param exception Exception pointer to pass to error handler
  * 
  * Posts the error handler callback to the associated SLLooper's message
  * queue for asynchronous execution. This ensures error handling occurs
  * in the correct thread context.
  * 
  * @note Exception pointer is copyable and safe to capture
  * @note Private method called internally by setException() and error propagation
  * 
  * @see \ref swt::SLLooper "SLLooper"
  */
 template <typename tValue>
 void State<tValue>::executeErrorHandler(std::exception_ptr exception) {
     if (m_errorLooper && m_errorHandler) {
         m_errorLooper->post([errorHandler = m_errorHandler, exception]() mutable {
             errorHandler(exception);
         });
     }
 }
 
 // ========== Template Specialization for void type (std::monostate) ==========
 
 /**
  * @brief Specialization: Register continuation for void promises
  * @tparam F Function type for continuation callback
  * @param looper_ SLLooper instance for callback execution
  * @param continuation Callback function to execute (takes no parameters)
  * 
  * Template specialization for handling void-returning promises using
  * std::monostate as the internal value type. This allows the promise
  * framework to work uniformly with both value-returning and void functions.
  * 
  * @note Specialization for State<std::monostate> (represents void)
  * @note Continuation callback takes no parameters for void promises
  * 
  * @code{.cpp}
  * auto voidState = std::make_shared<State<std::monostate>>();
  * voidState->setContinuation(looper, []() {
  *     std::cout << "Void promise completed!" << std::endl;
  * });
  * @endcode
  * 
  * @see \ref swt::State::setContinuation "setContinuation", \ref swt::SLLooper "SLLooper"
  */
 template<typename F>
 void State<std::monostate>::setContinuation(std::shared_ptr<SLLooper>& looper_, F&& continuation) {
     m_looper = looper_;
     m_continuation = std::forward<F>(continuation);
     
     // If value is already set, execute immediately
     if (m_value.has_value()) {
         executeContination(*m_value);
     } else if (m_exception) {
         // If exception is already set, propagate to error handler
         if (m_errorHandler && m_errorLooper) {
             executeErrorHandler(m_exception);
         }
     }
 }
 
 /**
  * @brief Specialization: Register error handler for void promises
  * @tparam F Function type for error handler callback
  * @param looper_ SLLooper instance for error handler execution
  * @param errorHandler Callback function to execute when exception occurs
  * 
  * Template specialization for error handling in void-returning promises.
  * Identical behavior to the general template but specialized for
  * State<std::monostate> type consistency.
  * 
  * @note Specialization maintains API consistency for void promises
  * @note Error handler signature remains the same (takes std::exception_ptr)
  * 
  * @see \ref swt::State::setErrorHandler "setErrorHandler", \ref swt::SLLooper "SLLooper"
  */
 template<typename F>
 void State<std::monostate>::setErrorHandler(std::shared_ptr<SLLooper>& looper_, F&& errorHandler) {
     m_errorLooper = looper_;
     m_errorHandler = std::forward<F>(errorHandler);
     
     // If exception is already set, execute immediately
     if (m_exception) {
         executeErrorHandler(m_exception);
     }
 }
 
 } // namespace swt
 
 #endif // STATE_TPP