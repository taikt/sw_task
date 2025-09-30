#include "SLLooper.h"
#include "Promise.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

// Simulate fetching data asynchronously
kt::Promise<int> fetchData(std::shared_ptr<SLLooper> looper) {
    auto promise = looper->createPromise<int>();
    looper->post([promise]() mutable {
        std::cout << "Fetching data..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        promise.set_value(7);
    });
    return promise;
}

int main() {
    auto looper = std::make_shared<SLLooper>();

    fetchData(looper)
    .then(looper, [](int value) {
        std::cout << "Step 1: Got value = " << value << std::endl;
        return value * 3;
    })
    .then(looper, [](int value) {
        std::cout << "Step 2: Value after multiply = " << value << std::endl;
        return std::to_string(value);
    })
    .then(looper, [](const std::string& str) {
        std::cout << "Step 3: Final string = " << str << std::endl;
        return 0;
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}