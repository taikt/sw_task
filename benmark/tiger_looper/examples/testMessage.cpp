#include "SLLooper.h"
#include "Message.h"
#include "Handler.h"
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>
#include <string>

// Định nghĩa myhandler trực tiếp trong file này
#define TEST1 1
#define TEST2 2

class myhandler : public Handler {
public:
    myhandler(std::shared_ptr<SLLooper>& looper) : Handler(looper) {}
    void handleMessage(const std::shared_ptr<Message>& msg) override {
        switch (msg->what) {
            case TEST1:
                std::cout << "receive test1\n";
                break;
            case TEST2:
                std::cout << "receive test2\n";
                break;
            default:
                break;
        }
    }
};

int main() {
    // Tạo SLLooper và myhandler
    auto looper = std::make_shared<SLLooper>();
    auto handler = std::make_shared<myhandler>(looper);

    // Khởi động thread loop
    std::thread loop_thread([&]() {
        looper->loop();
    });

    // Gửi 2 message test
    handler->sendMessage(handler->obtainMessage(TEST1));
    handler->sendMessage(handler->obtainMessage(TEST2));

    // Đợi một chút để message được xử lý
    std::this_thread::sleep_for(std::chrono::seconds(1));

    // Thoát loop và join thread
    looper->exit();
    loop_thread.join();

    std::cout << "Main finished.\n";
    return 0;
}