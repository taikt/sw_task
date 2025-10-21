#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <memory>
#include <thread>

using namespace swt;

// Simulate async operation by fetching data
int fetchData() {
    std::cout << "Fetching data" << std::endl;
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    return 123;
}

Task<void> demo(std::shared_ptr<SLLooper> looper) {
    std::cout << "Before awaitPost" << std::endl;
    int result = co_await looper->awaitPost(fetchData);

    std::cout << "Result from awaitPost: " << result << std::endl;
}

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto task = demo(looper);
    task.start();
    std::cout << "Task started, waiting for completion..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(4));
    return 0;
}