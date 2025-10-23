#include "SLLooper.h"
#include "Handler.h"
#include "Message.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

using namespace swt;

enum MsgType : int32_t {
    MSG_STEP1 = 1,
    MSG_STEP2 = 2
};

class ProcessorHandler : public Handler {
public:
    ProcessorHandler(std::shared_ptr<SLLooper>& looper)
        : Handler(looper) {}

    void handleMessage(const std::shared_ptr<Message>& msg) override {
        switch (msg->what) {
        case MSG_STEP1: {
            int input = msg->arg1;
            std::cout << "[Handler] STEP1 recv input=" << input << "\n";
            int a = input + 5;
            std::cout << "[Handler] STEP1 result a=" << a << " -> post STEP2\n";
            auto m2 = obtainMessage(MSG_STEP2, a);
            // Gửi liền (có thể dùng delay nếu muốn giống await)
            sendMessage(m2);
            break;
        }
        case MSG_STEP2: {
            int a = msg->arg1;
            std::cout << "[Handler] STEP2 recv a=" << a << "\n";
            int b = a * 2;
            std::cout << "[Handler] Final result = " << b << "\n";
            if (auto sp = mLooper.lock()) {
                sp->exit(); // Dừng vòng lặp
            }
            break;
        }
        default:
            std::cout << "[Handler] Unknown what=" << msg->what << "\n";
            break;
        }
    }
};

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto handler = std::make_shared<ProcessorHandler>(looper);

    std::cout << "[main] Posting STEP1...\n";
    auto m1 = handler->obtainMessage(MSG_STEP1, 10);
    handler->sendMessage(m1);

    // Chờ xử lý
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    std::cout << "[main] Exit.\n";
    return 0;
}