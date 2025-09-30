#include "SLLooper.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>

using namespace std::chrono_literals;

int main() {
    auto looper = std::make_shared<SLLooper>();
    
    std::cout << "=== Testing Timer with std::vector ===" << std::endl;
    
    // ✅ START EVENT LOOP FIRST
    std::cout << "\n=== Starting event loop ===" << std::endl;
    std::thread looperThread([&looper]() {
        looper->loop();
    });
    
    // Wait for event loop to start
    std::this_thread::sleep_for(200ms);
    std::cout << "Event loop started, proceeding with timer creation..." << std::endl;
    
    // ✅ Bây giờ có thể dùng vector an toàn với move semantics
    std::vector<Timer> timers;
    
    std::cout << "\n=== Creating timers and adding to vector ===" << std::endl;
    
    // Tạo nhiều timer và add vào vector
    for (int i = 0; i < 5; ++i) {
        auto timer = looper->addTimer([i]() {
            std::cout << "🔥 Vector Timer " << i << " fired at " 
                      << std::time(nullptr) << "!" << std::endl;
        }, (i + 1) * 1000);  // 1s, 2s, 3s, 4s, 5s
        
        std::cout << "Adding timer " << i << " (ID: " << timer.getId() 
                  << ") to vector..." << std::endl;
        timers.push_back(std::move(timer));  // Move vào vector an toàn
    }
    
    // Thêm periodic timer
    auto periodicTimer = looper->addPeriodicTimer([]() {
        static int count = 0;
        std::cout << "🔄 Periodic timer tick #" << ++count << std::endl;
    }, 800);
    
    std::cout << "Adding periodic timer (ID: " << periodicTimer.getId() 
              << ") to vector..." << std::endl;
    timers.push_back(std::move(periodicTimer));
    
    std::cout << "\n=== All " << timers.size() << " timers added to vector ===" << std::endl;
    
    // Check timer status
    std::cout << "\n=== Initial timer status ===" << std::endl;
    for (size_t i = 0; i < timers.size(); ++i) {
        std::cout << "Timer " << i << " - ID: " << timers[i].getId() 
                  << ", Active: " << timers[i].isActive() << std::endl;
    }
    
    // Test vector operations (reserve không cần thiết nữa)
    std::cout << "\n=== Testing vector operations ===" << std::endl;
    timers.reserve(10);  // Optional, không cần thiết nữa
    std::cout << "Vector reserved for 10 elements" << std::endl;
    
    // Add thêm timer sau resize
    auto extraTimer = looper->addTimer([]() {
        std::cout << "⭐ Extra timer fired!" << std::endl;
    }, 2500);
    
    std::cout << "Adding extra timer (ID: " << extraTimer.getId() 
              << ") after reserve..." << std::endl;
    timers.push_back(std::move(extraTimer));
    
    // Wait và observe timers firing
    std::cout << "\n=== Waiting for timers to fire (8 seconds) ===" << std::endl;
    std::this_thread::sleep_for(8s);
    
    // Cancel một timer
    std::cout << "\n=== Cancelling timer 2 ===" << std::endl;
    if (timers.size() > 2) {
        timers[2].cancel();
        std::cout << "Timer 2 cancelled" << std::endl;
    }
    
    // Check status after cancellation
    std::cout << "\n=== Timer status after cancellation ===" << std::endl;
    for (size_t i = 0; i < timers.size(); ++i) {
        std::cout << "Timer " << i << " - ID: " << timers[i].getId() 
                  << ", Active: " << timers[i].isActive() << std::endl;
    }
    
    // Restart một timer
    std::cout << "\n=== Restarting timer 1 with 1.5s delay ===" << std::endl;
    if (timers.size() > 1) {
        timers[1].restart(1500);
        std::cout << "Timer 1 restarted" << std::endl;
    }
    
    // Wait thêm để xem restart effect
    std::cout << "\n=== Waiting for restart effect (3 seconds) ===" << std::endl;
    std::this_thread::sleep_for(3s);
    
    // Cancel periodic timer (timer cuối)
    std::cout << "\n=== Cancelling periodic timer ===" << std::endl;
    if (!timers.empty()) {
        timers.back().cancel();  // Cancel periodic timer
        std::cout << "Periodic timer cancelled" << std::endl;
    }
    
    // Final status
    std::cout << "\n=== Final timer status ===" << std::endl;
    for (size_t i = 0; i < timers.size(); ++i) {
        std::cout << "Timer " << i << " - ID: " << timers[i].getId() 
                  << ", Active: " << timers[i].isActive() << std::endl;
    }
    
    std::cout << "\n=== Testing vector clear/destruction ===" << std::endl;
    std::cout << "Active timer count before clear: " << looper->getActiveTimerCount() << std::endl;
    
    // Clear vector - should properly destruct all Timer objects
    timers.clear();
    std::cout << "Vector cleared" << std::endl;
    std::cout << "Active timer count after clear: " << looper->getActiveTimerCount() << std::endl;
    
    // Wait một chút để confirm không có timer nào fire
    std::cout << "\n=== Waiting to confirm no timers fire (2 seconds) ===" << std::endl;
    std::this_thread::sleep_for(2s);
    
    // Stop looper
    std::cout << "\n=== Stopping looper ===" << std::endl;
    looper->exit();
    looperThread.join();
    
    std::cout << "\n=== Test completed successfully! ===" << std::endl;
    
    return 0;
}