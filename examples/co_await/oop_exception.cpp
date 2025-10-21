#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <memory>
#include <thread>
#include <stdexcept>

using namespace swt;

class Processor {
public:
    int step1(int v) {
        std::cout << "[Processor::step1] v=" << v << "\n";
        return v + 5;
    }
    int step2(int v) {
        std::cout << "[Processor::step2] v=" << v << " (will fail)\n";
        // Giả lập lỗi đơn giản
        throw std::runtime_error("Processor::step2 simulated failure");
        // return v * 2; // Nếu muốn thử case thành công, bỏ dòng throw ở trên và mở dòng này.
    }
};

// Coroutine: gọi 2 bước, bắt exception tại điểm lỗi
Task<void> demo(std::shared_ptr<SLLooper> looper, std::shared_ptr<Processor> proc) {
    std::cout << "[demo] Begin\n";

    int a = co_await looper->awaitPost([proc] {
        return proc->step1(10);
    });

    try {
        int b = co_await looper->awaitPost([proc, a] {
            return proc->step2(a);
        });
        std::cout << "[demo] Final result = " << b << "\n";
    } catch (const std::exception& ex) {
        std::cout << "[demo] Caught exception: " << ex.what() << "\n";
    }

    std::cout << "[demo] Done\n";
}

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto processor = std::make_shared<Processor>();

    auto task = demo(looper, processor);
    task.start(); // Task là lazy

    std::cout << "[main] Started coroutine, waiting...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "[main] Exit.\n";
    return 0;
}