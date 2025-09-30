#ifndef HANDLER_H
#define HANDLER_H

#include <memory>
#include <cstdint>
#include "Message.h"
#include "EventQueue.h"
#include "SLLooper.h"

namespace swt {

/**
 * @class Handler
 * @brief Android-style message handler for event-driven programming
 *
 * Handler provides a flexible mechanism for processing messages and events
 * in the context of an event loop (\ref swt::SLLooper "SLLooper"). It enables
 * asynchronous communication between threads and decouples message producers
 * from consumers. Each Handler is associated with a specific event loop and
 * message queue (\ref swt::EventQueue "EventQueue").
 *
 * Key features:
 * - **Message creation**: Multiple overloads of obtainMessage for flexible message construction
 * - **Message sending**: Immediate, delayed, and timed message posting
 * - **Message management**: Query and remove messages by type or object
 * - **Custom processing**: Override handleMessage() for custom logic
 * - **Thread safety**: Safe for use from multiple threads
 *
 * @code{.cpp}
 * class MyHandler : public Handler {
 * public:
 *     void handleMessage(const std::shared_ptr<Message>& msg) override {
 *         // Custom message processing
 *     }
 * };
 *
 * auto looper = std::make_shared<SLLooper>();
 * auto handler = std::make_shared<MyHandler>(looper);
 * auto msg = handler->obtainMessage(1, 42, 0);
 * handler->sendMessage(msg);
 * @endcode
 *
 * @see \ref swt::Message "Message", \ref swt::SLLooper "SLLooper", \ref swt::EventQueue "EventQueue"
 */
class Handler : public std::enable_shared_from_this<Handler>  
{
public:
    /**
     * @brief Default constructor - not associated with any looper
     */
    Handler();

    /**
     * @brief Constructor with looper association
     * @param looper Shared pointer to SLLooper for event loop context
     */
    Handler(std::shared_ptr<SLLooper>& looper);

    /**
     * @brief Virtual destructor
     */
    virtual ~Handler();

    /**
     * @brief Obtain a new empty message
     * @return Shared pointer to new Message
     * @see \ref swt::Message "Message"
     */
    std::shared_ptr<Message> obtainMessage();

    /**
     * @brief Obtain a message with type code
     * @param what Message type code
     * @return Shared pointer to new Message
     * @see \ref swt::Message "Message"
     */
    std::shared_ptr<Message> obtainMessage(int32_t what);

    /**
     * @brief Obtain a message with type code and argument
     * @param what Message type code
     * @param arg1 First argument
     * @return Shared pointer to new Message
     */
    std::shared_ptr<Message> obtainMessage(int32_t what, int32_t arg1);

    /**
     * @brief Obtain a message with type code and object pointer
     * @param what Message type code
     * @param obj Object pointer
     * @return Shared pointer to new Message
     */
    std::shared_ptr<Message> obtainMessage(int32_t what, void* obj);

    /**
     * @brief Obtain a message with type code and two arguments
     * @param what Message type code
     * @param arg1 First argument
     * @param arg2 Second argument
     * @return Shared pointer to new Message
     */
    std::shared_ptr<Message> obtainMessage(int32_t what, int32_t arg1, int32_t arg2);

    /**
     * @brief Obtain a message with type code, two arguments, and object pointer
     * @param what Message type code
     * @param arg1 First argument
     * @param arg2 Second argument
     * @param obj Object pointer
     * @return Shared pointer to new Message
     */
    std::shared_ptr<Message> obtainMessage(int32_t what, int32_t arg1, int32_t arg2, void* obj);

    /**
     * @brief Obtain a message with type code and three arguments
     * @param what Message type code
     * @param arg1 First argument
     * @param arg2 Second argument
     * @param arg3 Third argument
     * @return Shared pointer to new Message
     */
    std::shared_ptr<Message> obtainMessage(int32_t what, int32_t arg1, int32_t arg2, int32_t arg3);

    /**
     * @brief Obtain a message with type code and shared pointer reference
     * @param what Message type code
     * @param spRef Shared pointer reference
     * @return Shared pointer to new Message
     */
    std::shared_ptr<Message> obtainMessage(int32_t what, std::shared_ptr<RefBase> spRef);

    /**
     * @brief Send message for immediate processing
     * @param message Message to send
     * @return true if message was successfully sent
     * @see \ref swt::Message "Message"
     */
    bool sendMessage(const std::shared_ptr<Message>& message);

    /**
     * @brief Send message with delay
     * @param message Message to send
     * @param delayMs Delay in milliseconds
     * @return true if message was successfully scheduled
     */
    bool sendMessageDelayed(const std::shared_ptr<Message>& message, int64_t delayMs);

    /**
     * @brief Send message at specific time
     * @param message Message to send
     * @param whenUs Absolute time in microseconds
     * @return true if message was successfully scheduled
     */
    bool sendMessageAtTime(const std::shared_ptr<Message>& message, int64_t whenUs);

    /**
     * @brief Check if message with type code exists in queue
     * @param what Message type code
     * @return true if message exists
     */
    bool hasMessages(int32_t what);

    /**
     * @brief Remove messages with type code from queue
     * @param what Message type code
     * @return true if any messages were removed
     */
    bool removeMessages(int32_t what);

    /**
     * @brief Remove messages with type code and object pointer from queue
     * @param what Message type code
     * @param obj Object pointer
     * @return true if any messages were removed
     */
    bool removeMessages(int32_t what, void* obj);

    /**
     * @brief Dispatch message to handler (internal use)
     * @param message Message to dispatch
     * @see \ref swt::Handler::handleMessage "handleMessage()"
     */
    void dispatchMessage(const std::shared_ptr<Message>& message);

    /**
     * @brief Get current system uptime in microseconds
     * @return Current uptime in microseconds
     */
    int64_t uptimeMicros();

public:
    /**
     * @brief Override to handle received messages
     * @param msg Message to process
     * @see \ref swt::Message "Message"
     */
    virtual void handleMessage(const std::shared_ptr<Message>& msg) = 0;

private:
    std::shared_ptr<EventQueue> mEventQueue;  /**< Associated event queue */
    std::shared_ptr<SLLooper> mLooper;        /**< Associated event loop */
};

} // namespace swt

#endif // HANDLER_H