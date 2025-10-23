#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <memory>
#include <thread>

using namespace swt;

class Processor {
public:
    int step1(int v) {
        std::cout << "[Processor::step1] v=" << v << "\n";
        return v + 5;
    }
    int step2(int v) {
        std::cout << "[Processor::step2] v=" << v << "\n";
        return v * 2;
    }
};

Task<void> demo(std::shared_ptr<SLLooper> looper, std::shared_ptr<Processor> proc) {
    std::cout << "[demo] Begin\n";

    int a = co_await looper->awaitPost([proc] {
        return proc->step1(10);
    });

    int b = co_await looper->awaitPost([proc, a] {
        return proc->step2(a);
    });

    std::cout << "[demo] Final result = " << b << "\n";
    looper->exit(); // Thoát ngay sau khi hoàn tất
    std::cout << "[demo] Done\n";
}

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto processor = std::make_shared<Processor>();

    auto task = demo(looper, processor);
    task.start();

    // Chờ một chút để coroutine chạy
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    std::cout << "[main] Exit.\n";
    return 0;
}