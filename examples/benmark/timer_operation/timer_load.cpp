#include "SLLooper.h"
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
#include <unistd.h> 

using namespace std::chrono_literals;

class TimerLoadTest {
private:
    std::shared_ptr<SLLooper> looper;
    std::vector<Timer> timers;
    std::atomic<uint64_t> timer_fires{0};
    std::atomic<bool> running{true};
    bool stress_mode = false;
    
public:
    void set_stress_mode(bool enable) { stress_mode = enable; }
    
    // Reduced CPU work function - much lighter for single core usage
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
    
    void create_one_shot_timers(int count) {
        std::cout << "Creating " << count << " one-shot timers..." << std::endl;
        
        for (int i = 0; i < count; ++i) {
            auto timer = looper->addTimer([this, i]() {
                timer_fires++;
                
                // Light CPU work
                light_cpu_work(i, 1500);  // Reduced from 10000 to 1500
                
                // Reduced additional work frequency and size
                if (i % 10 == 0) {  // Every 10th instead of every 5th
                    std::vector<int> data(50);  // Reduced from 200 to 50
                    std::iota(data.begin(), data.end(), i);
                    volatile int sum = std::accumulate(data.begin(), data.end(), 0);
                    
                    // Lighter sort operation
                    std::sort(data.begin(), data.end());  // Regular sort instead of reverse
                    sum += data[0];
                }
                
            }, 1000 + (i % 3000)); // 1-4 seconds spread (slightly longer intervals)
            
            timers.push_back(std::move(timer));
            
            // Print progress every 50 timers (more frequent updates)
            if ((i + 1) % 50 == 0) {
                std::cout << "Created " << (i + 1) << " timers" << std::endl;
            }
        }
    }
    
    void create_periodic_timers(int count) {
        std::cout << "Creating " << count << " periodic timers..." << std::endl;
        
        for (int i = 0; i < count; ++i) {
            auto timer = looper->addPeriodicTimer([this, i]() {
                timer_fires++;
                
                // Lighter work with smaller multiplier
                int work_multiplier = stress_mode ? 1.5 : 1;  // Reduced from 2 to 1.5
                light_cpu_work(i, static_cast<int>(800 * work_multiplier));  // Reduced from 6000 to 800
                
                // Smaller data structure operations
                std::map<int, double> temp_map;
                for (int j = 0; j < 5; ++j) {  // Reduced from 20 to 5
                    temp_map[j] = j * std::sin(i + j);
                }
                
                // Lighter statistics calculation
                double sum = 0.0;
                for (const auto& pair : temp_map) {
                    sum += pair.second * pair.second;
                }
                
                // Shorter string building
                std::string data_str = std::to_string(i) + "_" + std::to_string(static_cast<int>(sum));
                volatile size_t str_hash = std::hash<std::string>{}(data_str);
                
                // Smaller matrix computation, less frequent
                if (i % 5 == 0) {  // Every 5th instead of every 3rd
                    volatile double matrix_sum = 0.0;
                    for (int row = 0; row < 5; ++row) {  // Reduced from 10x10 to 5x5
                        for (int col = 0; col < 5; ++col) {
                            matrix_sum += std::sin(row) * std::cos(col) + (row * col);
                        }
                    }
                }
                
            }, 150 + (i % 600)); // 150ms to 750ms intervals (longer intervals for less frequent execution)
            
            timers.push_back(std::move(timer));
            
            if ((i + 1) % 25 == 0) {
                std::cout << "Created " << (i + 1) << " periodic timers" << std::endl;
            }
        }
    }
    
    void run_test(int one_shot_count, int periodic_count, int duration_seconds) {
        std::cout << "\n=== Light Timer Load Test (Single Core Friendly) ===" << std::endl;
        std::cout << "One-shot timers: " << one_shot_count << std::endl;
        std::cout << "Periodic timers: " << periodic_count << std::endl;
        std::cout << "Duration: " << duration_seconds << " seconds" << std::endl;
        std::cout << "Stress mode: " << (stress_mode ? "ENABLED" : "disabled") << std::endl;
        std::cout << "CPU target: ~50-90% single core usage" << std::endl;
        std::cout << "PID: " << getpid() << " (use this for monitoring)" << std::endl;
        
        // Initialize looper
        looper = std::make_shared<SLLooper>();
        
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
        std::cout << "Active timers: " << looper->getActiveTimerCount() << std::endl;
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
                      << ", Active: " << looper->getActiveTimerCount() 
                      << ", Rate: " << timer_fires.load() / (elapsed + 1) << " fires/sec"
                      << std::endl;
        }
        
        std::cout << "\nTest completed. Cleaning up..." << std::endl;
        
        // Cleanup
        timers.clear();
        looper->exit();
        looper_thread.join();
        
        std::cout << "Final stats:" << std::endl;
        std::cout << "Total timer fires: " << timer_fires.load() << std::endl;
        std::cout << "Average rate: " << timer_fires.load() / duration_seconds << " fires/sec" << std::endl;
        std::cout << "Light test completed successfully!" << std::endl;
    }
};

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
    
    std::cout << "Light Timer Load Test - PID: " << getpid() << std::endl;
    std::cout << "Usage: " << argv[0] << " [one_shot_count] [periodic_count] [duration_seconds] [stress]" << std::endl;
    std::cout << "Single Core Examples:" << std::endl;
    std::cout << "  " << argv[0] << " 100 20 30          # Light load (~30-50% CPU)" << std::endl;
    std::cout << "  " << argv[0] << " 200 30 60          # Normal load (~50-70% CPU)" << std::endl;
    std::cout << "  " << argv[0] << " 400 50 60 stress   # Heavy load (~70-90% CPU)" << std::endl;
    std::cout << std::endl;
    
    TimerLoadTest test;
    test.set_stress_mode(stress_mode);
    test.run_test(one_shot_count, periodic_count, duration);
    
    return 0;
}