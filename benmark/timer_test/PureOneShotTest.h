#ifndef PURE_ONESHOT_TEST_H
#define PURE_ONESHOT_TEST_H

#include <iostream>
#include <chrono>
#include <vector>
#include <thread>
#include <atomic>
#include <signal.h>
#include <cmath>
#include <string>
#include <fstream>
#include <iomanip>
#include <unistd.h>
#include <sstream>
#include <algorithm>
#include <mutex>

class PureOneShotTestBase {
protected:
    std::atomic<uint64_t> timer_fires{0};
    std::atomic<uint64_t> timers_created{0};
    std::atomic<uint64_t> timers_completed{0};
    std::atomic<bool> running{true};
    bool stress_mode = false;
    
    struct MemorySnapshot {
        std::chrono::steady_clock::time_point timestamp;
        double elapsed_seconds;
        size_t memory_mb;
        uint64_t active_timers;
        uint64_t timer_fires;
        uint64_t timers_created;
        uint64_t timers_completed;
        std::string phase;
    };
    
    std::vector<MemorySnapshot> memory_history;
    std::chrono::steady_clock::time_point test_start_time;
    mutable std::mutex memory_mutex; // Thread safety for memory snapshots

public:
    void set_stress_mode(bool enable) { stress_mode = enable; }
    
    // Light CPU work - consistent across both frameworks
    void light_cpu_work(int timer_id, int base_iterations = 1000) {
        volatile double result = 0.0;
        int iterations = base_iterations;
        
        if (stress_mode) {
            iterations *= 2;
        }
        
        // Mathematical computations
        for (int i = 0; i < iterations; ++i) {
            result += std::sin(i * 0.01) * std::cos(i * 0.01);
            result += std::sqrt(static_cast<double>(i + 1));
            
            if (i % 100 == 0) {
                result += std::pow(static_cast<double>(i), 1.05) + std::log(static_cast<double>(i + 1));
            }
        }
        
        // Small memory operations
        std::vector<double> temp_data(20 + (timer_id % 30));
        for (size_t j = 0; j < temp_data.size(); ++j) {
            temp_data[j] = result + static_cast<double>(j) * 0.1;
            result += temp_data[j] * 0.001;
        }
        
        // String operations
        std::string work_str = "timer_" + std::to_string(timer_id);
        for (int k = 0; k < 10; ++k) {
            work_str += std::to_string(k * result);
            if (work_str.length() > 200) {
                work_str = "reset_" + std::to_string(timer_id);
            }
        }
        
        volatile size_t hash = std::hash<std::string>{}(work_str);
        result += static_cast<double>(hash) * 0.00001;
        
        // Prevent optimization
        (void)result;
    }
    
protected:
    // Get current memory usage in MB
    size_t get_current_memory_mb() {
        std::ifstream status("/proc/self/status");
        if (!status.is_open()) {
            return 0; // Fallback if can't read memory
        }
        
        std::string line;
        size_t rss_kb = 0;
        
        while (std::getline(status, line)) {
            if (line.substr(0, 6) == "VmRSS:") {
                std::istringstream iss(line);
                std::string key, value, unit;
                iss >> key >> value >> unit;
                try {
                    rss_kb = std::stoul(value);
                } catch (const std::exception&) {
                    return 0; // Fallback on parse error
                }
                break;
            }
        }
        
        return rss_kb / 1024; // Convert to MB
    }
    
    void take_memory_snapshot(const std::string& phase) {
        std::lock_guard<std::mutex> lock(memory_mutex);
        
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - test_start_time).count() / 1000.0;
        
        memory_history.push_back({
            now,
            elapsed,
            get_current_memory_mb(),
            get_active_timer_count(),
            timer_fires.load(),
            timers_created.load(),
            timers_completed.load(),
            phase
        });
    }
    
    // ✅ REMOVED: export_detailed_results() - no file export anymore!
    
    void print_real_time_stats(int elapsed_seconds) {
        // Prevent division by zero
        double fire_rate = elapsed_seconds > 0 ? 
            static_cast<double>(timer_fires.load()) / static_cast<double>(elapsed_seconds) : 0.0;
        
        std::cout << "[" << elapsed_seconds << "s] "
                  << "Fires: " << timer_fires.load()
                  << ", Active: " << get_active_timer_count()
                  << ", Created: " << timers_created.load()
                  << ", Completed: " << timers_completed.load()
                  << ", Memory: " << get_current_memory_mb() << "MB"
                  << ", Rate: " << std::fixed << std::setprecision(1) << fire_rate << " fires/sec"
                  << std::endl;
    }
    
    // Virtual functions to be implemented by derived classes
    virtual size_t get_active_timer_count() = 0;
    virtual void create_oneshot_timer_batch(int batch_size, int base_delay_ms) = 0;
    virtual void cleanup_framework() = 0;
    virtual std::string get_framework_name() = 0;
    
public:
    virtual ~PureOneShotTestBase() = default;
    
    void run_pure_oneshot_test(int initial_timer_count, int duration_seconds, bool enable_regeneration = true) {
        std::cout << "\n=== " << get_framework_name() << " PURE ONE-SHOT TIMER TEST ===" << std::endl;
        std::cout << "Initial timer count: " << initial_timer_count << std::endl;
        std::cout << "Test duration: " << duration_seconds << " seconds" << std::endl;
        std::cout << "Timer regeneration: " << (enable_regeneration ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << "Stress mode: " << (stress_mode ? "ENABLED" : "DISABLED") << std::endl;
        std::cout << "PID: " << getpid() << " (use this for external monitoring)" << std::endl;
        
        test_start_time = std::chrono::steady_clock::now();
        
        // Baseline memory snapshot
        take_memory_snapshot("baseline");
        
        // Phase 1: Create initial batch of timers
        std::cout << "\n📍 Phase 1: Creating initial timer batch..." << std::endl;
        try {
            create_oneshot_timer_batch(initial_timer_count, 1000); // 1-3 second delays
            take_memory_snapshot("initial_batch_created");
        } catch (const std::exception& e) {
            std::cerr << "❌ Error creating initial timers: " << e.what() << std::endl;
            return;
        }
        
        // Phase 2: Monitor and optionally regenerate timers
        std::cout << "\n📍 Phase 2: Monitoring timer execution..." << std::endl;
        
        for (int i = 0; i < duration_seconds && running.load(); ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            print_real_time_stats(i + 1);
            take_memory_snapshot("monitoring_" + std::to_string(i + 1));
            
            // Regenerate timers periodically to maintain load
            if (enable_regeneration && (i + 1) % 10 == 0 && running.load()) {
                std::cout << "  🔄 Regenerating timer batch..." << std::endl;
                try {
                    create_oneshot_timer_batch(initial_timer_count / 4, 500); // Smaller batch, shorter delays
                    take_memory_snapshot("regenerated_" + std::to_string(i + 1));
                } catch (const std::exception& e) {
                    std::cerr << "⚠️  Error regenerating timers: " << e.what() << std::endl;
                }
            }
        }
        
        // Phase 3: Cleanup and final measurements
        std::cout << "\n📍 Phase 3: Cleanup and final measurements..." << std::endl;
        running.store(false);
        
        try {
            cleanup_framework();
            take_memory_snapshot("cleanup_initiated");
        } catch (const std::exception& e) {
            std::cerr << "⚠️  Error during cleanup: " << e.what() << std::endl;
        }
        
        // Wait a bit to see memory release
        for (int i = 0; i < 5; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            take_memory_snapshot("post_cleanup_" + std::to_string(i + 1));
        }
        
        // ✅ REMOVED: export_detailed_results() call - no file export!
        
        std::cout << "\n🎉 " << get_framework_name() << " Pure One-Shot Test Completed!" << std::endl;
        std::cout << "📊 Final Stats:" << std::endl;
        std::cout << "  Total Timer Fires: " << timer_fires.load() << std::endl;
        std::cout << "  Total Timers Created: " << timers_created.load() << std::endl;
        
        if (duration_seconds > 0) {
            double avg_fire_rate = static_cast<double>(timer_fires.load()) / static_cast<double>(duration_seconds);
            std::cout << "  Average Fire Rate: " << std::fixed << std::setprecision(1) << avg_fire_rate << " fires/sec" << std::endl;
        }
    }
};

#endif // PURE_ONESHOT_TEST_H