#ifndef MESSAGE_H
#define MESSAGE_H

#include <memory>
#include <cstdint>
#include "Refbase.h"

namespace swt {

class Handler;

/**
 * @class Message
 * @brief Message object for event-driven communication between threads and handlers
 *
 * Message encapsulates data and metadata for asynchronous communication in the event loop
 * framework. It supports flexible construction, deep copy, and safe transfer between threads.
 * Each Message can be associated with a \ref swt::Handler "Handler" and posted to an
 * event queue (\ref swt::EventQueue "EventQueue") for timed or immediate processing.
 *
 * Key features:
 * - **Flexible construction**: Multiple static obtain() overloads for various use cases
 * - **Deep copy**: dup() and setTo() for safe message duplication
 * - **Object attachment**: Support for raw pointers and smart pointers (spRef)
 * - **Argument fields**: Four integer arguments for custom data
 * - **Safe handler association**: mHandler for target handler
 * - **Chaining**: mNextMessage for message queueing
 *
 * @code{.cpp}
 * auto msg = Message::obtain(handler, 1, 42, 0);
 * msg->obj = somePtr;
 * handler->sendMessage(msg);
 * @endcode
 *
 * @see \ref swt::Handler "Handler", \ref swt::EventQueue "EventQueue", \ref swt::SLLooper "SLLooper"
 */
class Message : public std::enable_shared_from_this<Message>
{
public:
    /**
     * @brief Default constructor - creates empty message
     */
    Message();

    /**
     * @brief Copy constructor - deep copy from another message
     * @param other Source message to copy from
     */
    Message(Message& other);

    /**
     * @brief Virtual destructor
     */
    virtual ~Message();

    // ========== STATIC FACTORY METHODS ==========

    /**
     * @brief Obtain a new empty message
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain();

    /**
     * @brief Obtain a copy of an existing message
     * @param message Pointer to source message
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain(const Message* message);

    /**
     * @brief Obtain a message associated with a handler
     * @param handler Target handler
     * @return Shared pointer to new Message
     * @see \ref swt::Handler "Handler"
     */
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler);

    /**
     * @brief Obtain a message with handler and type code
     * @param handler Target handler
     * @param obtain_what Message type code
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what);

    /**
     * @brief Obtain a message with handler, type code, and argument
     * @param handler Target handler
     * @param obtain_what Message type code
     * @param obtain_arg1 First argument
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 int32_t obtain_arg1);

    /**
     * @brief Obtain a message with handler, type code, and object pointer
     * @param handler Target handler
     * @param obtain_what Message type code
     * @param obtain_obj Object pointer
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 void* obtain_obj);

    /**
     * @brief Obtain a message with handler, type code, and two arguments
     * @param handler Target handler
     * @param obtain_what Message type code
     * @param obtain_arg1 First argument
     * @param obtain_arg2 Second argument
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 int32_t obtain_arg1, int32_t obtain_arg2);

    /**
     * @brief Obtain a message with handler, type code, two arguments, and object pointer
     * @param handler Target handler
     * @param obtain_what Message type code
     * @param obtain_arg1 First argument
     * @param obtain_arg2 Second argument
     * @param obtain_obj Object pointer
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 int32_t obtain_arg1, int32_t obtain_arg2, void* obtain_obj);

    /**
     * @brief Obtain a message with handler, type code, and three arguments
     * @param handler Target handler
     * @param obtain_what Message type code
     * @param obtain_arg1 First argument
     * @param obtain_arg2 Second argument
     * @param obtain_arg3 Third argument
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 int32_t obtain_arg1, int32_t obtain_arg2, int32_t obtain_arg3);

    /**
     * @brief Obtain a message with handler, type code, and smart pointer reference
     * @param handler Target handler
     * @param obtain_what Message type code
     * @param obtain_spRef Shared pointer reference
     * @return Shared pointer to new Message
     */
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 std::shared_ptr<RefBase> obtain_spRef);

    /*
    template<typename T>
    static std::shared_ptr<Message> obtain(const std::shared_ptr<Handler>& handler, int32_t obtain_what,
                                 std::shared_ptr<T> obtain_spRef);
    */

    /**
     * @brief Send this message to its target handler
     * @return true if message was successfully sent
     * @see \ref swt::Handler "Handler"
     */
    bool sendToTarget();

    /**
     * @brief Create a deep copy of this message
     * @return Shared pointer to duplicated Message
     */
    std::shared_ptr<Message> dup() const;

    /**
     * @brief Get attached object as shared pointer of type T
     * @tparam T Target type
     * @param obj Output shared pointer
     */
    template<typename T>
    void getObject(std::shared_ptr<T>& obj)
    {
        obj = static_cast<T*>( spRef.get() );
    }

protected:
    /**
     * @brief Assignment operator - deep copy from another message
     * @param other Source message
     * @return Reference to this message
     */
    Message& operator=(const Message& other);

    /**
     * @brief Clear all fields in the message
     */
    void clear();

public:
    /**
     * @brief Set this message to another message's contents
     * @param other Source message
     */
    void setTo(const Message& other);

    std::shared_ptr<Handler> mHandler;        /**< Target handler */
    std::shared_ptr<Message> mNextMessage;    /**< Next message in queue */

    int32_t what;     /**< Message type code */
    int32_t arg1;     /**< First argument */
    int32_t arg2;     /**< Second argument */
    int32_t arg3;     /**< Third argument */

    void* obj;        /**< Raw object pointer */
    ssize_t obj_size; /**< Size of object (if applicable) */
    std::shared_ptr<RefBase> spRef; /**< Smart pointer reference */

private:
    int64_t whenUs;   /**< Scheduled execution time (microseconds) */

    friend class Handler;
    friend class EventQueue;
    friend class SLLooper;
};

} // namespace swt

#endif // MESSAGE_H