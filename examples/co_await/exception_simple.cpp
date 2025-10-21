#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <memory>
#include <thread>
#include <stdexcept>

using namespace swt;

// Simulate an asynchronous task that maybe throws exception.
int fetchData() {
    std::cout << "[fetchData] Start...\n";
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    // You can replace with a conditional
    if (true)
        return 123; 
    else         
        throw std::runtime_error("fetchMaybeFail: simulated failure");
}

Task<void> demo(std::shared_ptr<SLLooper> looper) {
    std::cout << "[demo] Before awaitPost\n";
    try {
        int value = co_await looper->awaitPost(fetchData);
        std::cout << "[demo] Received value: " << value << "\n";
    } catch (const std::exception& ex) {
        std::cout << "[demo] Caught exception: " << ex.what() << "\n";
    }
    std::cout << "[demo] Done.\n";
}

int main() {
    auto looper = std::make_shared<SLLooper>();

    auto t = demo(looper);
    t.start(); 

    std::cout << "[main] Started, waiting briefly...\n";
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "[main] Exit.\n";
    return 0;
}