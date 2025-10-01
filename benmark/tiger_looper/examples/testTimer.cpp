#include "SLLooper.h"
#include "Handler.h"
#include "Message.h"
#include "TimerManager.h"
#include <memory>
#include <thread>
#include <chrono>
#include <iostream>

#define TIMER_MSG1 101
#define TIMER_MSG2 102

class MyHandler : public Handler {
public:
    MyHandler(std::shared_ptr<SLLooper>& looper) : Handler(looper) {}
    void handleMessage(const std::shared_ptr<Message>& msg) override {
        switch (msg->what) {
            case TIMER_MSG1:
                std::cout << "Timer 1 fired!\n";
                break;
            case TIMER_MSG2:
                std::cout << "Timer 2 fired!\n";
                break;
            default:
                std::cout << "Unknown message: " << msg->what << std::endl;
                break;
        }
    }
};

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto handler = std::make_shared<MyHandler>(looper);
    TimerManager timerMgr(handler);

    std::thread loop_thread([&]() {
        looper->loop();
    });

    // Đăng ký 2 timer, mỗi timer sẽ gửi message khác nhau vào handler khi timeout
    timerMgr.startTimer(TIMER_MSG1, 1000); // 1 giây
    timerMgr.startTimer(TIMER_MSG2, 2000); // 2 giây

    std::this_thread::sleep_for(std::chrono::seconds(3));

    looper->exit();
    loop_thread.join();

    std::cout << "Main finished.\n";
    return 0;
}