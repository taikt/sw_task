// test_post_simple.cpp
#include "SLLooper.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    auto looper = std::make_shared<SLLooper>();
       
    auto future1 = looper->post([]() -> int {
        std::cout << "Calculating..." << std::endl;
        return 42;
    });
    
    auto future2 = looper->postDelayed(1000, []() {
        std::cout << "Delayed execution after 1 second!" << std::endl;
    });
    
    // Get results
    std::cout << "Result: " << future1.get() << std::endl;
    
    std::cout << "Waiting for delayed task..." << std::endl;
    future2.get(); // Wait for completion

    
    return 0;
}