#include "SLLooper.h"
#include <iostream>
#include <chrono>

using namespace std::chrono_literals;
using namespace swt;

int main() {
    auto looper = std::make_shared<SLLooper>();
    std::cout << "Timer backend: " << TimerManager::getBackendName() << std::endl;
    
    // ✅ CORRECT: addTimer với chrono
    auto timer1 = looper->addTimer([]() {
        std::cout << "Timer 1 fired after 2 seconds!" << std::endl;
    }, 2s);
    
    // ✅ CORRECT: addPeriodicTimer với chrono
    auto periodicTimer = looper->addPeriodicTimer([]() {
        std::cout << "Periodic tick: " << std::time(nullptr) << std::endl;
    }, 1s);
    
    // ✅ CORRECT: addTimer với milliseconds
    auto timer2 = looper->addTimer([]() {
        std::cout << "Timer 2 fired!" << std::endl;
    }, 1500);
    
    // ✅ CORRECT: postWithTimeout
    auto timeoutTimer = looper->postWithTimeout([]() {
        std::cout << "Timeout function executed!" << std::endl;
    }, 3000);
    
    
    // Cancel timer easily
    std::this_thread::sleep_for(5s);
    periodicTimer.cancel();
    std::cout << "Cancelled periodic timer" << std::endl;
    
    // Restart timer
    timer1.restart(1000);
    
    // Check if active
    std::cout << "Timer1 active: " << timer1.isActive() << std::endl;
    std::cout << "PeriodicTimer active: " << periodicTimer.isActive() << std::endl;
    
    // Keep running
    std::this_thread::sleep_for(10s);
    
    return 0;
}