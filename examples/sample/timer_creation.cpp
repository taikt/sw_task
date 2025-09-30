#include "SLLooper.h"
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;

int main() {
    auto looper = std::make_shared<SLLooper>();
    
    // add one-shot timer with 2 seconds timeout
    auto timer1 = looper->addTimer([]() {
        std::cout << "Timer 1 fired after 2 seconds!" << std::endl;
        // do something
    }, 2s);
    
    // add periodic timer with 1 second interval
    auto periodicTimer = looper->addPeriodicTimer([]() {
        std::cout << "Periodic tick: " << std::time(nullptr) << std::endl;
        // do something periodically
    }, 1s);
}