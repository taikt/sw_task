#include "SLLooper.h"
#include "Promise.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <string>

class Processor {
public:
    int processStep1(int value) {
        std::cout << "Step 1 (method): Got value = " << value << std::endl;
        return value * 2;
    }
    std::string processStep2(int value) {
        std::cout << "Step 2 (method): Value after multiply = " << value << std::endl;
        return std::to_string(value);
    }
    int processStep3(const std::string& str) {
        std::cout << "Step 3 (method): Final string = " << str << std::endl;
        return 0;
    }
};

kt::Promise<int> fetchData(std::shared_ptr<SLLooper> looper) {
    auto promise = looper->createPromise<int>();
    looper->post([promise]() mutable {
        std::cout << "Fetching data..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        promise.set_value(5);
    });
    return promise;
}

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto processor = std::make_shared<Processor>();

    fetchData(looper)
    .then(looper, [processor](int value) {
        return processor->processStep1(value);
    })
    .then(looper, [processor](int value) {
        return processor->processStep2(value);
    })
    .then(looper, [processor](const std::string& str) {
        return processor->processStep3(str);
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}