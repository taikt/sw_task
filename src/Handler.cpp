/**
 * @file Handler.cpp
 * @brief Implementation of Handler - message-based communication in event loop
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #include <assert.h>
 #include <chrono>
 #include "Handler.h"
 #include <iostream>
 #include <string>
 
 using namespace std;
 
 /**
  * @def ms2us(x)
  * @brief Convert milliseconds to microseconds
  * @param x Time in milliseconds
  * @return Time in microseconds
  * 
  * Utility macro for time unit conversion used in message timing.
  */
 #define ms2us(x) (x*1000LL)
 
 /**
  * @brief Constructor - associates handler with event loop
  * @param looper SLLooper instance for message queue access
  * 
  * Initializes the handler with a reference to the event loop's message queue.
  * The handler will post messages to this queue and receive callbacks when
  * messages are processed in the event loop thread.
  * 
  * Key initialization:
  * - **EventQueue access**: Gets queue from looper for message operations
  * - **Looper reference**: Maintains weak reference to prevent circular deps
  * - **Thread context**: Handler callbacks execute in looper's thread context
  * 
  * @note Handler must be created as shared_ptr due to enable_shared_from_this
  * @note All message operations will use this looper's event queue
  */
 Handler::Handler(std::shared_ptr<SLLooper>& looper)
 {
     mEventQueue = looper->getEventQueue();  // ✅ Updated method name
     mLooper = looper;
 }
 
 /**
  * @brief Destructor - cleanup handler resources
  * 
  * Performs cleanup of handler resources. In a complete implementation,
  * this would remove any pending messages associated with this handler
  * from the event queue.
  * 
  * @note Currently no explicit cleanup needed due to shared_ptr management
  */
 Handler::~Handler()
 {
     // Future: Remove pending messages from queue
     // mEventQueue->removeMessagesForHandler(shared_from_this());
 }
 
 // ========== MESSAGE CREATION API ==========
 
 /**
  * @brief Create a basic message with this handler as target
  * @return std::shared_ptr<Message> New message object
  * 
  * Creates a message that will be delivered to this handler's handleMessage()
  * when processed by the event loop. This is the most basic message creation.
  * 
  * @code{.cpp}
  * auto message = handler->obtainMessage();
  * handler->sendMessage(message);
  * @endcode
  * 
  * @note Uses shared_from_this() - handler must be created as shared_ptr
  * @see Message::obtain(), sendMessage()
  */
 std::shared_ptr<Message> Handler::obtainMessage()
 {
     return Message::obtain(shared_from_this()); // ✅ SAFE!
 }
 
 /**
  * @brief Create message with 'what' identifier
  * @param what Message type identifier
  * @return std::shared_ptr<Message> New message object
  * 
  * Creates a message with a specific type identifier. The 'what' field
  * allows handlers to distinguish between different message types.
  * 
  * @code{.cpp}
  * enum MessageTypes { UPDATE_UI = 1, PROCESS_DATA = 2 };
  * auto message = handler->obtainMessage(UPDATE_UI);
  * @endcode
  */
 std::shared_ptr<Message> Handler::obtainMessage(int32_t what)
 {
     return Message::obtain(shared_from_this(), what); // ✅ SAFE!
 }
 
 /**
  * @brief Create message with 'what' and first argument
  * @param what Message type identifier
  * @param arg1 First integer argument
  * @return std::shared_ptr<Message> New message object
  * 
  * Creates a message with type and one integer argument. Useful for
  * simple parameter passing in message-based communication.
  */
 std::shared_ptr<Message> Handler::obtainMessage(int32_t what, int32_t arg1)
 {
     return Message::obtain(shared_from_this(), what, arg1); // ✅ SAFE!
 }
 
 /**
  * @brief Create message with 'what' and object pointer
  * @param what Message type identifier  
  * @param obj Pointer to arbitrary object data
  * @return std::shared_ptr<Message> New message object
  * 
  * Creates a message with type and object pointer. The object lifetime
  * must be managed carefully to ensure it remains valid when message
  * is processed.
  * 
  * @warning Object pointer lifetime must exceed message processing time
  */
 std::shared_ptr<Message> Handler::obtainMessage(int32_t what, void* obj)
 {
     return Message::obtain(shared_from_this(), what, obj); // ✅ SAFE!
 }
 
 /**
  * @brief Create message with 'what' and two arguments
  * @param what Message type identifier
  * @param arg1 First integer argument
  * @param arg2 Second integer argument
  * @return std::shared_ptr<Message> New message object
  * 
  * Creates a message with type and two integer arguments. Common pattern
  * for coordinate pairs, dimensions, or simple parameter combinations.
  * 
  * @code{.cpp}
  * // Send window resize message
  * auto message = handler->obtainMessage(RESIZE_WINDOW, width, height);
  * @endcode
  */
 std::shared_ptr<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2)
 {
     return Message::obtain(shared_from_this(), what, arg1, arg2); // ✅ SAFE!
 }
 
 /**
  * @brief Create message with 'what', two arguments, and object
  * @param what Message type identifier
  * @param arg1 First integer argument
  * @param arg2 Second integer argument
  * @param obj Pointer to arbitrary object data
  * @return std::shared_ptr<Message> New message object
  * 
  * Creates a message with full parameter set: type, two integers, and object.
  * Most comprehensive message creation method for complex scenarios.
  */
 std::shared_ptr<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2, void* obj)
 {
     return Message::obtain(shared_from_this(), what, arg1, arg2, obj); // ✅ SAFE!
 }
 
 /**
  * @brief Create message with 'what' and three arguments
  * @param what Message type identifier
  * @param arg1 First integer argument
  * @param arg2 Second integer argument
  * @param arg3 Third integer argument
  * @return std::shared_ptr<Message> New message object
  * 
  * Extended message creation with three integer arguments for scenarios
  * requiring additional parameter data.
  */
 std::shared_ptr<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2, int32_t arg3)
 {
     return Message::obtain(shared_from_this(), what, arg1, arg2, arg3); // ✅ SAFE!
 }
 
 /**
  * @brief Create message with 'what' and shared reference object
  * @param what Message type identifier
  * @param spRef Shared pointer to reference-counted object
  * @return std::shared_ptr<Message> New message object
  * 
  * Creates a message with a shared reference object. Preferred over raw
  * pointers for automatic lifetime management and memory safety.
  * 
  * @code{.cpp}
  * auto data = std::make_shared<MyData>();
  * auto message = handler->obtainMessage(PROCESS_DATA, data);
  * @endcode
  * 
  * @note Shared pointer ensures object lifetime during message processing
  */
 std::shared_ptr<Message> Handler::obtainMessage(int32_t what, std::shared_ptr<RefBase> spRef)
 {
     return Message::obtain(shared_from_this(), what, spRef); // ✅ SAFE!
 }
 
 // ========== MESSAGE SENDING API ==========
 
 /**
  * @brief Send message immediately to event queue
  * @param message Message to send
  * @return true if message was successfully queued
  * 
  * Posts message to the event queue for immediate processing. The message
  * will be processed as soon as the event loop reaches it in the queue.
  * 
  * @code{.cpp}
  * auto message = handler->obtainMessage(UPDATE_STATUS);
  * if (!handler->sendMessage(message)) {
  *     // Handle queue failure
  * }
  * @endcode
  * 
  * @note Message processed in event loop thread context
  * @note Returns immediately - message processed asynchronously
  */
 bool Handler::sendMessage(const std::shared_ptr<Message>& message)
 {
     return sendMessageAtTime(message, uptimeMicros());
 }
 
 /**
  * @brief Send message with delay
  * @param message Message to send
  * @param delayMs Delay in milliseconds before processing
  * @return true if message was successfully queued
  * 
  * Posts message to the event queue with a specified delay. The message
  * will be processed after the delay expires.
  * 
  * @code{.cpp}
  * // Send timeout message after 5 seconds
  * auto timeoutMsg = handler->obtainMessage(TIMEOUT);
  * handler->sendMessageDelayed(timeoutMsg, 5000);
  * @endcode
  * 
  * @note Delay measured from current time
  * @note Message can be cancelled before processing if needed
  */
 bool Handler::sendMessageDelayed(const std::shared_ptr<Message>& message, int64_t delayMs)
 {
     return sendMessageAtTime(message, (uptimeMicros() + ms2us(delayMs)));
 }
 
 /**
  * @brief Send message at specific time
  * @param message Message to send
  * @param whenUs Absolute time in microseconds when to process message
  * @return true if message was successfully queued
  * 
  * Posts message to be processed at a specific absolute time. Uses the
  * internal microsecond timestamp format for precise timing control.
  * 
  * @note Time specified in microseconds since system start
  * @note Lower-level method used by other send methods
  */
 bool Handler::sendMessageAtTime(const std::shared_ptr<Message>& message, int64_t whenUs)
 {
     return mEventQueue->enqueueMessage(message, whenUs);
 }
 
 // ========== MESSAGE QUERY API ==========
 
 /**
  * @brief Check if handler has pending messages of specific type
  * @param what Message type identifier to check
  * @return true if messages of this type are pending
  * 
  * Queries the event queue for pending messages with the specified 'what'
  * value that are targeted to this handler.
  * 
  * @code{.cpp}
  * if (handler->hasMessages(UPDATE_UI)) {
  *     // UI update already pending, skip duplicate
  * }
  * @endcode
  * 
  * @note Only checks messages targeted to this handler
  * @note Useful for avoiding duplicate message posting
  */
 bool Handler::hasMessages(int32_t what)
 {
     return mEventQueue->hasMessage(shared_from_this(), what, NULL); // ✅ SAFE!
 }
 
 /**
  * @brief Remove pending messages of specific type
  * @param what Message type identifier to remove
  * @return true if messages were removed
  * 
  * Removes all pending messages of the specified type that are targeted
  * to this handler from the event queue.
  * 
  * @note Currently not implemented - returns false
  * @todo Implement message removal functionality
  */
 bool Handler::removeMessages(int32_t what)
 {
     // TODO: Implement message removal
     // return mEventQueue->removeMessages(shared_from_this(), what);
     return false;
 }
 
 /**
  * @brief Remove pending messages of specific type and object
  * @param what Message type identifier to remove
  * @param obj Object pointer to match for removal
  * @return true if messages were removed
  * 
  * Removes pending messages that match both the 'what' type and object
  * pointer, providing more selective message removal.
  * 
  * @note Currently not implemented - returns false
  * @todo Implement selective message removal functionality
  */
 bool Handler::removeMessages(int32_t what, void* obj)
 {
     // TODO: Implement selective message removal
     // return mEventQueue->removeMessages(shared_from_this(), what, obj);
     return false;
 }
 
 // ========== MESSAGE PROCESSING ==========
 
 /**
  * @brief Dispatch message to handler for processing
  * @param message Message to process
  * 
  * Called by the event loop when a message targeted to this handler
  * is ready for processing. Delegates to the virtual handleMessage()
  * method for actual message handling.
  * 
  * This method provides an extension point for message preprocessing
  * or logging before actual message handling occurs.
  * 
  * @note Called in event loop thread context
  * @note Override handleMessage() for custom message processing
  */
 void Handler::dispatchMessage(const std::shared_ptr<Message>& message)
 {
     // Future: Add message preprocessing here
     // logMessage(message);
     // validateMessage(message);
     
     handleMessage(message);
 }
 
 // ========== UTILITY METHODS ==========
 
 /**
  * @brief Get current system uptime in microseconds
  * @return Current uptime in microseconds
  * 
  * Provides monotonic time measurement for message timing and scheduling.
  * Uses steady_clock to avoid issues with system clock adjustments.
  * 
  * Key characteristics:
  * - **Monotonic**: Always increases, unaffected by system clock changes
  * - **Microsecond precision**: High resolution for accurate message timing
  * - **System uptime**: Measures time since system/process start
  * - **Thread-safe**: std::chrono functions are thread-safe
  * 
  * Used internally for:
  * - Message scheduling timestamps
  * - Delay calculations
  * - Timer management
  * - Performance measurements
  * 
  * @note Returns steady_clock time, not wall clock time
  * @note Microsecond resolution may be limited by system capabilities
  */
 int64_t Handler::uptimeMicros()
 {
     auto now = std::chrono::steady_clock::now();
     auto dur = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
     return dur.count();
 }