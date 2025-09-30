#ifndef HANDLER_H
#define HANDLER_H

#include <memory>
#include <cstdint>
#include "Message.h"
#include "EventQueue.h"
#include "SLLooper.h"

namespace swt {

class Handler : public std::enable_shared_from_this<Handler>  
{
public:
    Handler();
    Handler(std::shared_ptr<SLLooper>& looper);
    virtual ~Handler();

    std::shared_ptr<Message> obtainMessage();
    std::shared_ptr<Message> obtainMessage(int32_t what);
    std::shared_ptr<Message> obtainMessage(int32_t what, int32_t arg1);
    std::shared_ptr<Message> obtainMessage(int32_t what, void* obj);
    std::shared_ptr<Message> obtainMessage(int32_t what, int32_t arg1, int32_t arg2);
    std::shared_ptr<Message> obtainMessage(int32_t what, int32_t arg1, int32_t arg2, void* obj);
    std::shared_ptr<Message> obtainMessage(int32_t what, int32_t arg1, int32_t arg2, int32_t arg3);
    std::shared_ptr<Message> obtainMessage(int32_t what, std::shared_ptr<RefBase> spRef);

    bool sendMessage(const std::shared_ptr<Message>& message);
    bool sendMessageDelayed(const std::shared_ptr<Message>& message, int64_t delayMs);
    bool sendMessageAtTime(const std::shared_ptr<Message>& message, int64_t whenUs);

    bool hasMessages(int32_t what);
    bool removeMessages(int32_t what);
    bool removeMessages(int32_t what, void* obj);

    void dispatchMessage(const std::shared_ptr<Message>& message);

    int64_t uptimeMicros();

public:
    virtual void handleMessage(const std::shared_ptr<Message>& msg) = 0;

private:
    std::shared_ptr<EventQueue> mEventQueue;
    std::shared_ptr<SLLooper> mLooper;
};

#endif // HANDLER_H
} // namespace swt