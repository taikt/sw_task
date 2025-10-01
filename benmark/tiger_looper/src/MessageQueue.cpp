#include "MessageQueue.h"
#include "Message.h"
#include <chrono>
#include <iostream>

MessageQueue::MessageQueue() : mQuit(false), mCurrentMessage(nullptr) {}

MessageQueue::~MessageQueue() {
    quit();
}

bool MessageQueue::enqueueMessage(const std::shared_ptr<Message>& msg, int64_t whenUs) {
    std::unique_lock<std::mutex> lock(iMutex);
    // std::cout << "message queue: receive msg\n";
    msg->whenUs = whenUs;
    msg->mNextMessage = nullptr;

    if (!mCurrentMessage || whenUs < mCurrentMessage->whenUs) {
        msg->mNextMessage = mCurrentMessage;
        mCurrentMessage = msg;
    } else {
        auto p = mCurrentMessage;
        while (p->mNextMessage && p->mNextMessage->whenUs <= whenUs) {
            p = p->mNextMessage;
        }
        msg->mNextMessage = p->mNextMessage;
        p->mNextMessage = msg;
    }
    mQueueChanged.notify_one();
    return true;
}

std::shared_ptr<Message> MessageQueue::poll() {
    std::unique_lock<std::mutex> lock(iMutex);
    while (!mQuit) {
        auto now = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        if (mCurrentMessage && mCurrentMessage->whenUs <= now) {
            auto msg = mCurrentMessage;
            mCurrentMessage = mCurrentMessage->mNextMessage;
            return msg;
        }

        if (mCurrentMessage) {
            auto wait_us = mCurrentMessage->whenUs - now;
            if (wait_us > 0) {
                mQueueChanged.wait_for(lock, std::chrono::microseconds(wait_us));
            }
        } else {
            mQueueChanged.wait(lock);
        }
    }
    return nullptr;
}

void MessageQueue::quit() {
    std::unique_lock<std::mutex> lock(iMutex);
    mQuit = true;
    mQueueChanged.notify_all();
}