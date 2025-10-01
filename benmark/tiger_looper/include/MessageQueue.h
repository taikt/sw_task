#include <memory>
#include <mutex>
#include <condition_variable>
#pragma once

class Message;

class MessageQueue {
public:
    MessageQueue();
    ~MessageQueue();

    bool enqueueMessage(const std::shared_ptr<Message>& msg, int64_t whenUs);
    std::shared_ptr<Message> poll();
    void quit();

private:
    std::mutex iMutex;
    std::condition_variable mQueueChanged;
    bool mQuit;
    std::shared_ptr<Message> mCurrentMessage;
};