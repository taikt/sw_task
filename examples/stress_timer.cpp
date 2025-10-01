#include "SLLooper.h"
#include "Timer.h"
#include <random>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>
#include <iomanip>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <csignal> // Thêm để dùng signal/SIGINT/SIGTERM

using namespace swt;

class SWTaskPureOneShotTest {
private:
    std::shared_ptr<SLLooper> looper;
    std::vector<Timer> active_timers;
    std::thread looper_thread;
    std::mutex timers_mutex;
    int next_timer_id = 0;

    // Random number generators
    std::random_device rd;
    std::mt19937 gen;

    // Memory management counters
    size_t cleanup_counter = 0;

    // Configurable timeout range
    int max_timeout_sec = 40; // Default 40 seconds

    // Thêm các biến còn thiếu
    std::atomic<bool> running{true};
    std::atomic<int> timer_fires{0};
    std::atomic<int> timers_completed{0};
    std::atomic<int> timers_created{0};
    bool stress_mode = false;
    std::chrono::steady_clock::time_point test_start_time;

public:
    SWTaskPureOneShotTest(int timeout_max = 40) : gen(rd()), max_timeout_sec(timeout_max) {
        looper = std::make_shared<SLLooper>();
        looper_thread = std::thread([this]() {
            looper->loop();
        });
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        std::cout << "✅ SW Task framework initialized (NO REGENERATION)" << std::endl;
        std::cout << "📊 Timer timeout range: 0-" << max_timeout_sec << " seconds" << std::endl;
    }

    ~SWTaskPureOneShotTest() {
        cleanup_framework();
    }

    size_t get_active_timer_count() {
        std::lock_guard<std::mutex> lock(timers_mutex);
        return active_timers.size();
    }

    void create_oneshot_timer_batch(int batch_size) {
        std::lock_guard<std::mutex> lock(timers_mutex);
        cleanup_completed_timers_internal();

        std::uniform_int_distribution<> timeout_dist(0, max_timeout_sec * 1000);
        std::string timeout_range = "0-" + std::to_string(max_timeout_sec) + "s";

        std::cout << "  🎲 Creating " << batch_size << " timers with random timeouts (" 
                  << timeout_range << "), current active: " << active_timers.size() << std::endl;

        for (int i = 0; i < batch_size; ++i) {
            int timer_id = ++next_timer_id;
            int delay_ms = timeout_dist(gen);

            std::cout << "    Timer " << timer_id << " -> " 
                      << std::fixed << std::setprecision(1) << (delay_ms/1000.0) << "s" << std::endl;

            auto timer = looper->addTimer([this, timer_id, delay_ms]() {
                timer_fires++;
                timers_completed++;

                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                    now - test_start_time).count();

                std::cout << "🔥 [" << elapsed << "s] Timer " << timer_id 
                          << " FIRED (scheduled: " << (delay_ms/1000.0) << "s)" << std::endl;

                light_cpu_work(timer_id, stress_mode ? 1500 : 1000);
            }, delay_ms);

            active_timers.push_back(std::move(timer));
            timers_created++;
        }

        std::cout << "  ✅ Created " << batch_size << " one-shot timers, total active: " 
                  << active_timers.size() << std::endl;

        cleanup_completed_timers_internal();
    }

    void cleanup_framework() {
        running.store(false);

        {
            std::lock_guard<std::mutex> lock(timers_mutex);
            std::cout << "🧹 Final cleanup: cancelling " << active_timers.size() << " timers" << std::endl;
            for (auto& timer : active_timers) {
                if (timer.isActive()) {
                    timer.cancel();
                }
            }
            active_timers.clear();
            active_timers.shrink_to_fit();
        }

        if (looper) {
            looper->exit();
        }
        if (looper_thread.joinable()) {
            looper_thread.join();
        }

        std::cout << "✅ SW Task framework cleaned up (memory freed)" << std::endl;
    }

    std::string get_framework_name() {
        return "SW_Task";
    }

    void export_detailed_results(const std::string& framework_name) {
        std::cout << "📊 File export disabled for CPU monitoring test" << std::endl;
    }

    // Thêm các hàm còn thiếu
    void set_stress_mode(bool mode) {
        stress_mode = mode;
    }

    // Hàm mô phỏng công việc nhẹ
    void light_cpu_work(int timer_id, int iterations) {
        double dummy = 0.0;
        for (int i = 0; i < iterations * 1000; ++i) {
            dummy += std::sin(i + timer_id) * std::cos(i - timer_id);
        }
    }

    // Hàm chạy test chính - sửa lại để chờ tất cả timer hoàn thành + 10s
    void run_pure_oneshot_test(int initial_timer_count, int max_timeout_sec) {
        test_start_time = std::chrono::steady_clock::now();
        
        create_oneshot_timer_batch(initial_timer_count);

        std::cout << "⏳ Waiting for all timers to complete..." << std::endl;
        
        // Chờ đến khi tất cả timer hoàn thành
        while (running.load()) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
            
            size_t active = get_active_timer_count();
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - test_start_time).count();
            
            std::cout << "⏳ Time: " << elapsed << "s"
                      << ", Active timers: " << active
                      << ", Timers fired: " << timer_fires.load()
                      << ", Timers completed: " << timers_completed.load()
                      << std::endl;

            cleanup_completed_timers();
            
            // Kiểm tra xem tất cả timer đã hoàn thành chưa
            if (active == 0 && timers_created.load() > 0) {
                std::cout << "✅ All timers completed! Waiting additional 10 seconds before exit..." << std::endl;
                break;
            }
        }
        
        // Chờ thêm 10 giây sau khi tất cả timer hoàn thành
        for (int i = 10; i > 0; i--) {
            std::cout << "⏰ Exiting in " << i << " seconds..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        std::cout << "🏁 Test finished. Total timers created: " << timers_created.load()
                  << ", fired: " << timer_fires.load()
                  << ", completed: " << timers_completed.load() << std::endl;
    }

private:
    void cleanup_completed_timers() {
        std::lock_guard<std::mutex> lock(timers_mutex);
        cleanup_completed_timers_internal();
    }

    void cleanup_completed_timers_internal() {
        cleanup_counter++;
        size_t before = active_timers.size();
        active_timers.erase(
            std::remove_if(active_timers.begin(), active_timers.end(),
                [](const Timer& timer) { 
                    return !timer.isActive();
                }),
            active_timers.end()
        );
        size_t after = active_timers.size();
        if (after < before) {
            std::cout << "    🧹 Cleaned up " << (before - after) 
                      << " completed timers, remaining: " << after << std::endl;
        }
        if (cleanup_counter % 10 == 0) {
            if (active_timers.capacity() > active_timers.size() * 2 && 
                active_timers.capacity() > 50) {
                size_t old_capacity = active_timers.capacity();
                active_timers.shrink_to_fit();
                std::cout << "    💾 Shrunk vector capacity: " << old_capacity 
                         << " -> " << active_timers.capacity() << std::endl;
            }
        }
    }
};

void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ". Exiting gracefully..." << std::endl;
    exit(0);
}

int main(int argc, char* argv[]) {
    // Kiểm tra nếu không có tham số thì hiển thị hướng dẫn
    if (argc < 3) {
        std::cout << "SW Task Pure One-Shot Timer Test" << std::endl;
        std::cout << "Usage: " << argv[0] << " <timer_count> <max_timeout_sec>" << std::endl;
        std::cout << std::endl;
        std::cout << "Parameters:" << std::endl;
        std::cout << "  timer_count      - Number of one-shot timers to create" << std::endl;
        std::cout << "  max_timeout_sec  - Maximum timeout (0 to max_timeout_sec seconds)" << std::endl;
        std::cout << std::endl;
        std::cout << "Behavior:" << std::endl;
        std::cout << "  - Creates specified number of one-shot timers" << std::endl;
        std::cout << "  - Each timer has random timeout from 0 to max_timeout_sec" << std::endl;
        std::cout << "  - Waits for all timers to complete" << std::endl;
        std::cout << "  - Waits additional 10 seconds before program exit" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " 100 40    # 100 timers, 0-40s random timeouts" << std::endl;
        std::cout << "  " << argv[0] << " 50 20     # 50 timers, 0-20s random timeouts" << std::endl;
        std::cout << "  " << argv[0] << " 200 60    # 200 timers, 0-60s random timeouts" << std::endl;
        std::cout << std::endl;
        return 1;
    }

    // Setup signal handler  
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    int initial_timer_count = std::atoi(argv[1]);
    int max_timeout = std::atoi(argv[2]);

    if (initial_timer_count <= 0) {
        std::cerr << "Error: timer_count must be greater than 0" << std::endl;
        return 1;
    }

    if (max_timeout < 0) {
        std::cerr << "Error: max_timeout_sec must be >= 0" << std::endl;
        return 1;
    }

    std::cout << "SW Task Pure One-Shot Timer Test" << std::endl;
    std::cout << "Configuration:" << std::endl;
    std::cout << "  - Timer count: " << initial_timer_count << std::endl;
    std::cout << "  - Timeout range: 0-" << max_timeout << " seconds (random)" << std::endl;
    std::cout << "  - Behavior: Wait for all timers + 10s before exit" << std::endl;
    std::cout << std::endl;

    try {
        SWTaskPureOneShotTest test(max_timeout);
        test.run_pure_oneshot_test(initial_timer_count, max_timeout);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}