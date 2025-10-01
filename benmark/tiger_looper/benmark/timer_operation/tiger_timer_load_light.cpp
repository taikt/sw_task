// tiger_timer_load_light.cpp - Light Timer Load Test for Tiger Looper Framework
#include "SLLooper.h"
#include "Handler.h"
#include "Message.h"
#include "TimerManager.h"
#include "Refbase.h"
#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <signal.h>
#include <cmath>
#include <string>
#include <map>
#include <numeric>
#include <algorithm>
#include <memory>

using namespace std::chrono_literals;

// Message IDs for different timer types
#define MSG_ONE_SHOT_TIMER_BASE 1000
#define MSG_PERIODIC_TIMER_BASE 2000
#define MSG_TIMER_CLEANUP 9999

// Forward declaration
class TigerTimerLoadTest;

class LoadTestHandler : public Handler {
private:
    TigerTimerLoadTest* mLoadTest;
    bool stress_mode = false;
    
public:
    LoadTestHandler(std::shared_ptr<SLLooper>& looper, TigerTimerLoadTest* loadTest) 
        : Handler(looper), mLoadTest(loadTest) {}
    
    void setStressMode(bool enable) { stress_mode = enable; }
    
    void handleMessage(const std::shared_ptr<Message>& msg) override;
    
private:
    // Light CPU work function - same workload as timer_load_light.cpp
    void light_cpu_work(int timer_id, int base_iterations = 1000) {  // Reduced from 8000 to 1000
        volatile double result = 0.0;
        int iterations = base_iterations;
        
        if (stress_mode) {
            iterations *= 2; // 2x instead of 3x for more reasonable load
        }
        
        // Lighter mathematical computations
        for (int i = 0; i < iterations; ++i) {
            result += std::sin(i * 0.01) * std::cos(i * 0.01);  // Reduced frequency calculations
            result += std::sqrt(i + 1);
            
            // Less frequent expensive operations
            if (i % 100 == 0) {  // Every 100 instead of 50
                result += std::pow(i, 1.05) + std::log(i + 1);  // Lighter pow operation
            }
        }
        
        // Smaller memory operations
        std::vector<double> temp_data(20 + (timer_id % 30));  // Reduced from 50-150 to 20-50 elements
        for (size_t j = 0; j < temp_data.size(); ++j) {
            temp_data[j] = result + j * 0.1;
            result += temp_data[j] * 0.001;
        }
        
        // Lighter string operations
        std::string work_str = "timer_" + std::to_string(timer_id);
        for (int k = 0; k < 10; ++k) {  // Reduced from 50 to 10 iterations
            work_str += std::to_string(k * result);
            if (work_str.length() > 200) {  // Reduced from 500 to 200
                work_str = "reset_" + std::to_string(timer_id);
            }
        }
        
        // Simple hash computation
        volatile size_t hash = std::hash<std::string>{}(work_str);
        result += hash * 0.00001;
    }
};

class TigerTimerLoadTest {
private:
    std::shared_ptr<SLLooper> looper;
    std::shared_ptr<LoadTestHandler> handler;
    std::unique_ptr<TimerManager> timerManager;
    std::vector<timer_t> activeTimers;
    std::atomic<uint64_t> timer_fires{0};
    std::atomic<bool> running{true};
    bool stress_mode = false;
    int periodic_count = 0;
    
public:
    void set_stress_mode(bool enable) { 
        stress_mode = enable; 
        if (handler) {
            handler->setStressMode(enable);
        }
    }
    
    void increment_timer_fires() { timer_fires++; }
    
    void create_one_shot_timers(int count) {
        std::cout << "Creating " << count << " one-shot timers..." << std::endl;
        
        for (int i = 0; i < count; ++i) {
            int messageId = MSG_ONE_SHOT_TIMER_BASE + i;
            int delay = 1000 + (i % 3000); // 1-4 seconds spread (slightly longer intervals)
            
            timer_t timerId = timerManager->startTimer(messageId, delay);
            if (timerId != (timer_t)0) {
                activeTimers.push_back(timerId);
            }
            
            // Print progress every 50 timers (more frequent updates)
            if ((i + 1) % 50 == 0) {
                std::cout << "Created " << (i + 1) << " timers" << std::endl;
            }
        }
    }
    
    void create_periodic_timers(int count) {
        std::cout << "Creating " << count << " periodic timers..." << std::endl;
        periodic_count = count;
        
        // Note: TimerManager only supports one-shot timers
        // For periodic behavior, we recreate timers in handleMessage
        for (int i = 0; i < count; ++i) {
            int messageId = MSG_PERIODIC_TIMER_BASE + i;
            int interval = 150 + (i % 600); // 150ms to 750ms intervals (longer intervals)
            
            timer_t timerId = timerManager->startTimer(messageId, interval);
            if (timerId != (timer_t)0) {
                activeTimers.push_back(timerId);
            }
            
            if ((i + 1) % 25 == 0) {
                std::cout << "Created " << (i + 1) << " periodic timers" << std::endl;
            }
        }
    }
    
    void restart_periodic_timer(int messageId) {
        if (!running) return;
        
        // Restart periodic timer
        int timer_index = messageId - MSG_PERIODIC_TIMER_BASE;
        if (timer_index >= 0 && timer_index < periodic_count) {
            int interval = 150 + (timer_index % 600);
            
            timer_t timerId = timerManager->startTimer(messageId, interval);
            if (timerId != (timer_t)0) {
                activeTimers.push_back(timerId);
            }
        }
    }
    
    void run_test(int one_shot_count, int periodic_count, int duration_seconds) {
        std::cout << "\n=== Tiger Looper Light Timer Load Test (Single Core Friendly) ===" << std::endl;
        std::cout << "One-shot timers: " << one_shot_count << std::endl;
        std::cout << "Periodic timers: " << periodic_count << std::endl;
        std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
        std::cout << "Stress mode: " << (stress_mode ? "ENABLED" : "disabled") << std::endl;
        std::cout << "CPU target: ~50-90% single core usage" << std::endl;
        std::cout << "PID: " << getpid() << " (use this for monitoring)" << std::endl;
        
        // Initialize tiger_looper components
        looper = std::make_shared<SLLooper>();
        handler = std::make_shared<LoadTestHandler>(looper, this);
        handler->setStressMode(stress_mode);
        timerManager = std::make_unique<TimerManager>(handler);
        
        // Start event loop in separate thread
        std::thread looper_thread([this]() {
            looper->loop();
        });
        
        // Wait for loop to start
        std::this_thread::sleep_for(300ms);
        
        std::cout << "\nEvent loop started. Creating timers..." << std::endl;
        
        // Create timers
        create_one_shot_timers(one_shot_count);
        create_periodic_timers(periodic_count);
        
        std::cout << "\nAll timers created!" << std::endl;
        std::cout << "Total active timers: " << activeTimers.size() << std::endl;
        std::cout << "Expected CPU load: " << (stress_mode ? "60-90%" : "30-70%") << " single core" << std::endl;
        std::cout << "Starting monitoring phase..." << std::endl;
        
        // Print periodic stats
        auto start_time = std::chrono::steady_clock::now();
        
        for (int i = 0; i < duration_seconds; ++i) {
            std::this_thread::sleep_for(1s);
            
            auto current_time = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                current_time - start_time).count();
            
            std::cout << "[" << elapsed << "s] "
                      << "Timer fires: " << timer_fires.load() 
                      << ", Active timers: " << activeTimers.size()
                      << ", Rate: " << timer_fires.load() / (elapsed + 1) << " fires/sec"
                      << std::endl;
        }
        
        std::cout << "\nTest completed. Cleaning up..." << std::endl;
        
        // Stop accepting new periodic timers
        running = false;
        
        // Cleanup timers
        for (auto timerId : activeTimers) {
            timerManager->stopTimer(timerId);
        }
        activeTimers.clear();
        
        looper->exit();
        looper_thread.join();
        
        std::cout << "Final stats:" << std::endl;
        std::cout << "Total timer fires: " << timer_fires.load() << std::endl;
        std::cout << "Average rate: " << timer_fires.load() / duration_seconds << " fires/sec" << std::endl;
        std::cout << "Light test completed successfully!" << std::endl;
    }
};

// Implementation of LoadTestHandler::handleMessage
void LoadTestHandler::handleMessage(const std::shared_ptr<Message>& msg) {
    if (!mLoadTest) return;
    
    mLoadTest->increment_timer_fires();
    
    if (msg->what >= MSG_ONE_SHOT_TIMER_BASE && msg->what < MSG_PERIODIC_TIMER_BASE) {
        // One-shot timer callback
        int timer_id = msg->what - MSG_ONE_SHOT_TIMER_BASE;
        
        // Light CPU work
        light_cpu_work(timer_id, 1500);  // Reduced from 10000 to 1500
        
        // Reduced additional work frequency and size
        if (timer_id % 10 == 0) {  // Every 10th instead of every 5th
            std::vector<int> data(50);  // Reduced from 200 to 50
            std::iota(data.begin(), data.end(), timer_id);
            volatile int sum = std::accumulate(data.begin(), data.end(), 0);
            
            // Lighter sort operation
            std::sort(data.begin(), data.end());  // Regular sort instead of reverse
            sum += data[0];
        }
        
    } else if (msg->what >= MSG_PERIODIC_TIMER_BASE && msg->what < MSG_TIMER_CLEANUP) {
        // Periodic timer callback
        int timer_id = msg->what - MSG_PERIODIC_TIMER_BASE;
        
        // Lighter work with smaller multiplier
        int work_multiplier = stress_mode ? 1.5 : 1;  // Reduced from 2 to 1.5
        light_cpu_work(timer_id, static_cast<int>(800 * work_multiplier));  // Reduced from 6000 to 800
        
        // Smaller data structure operations
        std::map<int, double> temp_map;
        for (int j = 0; j < 5; ++j) {  // Reduced from 20 to 5
            temp_map[j] = j * std::sin(timer_id + j);
        }
        
        // Lighter statistics calculation
        double sum = 0.0;
        for (const auto& pair : temp_map) {
            sum += pair.second * pair.second;
        }
        
        // Shorter string building
        std::string data_str = std::to_string(timer_id) + "_" + std::to_string(static_cast<int>(sum));
        volatile size_t str_hash = std::hash<std::string>{}(data_str);
        
        // Smaller matrix computation, less frequent
        if (timer_id % 5 == 0) {  // Every 5th instead of every 3rd
            volatile double matrix_sum = 0.0;
            for (int row = 0; row < 5; ++row) {  // Reduced from 10x10 to 5x5
                for (int col = 0; col < 5; ++col) {
                    matrix_sum += std::sin(row) * std::cos(col) + (row * col);
                }
            }
        }
        
        // Restart this periodic timer
        mLoadTest->restart_periodic_timer(msg->what);
    }
}

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Exiting gracefully..." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    // Setup signal handler for clean exit
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Parse command line arguments - lighter defaults for single core
    int one_shot_count = 200;   // Reduced from 1000
    int periodic_count = 30;    // Reduced from 100
    int duration = 60;
    bool stress_mode = false;
    
    if (argc >= 2) one_shot_count = std::atoi(argv[1]);
    if (argc >= 3) periodic_count = std::atoi(argv[2]);  
    if (argc >= 4) duration = std::atoi(argv[3]);
    if (argc >= 5) stress_mode = (std::string(argv[4]) == "stress");
    
    std::cout << "Tiger Looper Light Timer Load Test - PID: " << getpid() << std::endl;
    std::cout << "Usage: " << argv[0] << " [one_shot_count] [periodic_count] [duration_seconds] [stress]" << std::endl;
    std::cout << "Single Core Examples:" << std::endl;
    std::cout << "  " << argv[0] << " 100 20 30          # Light load (~30-50% CPU)" << std::endl;
    std::cout << "  " << argv[0] << " 200 30 60          # Normal load (~50-70% CPU)" << std::endl;
    std::cout << "  " << argv[0] << " 400 50 60 stress   # Heavy load (~70-90% CPU)" << std::endl;
    std::cout << std::endl;
    
    try {
        TigerTimerLoadTest test;
        test.set_stress_mode(stress_mode);
        test.run_test(one_shot_count, periodic_count, duration);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}