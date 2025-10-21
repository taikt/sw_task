#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include <string>
#include <cstdlib>

using namespace swt;

// Chia cho 0 với số nguyên, hoặc truy cập bộ nhớ sai (segfault) KHÔNG sinh C++ exception 
// bắt được bằng try/catch; đó là undefined behavior hoặc tín hiệu (SIGFPE, SIGSEGV).
// Muốn có exception “tự phát” ta dùng các hàm chuẩn như std::vector::at() 
// (ném std::out_of_range), std::stoi() (ném std::invalid_argument hoặc std::out_of_range), 
// std::string::substr() với tham số sai, v.v.

// Hàm gây ra std::out_of_range do truy cập ngoài biên bằng vector.at()
int implicitOutOfRange() {
    std::cout << "[implicitOutOfRange] Prepare...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::vector<int> v = {1, 2, 3};
    // Không dùng throw explícit mà rely vào .at() tự ném out_of_range
    return v.at(10); // ném std::out_of_range
}

// Hàm gây ra std::invalid_argument do chuyển đổi chuỗi không hợp lệ
int implicitInvalidArgument() {
    std::cout << "[implicitInvalidArgument] Prepare...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    // std::stoi ném std::invalid_argument với chuỗi không phải số
    int value = std::stoi("abcXYZ"); // ném std::invalid_argument
    return value;
}

// Coroutine bắt lỗi out_of_range
Task<void> demoOutOfRange(std::shared_ptr<SLLooper> looper) {
    std::cout << "[demoOutOfRange] Before awaitPost\n";
    try {
        int v = co_await looper->awaitPost(implicitOutOfRange);
        std::cout << "[demoOutOfRange] Value: " << v << "\n";
    // } catch (const std::out_of_range& ex) {
    //     std::cout << "[demoOutOfRange] Caught out_of_range: " << ex.what() << "\n";
    } catch (const std::exception& ex) {
        std::cout << "[demoOutOfRange] Other exception: " << ex.what() << "\n";
    }
    std::cout << "[demoOutOfRange] Done.\n";
}

// Coroutine bắt lỗi invalid_argument
Task<void> demoInvalidArgument(std::shared_ptr<SLLooper> looper) {
    std::cout << "[demoInvalidArgument] Before awaitPost\n";
    try {
        int v = co_await looper->awaitPost(implicitInvalidArgument);
        std::cout << "[demoInvalidArgument] Value: " << v << "\n";
    // } catch (const std::invalid_argument& ex) {
    //     std::cout << "[demoInvalidArgument] Caught invalid_argument: " << ex.what() << "\n";
    } catch (const std::exception& ex) {
        std::cout << "[demoInvalidArgument] Other exception: " << ex.what() << "\n";
    }
    std::cout << "[demoInvalidArgument] Done.\n";
}

int main() {
    auto looper = std::make_shared<SLLooper>();

    auto t1 = demoOutOfRange(looper);
    auto t2 = demoInvalidArgument(looper);

    t1.start();
    t2.start();

    std::cout << "[main] Started tasks, waiting...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "[main] Exit.\n";
    return 0;
}