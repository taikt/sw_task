#include <assert.h>
#include <chrono>
#include "Handler.h"
#include <iostream>
#include <string>
using namespace std;

#define ms2us(x) (x*1000LL)

/*
Handler::Handler()
{
    std::shared_ptr<SLLooper> looper = SLLooper::myLooper();
    assert(looper != NULL);
    mMessageQueue = looper->messageQueue();
}
*/
Handler::Handler(std::shared_ptr<SLLooper>& looper)
{
    mMessageQueue = looper->messageQueue();
    mLooper = looper;
}

Handler::~Handler()
{
}

std::shared_ptr<Message> Handler::obtainMessage()
{
    return Message::obtain(shared_from_this());
}

std::shared_ptr<Message> Handler::obtainMessage(int32_t what)
{
    return Message::obtain(shared_from_this(), what);
}

std::shared_ptr<Message> Handler::obtainMessage(int32_t what, int32_t arg1)
{
    return Message::obtain(shared_from_this(), what, arg1);
}

std::shared_ptr<Message> Handler::obtainMessage(int32_t what, void* obj)
{
    return Message::obtain(shared_from_this(), what, obj);
}

std::shared_ptr<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2)
{
    return Message::obtain(shared_from_this(), what, arg1, arg2);
}

std::shared_ptr<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2, void* obj)
{
    return Message::obtain(shared_from_this(), what, arg1, arg2, obj);
}

std::shared_ptr<Message> Handler::obtainMessage(int32_t what, int32_t arg1, int32_t arg2, int32_t arg3)
{
    return Message::obtain(shared_from_this(), what, arg1, arg2, arg3);
}

std::shared_ptr<Message> Handler::obtainMessage(int32_t what , std::shared_ptr<RefBase> spRef)
{
    return Message::obtain(shared_from_this(), what, spRef);
}

bool Handler::sendMessage(const std::shared_ptr<Message>& message)
{
    // cout<<"handler sendMessage\n";
    return sendMessageAtTime(message, uptimeMicros());
}

bool Handler::sendMessageDelayed(const std::shared_ptr<Message>& message, int64_t delayMs)
{
    return sendMessageAtTime(message, (uptimeMicros() + ms2us(delayMs)));
}

bool Handler::sendMessageAtTime(const std::shared_ptr<Message>& message, int64_t whenUs)
{
    //message->mHandler = this;
    //message->mHandler = std::shared_ptr<Handler>(this);

    return mMessageQueue->enqueueMessage(message, whenUs);
}

/*
bool Handler::hasMessages(int32_t what)
{
    return mMessageQueue->hasMessage(this, what, NULL);
}

bool Handler::removeMessages(int32_t what)
{
    return mMessageQueue->removeMessages(this, what, NULL);
}

bool Handler::removeMessages(int32_t what, void* obj)
{
    return mMessageQueue->removeMessages(this, what, obj);
}
*/
void Handler::dispatchMessage(const std::shared_ptr<Message>& message)
{
    
    handleMessage(message);
}

int64_t Handler::uptimeMicros()
{
    auto now = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return dur.count();
}