#include <thread>
#include <memory>
#pragma once

class MessageQueue;

class SLLooper 
{
public:
    SLLooper();
    ~SLLooper();
    bool loop();
    std::shared_ptr<MessageQueue> messageQueue();
    void exit(); 

private:
    bool mStarted;   
    std::thread t1;
    std::shared_ptr<MessageQueue> mMessageQueue;
};