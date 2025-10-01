#include "SLLooper.h"
#include "MessageQueue.h"
#include "Message.h"
#include <iostream>

SLLooper::SLLooper() : mStarted(false) {
    std::cout << "finish init slloper\n";
    mMessageQueue = std::make_shared<MessageQueue>();
}

SLLooper::~SLLooper() {
    exit();
}

bool SLLooper::loop() {
    std::cout << "start looper\n";
    mStarted = true;
    while (mStarted) {
        // std::cout << "looper waiting\n";
        auto msg = mMessageQueue->poll();
        if (!mStarted) break;
        if (msg) {
            // std::cout << "looper notified msg\n";
            msg->sendToTarget();
            // std::cout << "looper dispatched msg\n";
        }
    }
    return true;
}

std::shared_ptr<MessageQueue> SLLooper::messageQueue() {
    return mMessageQueue;
}

void SLLooper::exit() {
    mStarted = false;
    if (mMessageQueue) {
        mMessageQueue->quit();
    }
}