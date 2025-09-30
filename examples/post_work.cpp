// test_cpu.cpp
#include "SLLooper.h"
#include <iostream>
#include <thread>
#include <chrono>
using namespace swt;

int main() {
    auto looper = std::make_shared<SLLooper>();
    
    std::cout << "Testing CPU task..." << std::endl;
    
    // Test basic CPU task
    auto promise = looper->postWork([]() -> int {
        std::cout << "Doing heavy work..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(2));
        return 42;
    });
    
    // Chain result
    promise.then(looper, [](int result) {
        std::cout << "Result: " << result << std::endl;
        return result * 2;
    });
    
    // Wait a bit
    std::this_thread::sleep_for(std::chrono::seconds(5));
    return 0;
}