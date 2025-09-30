/**
 * @file EventQueue.h
 * @brief Unified event queue supporting messages and functions with timed execution
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once

 #include <iostream>
 #include <mutex>
 #include <thread>
 #include <condition_variable>
 #include <functional>
 #include <future>
 #include <deque>
 #include <optional>
 #include <algorithm>
 #include <variant>
 #include "Message.h"
 
 // Forward declaration
 namespace swt {
     template<typename T> class Promise;
 }
 
 namespace swt {
 /**
  * @class EventQueue
  * @brief Thread-safe unified queue for messages and function tasks with timed execution
  * 
  * EventQueue provides a unified interface for queuing both traditional messages
  * and modern function tasks with support for immediate and delayed execution.
  * Key features:
  * 
  * - **Unified queue**: Single deque handling both messages and function tasks
  * - **Timed execution**: Support for delayed execution with microsecond precision
  * - **Thread safety**: Full thread-safe operations with mutex protection
  * - **Type safety**: Template-based function enqueuing with automatic type deduction
  * - **Future support**: std::future integration for result retrieval
  * - **Promise integration**: Built-in promise creation and management
  * - **Efficient ordering**: Binary search insertion for delayed tasks
  * 
  * The queue maintains chronological order for timed items while supporting
  * immediate execution for urgent tasks. Uses type erasure to store different
  * function signatures uniformly while preserving type safety at the API level.
  * 
  * Architecture:
  * - **QueueItem**: Variant-like structure supporting messages and function tasks
  * - **Binary heap ordering**: Maintains execution order based on timestamps
  * - **Condition variable**: Efficient thread synchronization for polling
  * - **Move semantics**: Optimal performance for large function objects
  * 
  * @code{.cpp}
  * EventQueue queue;
  * 
  * // Enqueue immediate function
  * auto future1 = queue.enqueueFunction([](int x) { return x * 2; }, 21);
  * 
  * // Enqueue delayed function
  * auto future2 = queue.enqueueFunctionDelayed(1000, []() { 
  *     return "Hello after 1 second!"; 
  * });
  * 
  * // Traditional message support
  * auto msg = std::make_shared<Message>();
  * queue.enqueueMessage(msg, uptimeMicros() + 500000);  // 500ms delay
  * 
  * // Poll for next item
  * while (auto item = queue.pollNext()) {
  *     if (item->type == QueueItemType::FUNCTION) {
  *         item->task();  // Execute function
  *     } else {
  *         // Handle message
  *     }
  * }
  * @endcode
  * 
  * @note Thread-safe for concurrent producers and single consumer
  * @warning Consumer polling should be from single thread for message ordering
  * 
  * @see \ref swt::Message "Message", \ref swt::Handler "Handler", \ref swt::Promise "Promise"
  */
 class EventQueue
 {
 public:
     /**
      * @enum QueueItemType
      * @brief Type discriminator for queue items
      * 
      * Identifies whether a queue item contains a traditional message
      * or a modern function task for proper handling during polling.
      */
     enum class QueueItemType { 
         MESSAGE,    /**< Traditional message with handler */
         FUNCTION    /**< Function task with std::packaged_task */
     };
     
     /**
      * @struct QueueItem
      * @brief Unified queue item supporting both messages and function tasks
      * 
      * QueueItem provides a variant-like structure that can hold either
      * traditional messages or modern function tasks while maintaining
      * uniform timing and ordering semantics. Uses move-only semantics
      * for optimal performance with function objects.
      * 
      * Key design principles:
      * - **Type discrimination**: QueueItemType enum for runtime type safety
      * - **Unified timing**: int64_t whenUs for consistent execution scheduling
      * - **Move-only**: Prevents accidental copying of expensive function objects
      * - **Memory efficiency**: Uses shared_ptr for messages, move semantics for tasks
      */
     struct QueueItem {
         QueueItemType type;                          /**< Type discriminator for queue item */
         int64_t whenUs;                             /**< Execution time in microseconds (absolute) */
         
         std::shared_ptr<Message> message;           /**< Message payload (for MESSAGE type) */
         std::packaged_task<void()> task;           /**< Function task (for FUNCTION type) */
         
         /**
          * @brief Constructor for message queue items
          * @param msg Shared pointer to message object
          * @param when Absolute execution time in microseconds
          * 
          * Creates a queue item containing a traditional message for
          * handler-based processing with specified execution timing.
          */
         QueueItem(std::shared_ptr<Message> msg, int64_t when) 
             : type(QueueItemType::MESSAGE), whenUs(when), message(msg) {}
             
         /**
          * @brief Constructor for function queue items
          * @param t Packaged task to execute (moved)
          * @param when Absolute execution time in microseconds
          * 
          * Creates a queue item containing a function task for
          * direct execution with specified timing.
          */
         QueueItem(std::packaged_task<void()>&& t, int64_t when)
             : type(QueueItemType::FUNCTION), whenUs(when), task(std::move(t)) {}
         
         /**
          * @brief Move constructor - transfers ownership of queue item
          * @param other Queue item to move from
          * 
          * Efficiently transfers ownership of either message or task
          * without copying expensive function objects.
          */
         QueueItem(QueueItem&& other) noexcept
             : type(other.type), whenUs(other.whenUs), 
             message(std::move(other.message)), task(std::move(other.task)) {}
         
         /**
          * @brief Move assignment operator - transfers ownership
          * @param other Queue item to move from
          * @return Reference to this queue item
          * 
          * Handles self-assignment safely and transfers ownership
          * of contained message or task.
          */
         QueueItem& operator=(QueueItem&& other) noexcept {
             if (this != &other) {
                 type = other.type;
                 whenUs = other.whenUs;
                 message = std::move(other.message);
                 task = std::move(other.task);
             }
             return *this;
         }
         
         /**
          * @brief Copy constructor - deleted (move-only semantics)
          * 
          * Queue items cannot be copied to prevent expensive function
          * object duplication and ensure unique ownership.
          */
         QueueItem(const QueueItem&) = delete;
         
         /**
          * @brief Copy assignment - deleted (move-only semantics)
          * 
          * Queue items cannot be copy-assigned to maintain move-only
          * semantics and prevent accidental expensive copies.
          */
         QueueItem& operator=(const QueueItem&) = delete;
     };
 
 public:
     /**
      * @brief Constructor - initializes empty queue
      * 
      * Creates an empty event queue with initialized mutex and
      * condition variable for thread-safe operations.
      */
     EventQueue();
     
     /**
      * @brief Destructor - cleanup remaining items
      * 
      * Cleans up any remaining messages and function tasks in the queue.
      * Note that pending futures may become invalid after destruction.
      */
     ~EventQueue();
     
     // ========== LEGACY MESSAGE API ==========
     
     /**
      * @brief Enqueue traditional message for timed execution
      * @param message Shared pointer to message object
      * @param whenUs Absolute execution time in microseconds
      * @return true if message was successfully enqueued
      * 
      * Enqueues a traditional message for execution at the specified time.
      * Messages are inserted in chronological order using binary search
      * for efficient ordering without full sorting.
      * 
      * @code{.cpp}
      * auto msg = std::make_shared<Message>();
      * msg->what = MSG_TIMEOUT;
      * queue.enqueueMessage(msg, uptimeMicros() + 1000000);  // 1 second delay
      * @endcode
      * 
      * @note Thread-safe operation
      * @note Uses binary search for O(log n) insertion
      * @see \ref swt::Message "Message"
      */
     bool enqueueMessage(const std::shared_ptr<Message>& message, int64_t whenUs);
     
     /**
      * @brief Poll for next ready message (legacy compatibility)
      * @return Shared pointer to next ready message, or nullptr if none
      * 
      * Legacy method that polls only for messages, ignoring function tasks.
      * Provided for backward compatibility with existing message-based code.
      * 
      * @note Prefer pollNext() for unified polling
      * @note Only returns MESSAGE type items
      * @see \ref swt::Message "Message"
      */
     std::shared_ptr<Message> poll();
 
     /**
      * @brief Check if quit message was received
      * @return true if quit message is pending
      * 
      * Legacy method for checking quit state in message-based loops.
      */
     bool isQuit();
 
     // ========== MODERN FUNCTION API ==========
 
     /**
      * @brief Enqueue function for immediate asynchronous execution
      * @tparam F Function type (auto-deduced)
      * @tparam Args Variadic argument types (auto-deduced)
      * @param func Callable object to execute asynchronously
      * @param args Arguments to forward to the function
      * @return std::future<ReturnType> Future containing the function result
      * 
      * Enqueues a function for immediate execution at the front of the queue.
      * Uses perfect forwarding to preserve argument types and std::packaged_task
      * for type-safe result retrieval through futures.
      * 
      * @note Implementation in EventQueue.tpp
      * @note Thread-safe operation with immediate notification
      * @see \ref swt::Promise "Promise"
      */
     template<typename F, typename... Args>
     auto enqueueFunction(F&& func, Args&&... args) -> std::future<decltype(func(args...))>;
     
     /**
      * @brief Enqueue function for delayed asynchronous execution
      * @tparam F Function type (auto-deduced)
      * @tparam Args Variadic argument types (auto-deduced)
      * @param delayMs Delay in milliseconds before execution
      * @param func Callable object to execute asynchronously
      * @param args Arguments to forward to the function
      * @return std::future<ReturnType> Future containing the function result
      * 
      * Enqueues a function for delayed execution, maintaining chronological
      * order with other timed items. Uses binary search insertion for
      * efficient ordering without full queue sorting.
      * 
      * @note Implementation in EventQueue.tpp
      * @note Thread-safe operation with conditional notification
      * @see \ref swt::Promise "Promise"
      */
     template<typename F, typename... Args>  
     auto enqueueFunctionDelayed(int64_t delayMs, F&& func, Args&&... args) -> std::future<decltype(func(args...))>;
     
     // ========== PROMISE INTEGRATION ==========
 
     /**
      * @brief Create and enqueue promise for manual resolution
      * @tparam T Value type for the promise
      * @return swt::Promise<T> New promise object for manual control
      * 
      * Creates a new promise that can be resolved manually from any thread.
      * The promise callbacks will execute in the event loop thread when
      * the promise is resolved or rejected.
      * 
      * @code{.cpp}
      * auto promise = queue.enqueuePromise<int>();
      * 
      * // Set up continuation in event loop thread
      * promise.then(looper, [](int result) {
      *     std::cout << "Promise resolved: " << result << std::endl;
      * });
      * 
      * // Resolve from any thread
      * std::thread([promise]() mutable {
      *     std::this_thread::sleep_for(1s);
      *     promise.set_value(42);
      * }).detach();
      * @endcode
      * 
      * @note Promise callbacks execute in event loop thread
      * @note Thread-safe promise resolution from any thread
      * @see \ref swt::Promise "Promise"
      */
     template<typename T>
     Promise<T> enqueuePromise();
     
     // ========== UNIFIED POLLING ==========
 
     /**
      * @brief Poll for next ready queue item (unified interface)
      * @return std::optional<QueueItem> Next ready item, or nullopt if none
      * 
      * Unified polling method that returns the next ready item regardless
      * of type (message or function). Checks timing and only returns items
      * whose execution time has arrived. Blocks until an item is ready
      * or the queue is quit.
      * 
      * @code{.cpp}
      * while (auto item = queue.pollNext()) {
      *     switch (item->type) {
      *         case QueueItemType::MESSAGE:
      *             handleMessage(item->message);
      *             break;
      *         case QueueItemType::FUNCTION:
      *             item->task();  // Execute function
      *             break;
      *     }
      * }
      * @endcode
      * 
      * @note Blocks until item is ready or quit is called
      * @note Thread-safe for single consumer
      * @note Preferred method for modern event loop implementations
      * @see \ref swt::Message "Message"
      */
     std::optional<QueueItem> pollNext();
     
     // ========== QUEUE CONTROL ==========
 
     /**
      * @brief Request queue shutdown
      * 
      * Signals the queue to stop processing and causes polling methods
      * to return null/empty to exit event loop gracefully.
      */
     void quit();
     
     /**
      * @brief Check for specific message in queue
      * @param h Handler to match
      * @param what Message type to match
      * @param obj Object pointer to match
      * @return true if matching message found
      * 
      * Legacy method for checking if a specific message type is queued.
      * Used for message deduplication and state checking.
      * @see \ref swt::Handler "Handler"
      */
     bool hasMessage(const std::shared_ptr<Handler>& h, int32_t what, void* obj);
     
     /**
      * @brief Get current system uptime in microseconds
      * @return int64_t Current uptime in microseconds
      * 
      * Utility method providing high-precision timing for queue scheduling.
      * Uses steady_clock to avoid issues with system time adjustments.
      * 
      * @note Uses std::chrono::steady_clock for monotonic timing
      * @note Microsecond precision for accurate scheduling
      */
     int64_t uptimeMicros();
 
 private:
     std::deque<QueueItem> mQueue;                    /**< Unified queue storing messages and function tasks */
     std::shared_ptr<Message> mCurrentMessage;       /**< Current message for legacy compatibility */
     std::mutex iMutex;                              /**< Mutex protecting queue operations */
     bool mStarted;                                  /**< Flag indicating if queue processing started */
     mutable bool mQuit;                             /**< Atomic quit flag for graceful shutdown */
     std::condition_variable mQueueChanged;          /**< Condition variable for efficient polling */
 };
 }
 // Include template implementations
 #include "EventQueue.tpp"