#include "Handler.h"
#include <iostream>
#include <string>
using namespace std;
#pragma once

#define TEST1 1
#define TEST2 2

class myhandler : public Handler {
    public:
        myhandler(std::shared_ptr<SLLooper>& looper)
        : Handler(looper){} 
        void handleMessage(const std::shared_ptr<Message>& msg);
};

void myhandler::handleMessage(const std::shared_ptr<Message>& msg) {
    switch (msg->what) {
        case TEST1:
            cout << "receive test1\n";
            break;
        case TEST2:
            cout << "receive test2\n";
            break;
        default:
            break;
    }

}

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto handler = std::make_shared<myhandler>(looper);

    // Send message TEST1
    auto msg1 = handler->obtainMessage(TEST1);
    handler->sendMessage(msg1);

    // Send message TEST2
    auto msg2 = handler->obtainMessage(TEST2);
    handler->sendMessage(msg2);

    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    looper->exit();
    return 0;
}