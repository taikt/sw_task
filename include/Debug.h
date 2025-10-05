/**
 * @file Debug.h
 * @brief Centralized debug and logging macros for SW Task Framework components
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once
 #include <iostream>
 #include <string>
 
 // ========== DEBUG CONFIGURATION ==========
 
 /**
  * @def TIMER_DEBUG_ENABLED
  * @brief Enable/disable debug output for TimerManager operations
  * 
  * Controls whether debug messages from TimerManager are printed.
  * Default is 0 (disabled) for production builds. Can be overridden
  * at compile time with -DTIMER_DEBUG_ENABLED=1.
  * 
  * @note Debug messages include timer creation, expiration, and cancellation
  * @note Production builds should keep this disabled for performance
  */
 #ifndef TIMER_DEBUG_ENABLED
 #define TIMER_DEBUG_ENABLED 0
 #endif
 
 /**
  * @def SLLOOPER_DEBUG_ENABLED
  * @brief Enable/disable debug output for SLLooper operations
  * 
  * Controls whether debug messages from SLLooper are printed.
  * Default is 0 (disabled) for production builds. Useful for debugging
  * event loop behavior, task posting, and thread coordination.
  * 
  * @note Debug messages include task posting, loop iterations, and shutdown
  * @note Can impact performance when enabled due to I/O operations
  */
 #ifndef SLLOOPER_DEBUG_ENABLED
 #define SLLOOPER_DEBUG_ENABLED 0
 #endif
 
 /**
  * @def EventQueue_DEBUG_ENABLED
  * @brief Enable/disable debug output for EventQueue operations
  * 
  * Controls whether debug messages from EventQueue are printed.
  * Default is 0 (disabled) for production builds. Helps debug
  * message queuing, function enqueuing, and polling behavior.
  * 
  * @note Debug messages include queue operations and timing information
  * @note High-frequency operations may generate significant output when enabled
  */
 #ifndef EventQueue_DEBUG_ENABLED
 #define EventQueue_DEBUG_ENABLED 0
 #endif
 
 // ========== TIMER MANAGER DEBUG MACROS ==========
 
 /**
  * @def TIMER_DEBUG(msg)
  * @brief Conditional debug logging for TimerManager with simple message
  * @param msg String literal or expression for debug message
  * 
  * Prints debug message with [TimerManager] prefix only if TIMER_DEBUG_ENABLED
  * is true. Uses do-while(0) idiom for safe macro expansion in all contexts.
  * 
  * @code{.cpp}
  * TIMER_DEBUG("Creating new timer with 1000ms delay");
  * @endcode
  * 
  * @note Zero runtime cost when TIMER_DEBUG_ENABLED is 0
  * @note Thread-safe assuming std::cout is thread-safe
  */
 #define TIMER_DEBUG(msg) \
     do { \
         if (TIMER_DEBUG_ENABLED) { \
             std::cout << "[TimerManager] " << msg << std::endl; \
         } \
     } while(0)
 
 /**
  * @def TIMER_DEBUG_STREAM(stream_expr)
  * @brief Conditional debug logging for TimerManager with stream expression
  * @param stream_expr Stream expression (e.g., "Timer " << id << " expired")
  * 
  * Prints debug message using stream operations only if TIMER_DEBUG_ENABLED
  * is true. Allows complex formatted output with variables and expressions.
  * 
  * @code{.cpp}
  * TIMER_DEBUG_STREAM("Timer " << timerId << " scheduled for " << delay << "ms");
  * @endcode
  * 
  * @note Expressions are only evaluated when debug is enabled
  * @note More flexible than TIMER_DEBUG for complex formatting
  */
 #define TIMER_DEBUG_STREAM(stream_expr) \
     do { \
         if (TIMER_DEBUG_ENABLED) { \
             std::cout << "[TimerManager] " << stream_expr << std::endl; \
         } \
     } while(0)
 
 /**
  * @def TIMER_ERROR(msg)
  * @brief Always-on error logging for TimerManager critical issues
  * @param msg String literal or expression for error message
  * 
  * Prints error message with [TimerManager ERROR] prefix unconditionally.
  * Used for critical errors that should always be logged regardless of
  * debug settings (e.g., system call failures, resource allocation errors).
  * 
  * @code{.cpp}
  * TIMER_ERROR("Failed to create timerfd: " << strerror(errno));
  * @endcode
  * 
  * @note Always enabled - use sparingly for true error conditions
  * @note Consider exception handling instead of just logging
  */
 #define TIMER_ERROR(msg) \
     std::cout << "[TimerManager ERROR] " << msg << std::endl
 
 /**
  * @def TIMER_ERROR_STREAM(stream_expr)
  * @brief Always-on error logging for TimerManager with stream expression
  * @param stream_expr Stream expression for formatted error output
  * 
  * Prints error message using stream operations unconditionally.
  * Supports complex formatted error messages with variables and context.
  * 
  * @code{.cpp}
  * TIMER_ERROR_STREAM("Timer " << id << " failed with errno " << errno);
  * @endcode
  */
 #define TIMER_ERROR_STREAM(stream_expr) \
     std::cout << "[TimerManager ERROR] " << stream_expr << std::endl
 
 /**
  * @def TIMER_INFO(msg)
  * @brief Always-on info logging for TimerManager lifecycle events
  * @param msg String literal or expression for info message
  * 
  * Prints informational message with [TimerManager] prefix unconditionally.
  * Used for important lifecycle events that should be logged in production
  * (e.g., initialization, shutdown, significant state changes).
  * 
  * @code{.cpp}
  * TIMER_INFO("TimerManager initialized with epoll support");
  * @endcode
  * 
  * @note Always enabled - use for important but non-error events
  * @note Balance between informativeness and log volume
  */
 #define TIMER_INFO(msg) \
     std::cout << "[TimerManager] " << msg << std::endl
 
 /**
  * @def TIMER_INFO_STREAM(stream_expr)
  * @brief Always-on info logging for TimerManager with stream expression
  * @param stream_expr Stream expression for formatted info output
  * 
  * Prints informational message using stream operations unconditionally.
  * Supports formatted output for detailed lifecycle information.
  * 
  * @code{.cpp}
  * TIMER_INFO_STREAM("Active timers: " << activeCount << ", capacity: " << maxTimers);
  * @endcode
  */
 #define TIMER_INFO_STREAM(stream_expr) \
     std::cout << "[TimerManager] " << stream_expr << std::endl
 
 // ========== SLLOOPER DEBUG MACROS ==========
 
 /**
  * @def SLLOOPER_DEBUG(msg)
  * @brief Conditional debug logging for SLLooper operations
  * @param msg String literal or expression for debug message
  * 
  * Prints debug message with [SLLooper] prefix only if SLLOOPER_DEBUG_ENABLED
  * is true. Useful for debugging event loop behavior and task execution.
  * 
  * @code{.cpp}
  * SLLOOPER_DEBUG("Starting event loop thread");
  * @endcode
  * 
  * @note Zero runtime cost when SLLOOPER_DEBUG_ENABLED is 0
  * @note Can be very verbose during active event processing
  */
 #define SLLOOPER_DEBUG(msg) \
     do { \
         if (SLLOOPER_DEBUG_ENABLED) { \
             std::cout << "[SLLooper] " << msg << std::endl; \
         } \
     } while(0)
 
 /**
  * @def SLLOOPER_DEBUG_STREAM(stream_expr)
  * @brief Conditional debug logging for SLLooper with stream expression
  * @param stream_expr Stream expression for formatted debug output
  * 
  * Prints debug message using stream operations only if SLLOOPER_DEBUG_ENABLED
  * is true. Allows detailed formatting of debug information.
  * 
  * @code{.cpp}
  * SLLOOPER_DEBUG_STREAM("Posted task " << taskId << " to queue, size: " << queueSize);
  * @endcode
  */
 #define SLLOOPER_DEBUG_STREAM(stream_expr) \
     do { \
         if (SLLOOPER_DEBUG_ENABLED) { \
             std::cout << "[SLLooper] " << stream_expr << std::endl; \
         } \
     } while(0)
 
 /**
  * @def SLLOOPER_ERROR(msg)
  * @brief Always-on error logging for SLLooper critical issues
  * @param msg String literal or expression for error message
  * 
  * Prints error message with [SLLooper ERROR] prefix unconditionally.
  * Used for critical errors in event loop operation that should always
  * be logged (e.g., thread creation failures, queue corruption).
  * 
  * @code{.cpp}
  * SLLOOPER_ERROR("Failed to start event loop thread");
  * @endcode
  */
 #define SLLOOPER_ERROR(msg) \
     std::cout << "[SLLooper ERROR] " << msg << std::endl
 
 /**
  * @def SLLOOPER_ERROR_STREAM(stream_expr)
  * @brief Always-on error logging for SLLooper with stream expression
  * @param stream_expr Stream expression for formatted error output
  */
 #define SLLOOPER_ERROR_STREAM(stream_expr) \
     std::cout << "[SLLooper ERROR] " << stream_expr << std::endl
 
 /**
  * @def SLLOOPER_INFO(msg)
  * @brief Always-on info logging for SLLooper lifecycle events
  * @param msg String literal or expression for info message
  * 
  * Prints informational message with [SLLooper] prefix unconditionally.
  * Used for important event loop lifecycle events and state transitions.
  * 
  * @code{.cpp}
  * SLLOOPER_INFO("Event loop started successfully");
  * @endcode
  */
 #define SLLOOPER_INFO(msg) \
     std::cout << "[SLLooper] " << msg << std::endl
 
 /**
  * @def SLLOOPER_INFO_STREAM(stream_expr)
  * @brief Always-on info logging for SLLooper with stream expression
  * @param stream_expr Stream expression for formatted info output
  */
 #define SLLOOPER_INFO_STREAM(stream_expr) \
     std::cout << "[SLLooper] " << stream_expr << std::endl
 
 // ========== EVENT QUEUE DEBUG MACROS ==========
 
 /**
  * @def EventQueue_DEBUG(msg)
  * @brief Conditional debug logging for EventQueue operations
  * @param msg String literal or expression for debug message
  * 
  * Prints debug message with [EventQueue] prefix only if EventQueue_DEBUG_ENABLED
  * is true. Useful for debugging message queuing and function execution.
  * 
  * @code{.cpp}
  * EventQueue_DEBUG("Enqueuing function for immediate execution");
  * @endcode
  * 
  * @note Can be very high-frequency during active queue processing
  * @note Zero runtime cost when EventQueue_DEBUG_ENABLED is 0
  */
 #define EventQueue_DEBUG(msg) \
     do { \
         if (EventQueue_DEBUG_ENABLED) { \
             std::cout << "[EventQueue] " << msg << std::endl; \
         } \
     } while(0)
 
 /**
  * @def EventQueue_DEBUG_STREAM(stream_expr)
  * @brief Conditional debug logging for EventQueue with stream expression
  * @param stream_expr Stream expression for formatted debug output
  * 
  * Prints debug message using stream operations only if EventQueue_DEBUG_ENABLED
  * is true. Supports detailed queue state and timing information.
  * 
  * @code{.cpp}
  * EventQueue_DEBUG_STREAM("Queue size: " << size << ", next execution: " << nextTime);
  * @endcode
  */
 #define EventQueue_DEBUG_STREAM(stream_expr) \
     do { \
         if (EventQueue_DEBUG_ENABLED) { \
             std::cout << "[EventQueue] " << stream_expr << std::endl; \
         } \
     } while(0)
 
 /**
  * @def EventQueue_ERROR(msg)
  * @brief Always-on error logging for EventQueue critical issues
  * @param msg String literal or expression for error message
  * 
  * Prints error message with [EventQueue ERROR] prefix unconditionally.
  * Used for critical queue operation errors that should always be logged.
  * 
  * @code{.cpp}
  * EventQueue_ERROR("Failed to enqueue message: out of memory");
  * @endcode
  */
 #define EventQueue_ERROR(msg) \
     std::cout << "[EventQueue ERROR] " << msg << std::endl
 
 /**
  * @def EventQueue_ERROR_STREAM(stream_expr)
  * @brief Always-on error logging for EventQueue with stream expression
  * @param stream_expr Stream expression for formatted error output
  */
 #define EventQueue_ERROR_STREAM(stream_expr) \
     std::cout << "[EventQueue ERROR] " << stream_expr << std::endl
 
 /**
  * @def EventQueue_INFO(msg)
  * @brief Always-on info logging for EventQueue lifecycle events
  * @param msg String literal or expression for info message
  * 
  * Prints informational message with [EventQueue] prefix unconditionally.
  * Used for important queue lifecycle events and significant state changes.
  * 
  * @code{.cpp}
  * EventQueue_INFO("EventQueue initialized successfully");
  * @endcode
  */
 #define EventQueue_INFO(msg) \
     std::cout << "[EventQueue] " << msg << std::endl
 
 /**
  * @def EventQueue_INFO_STREAM(stream_expr)
  * @brief Always-on info logging for EventQueue with stream expression
  * @param stream_expr Stream expression for formatted info output
  * 
  * @code{.cpp}
  * EventQueue_INFO_STREAM("Queue statistics - processed: " << processed << ", pending: " << pending);
  * @endcode
  */
 #define EventQueue_INFO_STREAM(stream_expr) \
    do { \
        std::cout << "[EventQueue] " << stream_expr << std::endl; \
    } while(0)
    