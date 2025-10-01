// tiger_response_time.cpp - Fixed to measure Total Response Time correctly
#include "SLLooper.h"
#include "Handler.h"
#include "Message.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <unistd.h>
#include <mutex>
#include <fstream>
#include <iomanip>

// Message IDs for different task types
#define MSG_LIGHT_TASK_BASE 1000
#define MSG_HEAVY_TASK_BASE 2000
#define MSG_START_TEST 500
#define MSG_EXIT_TEST 9999

class TigerResponseTimeTestHandler : public Handler {
private:
    std::atomic<int> completed_tasks{0};
    int total_tasks;
    std::string current_test_mode;
    
    // ✅ SIMPLIFIED Timeline tracking - đo Total Response Time
    struct TaskTiming {
        int task_id;
        std::string task_type;
        std::chrono::high_resolution_clock::time_point post_time;      // Khi post message vào looper
        std::chrono::high_resolution_clock::time_point complete_time;  // Khi task hoàn thành
        double total_response_ms;     // Post -> Complete (chính xác)
        double computation_ms;        // Pure computation time (debug)
        std::thread::id thread_id;
        double task_result;
    };
    
    std::vector<TaskTiming> task_timelines;
    std::mutex timeline_mutex;
    std::chrono::high_resolution_clock::time_point test_start_time;
    
public:
    TigerResponseTimeTestHandler(std::shared_ptr<SLLooper>& looper) : Handler(looper) {}
    
    void handleMessage(const std::shared_ptr<Message>& msg) override {
        switch(msg->what) {
            case MSG_START_TEST: {
                int num_tasks = msg->arg1;
                int test_type = msg->arg2; // 1=light, 2=heavy, 3=mixed
                
                total_tasks = num_tasks;
                completed_tasks = 0;
                test_start_time = std::chrono::high_resolution_clock::now();
                
                if (test_type == 1) {
                    current_test_mode = "LIGHT";
                    std::cout << "\n=== TIGER LOOPER LIGHT TASK TEST ===" << std::endl;
                    std::cout << "💡 Target: 200-500ms per task (sequential execution)" << std::endl;
                } else if (test_type == 2) {
                    current_test_mode = "HEAVY";
                    std::cout << "\n=== TIGER LOOPER HEAVY TASK TEST ===" << std::endl;
                    std::cout << "🔥 Target: 800-1500ms per task (sequential execution)" << std::endl;
                } else if (test_type == 3) {
                    current_test_mode = "MIXED";
                    std::cout << "\n=== TIGER LOOPER MIXED TASK TEST ===" << std::endl;
                    std::cout << "💡🔥 Mixed: Light + Heavy tasks (sequential execution)" << std::endl;
                }
                
                std::cout << "Testing " << num_tasks << " tasks using Tiger Looper" << std::endl;
                std::cout << "PID: " << getpid() << std::endl;
                std::cout << "⚠️  All tasks run sequentially on single event thread!" << std::endl;
                std::cout << std::endl;
                
                break;
            }
            
            case MSG_LIGHT_TASK_BASE ... MSG_LIGHT_TASK_BASE + 999: {
                int task_id = msg->arg1;  // Use arg1 for task_id
                executeTask(task_id, "LIGHT");
                break;
            }
            
            case MSG_HEAVY_TASK_BASE ... MSG_HEAVY_TASK_BASE + 999: {
                int task_id = msg->arg1;  // Use arg1 for task_id
                executeTask(task_id, "HEAVY");
                break;
            }
            
            case MSG_EXIT_TEST:
                std::cout << "\nExiting Tiger Looper test..." << std::endl;
                exit(0);
                break;
        }
    }
    
private:
    // ✅ FIXED executeTask - đo đúng Total Response Time
    void executeTask(int task_id, const std::string& task_type) {
        auto computation_start = std::chrono::high_resolution_clock::now();
        
        std::cout << task_type << " Task " << task_id << " STARTED execution on thread " 
                << std::this_thread::get_id() << std::endl;
        
        double result;
        if (task_type == "LIGHT") {
            result = performLightComputation(task_id);
        } else {
            result = performHeavyComputation(task_id);
        }
        
        auto computation_end = std::chrono::high_resolution_clock::now();
        auto complete_time = std::chrono::high_resolution_clock::now();
        
        double computation_time = std::chrono::duration_cast<std::chrono::microseconds>(
            computation_end - computation_start).count() / 1000.0;
        
        {
            std::lock_guard<std::mutex> lock(timeline_mutex);
            if (task_id >= 0 && task_id < static_cast<int>(task_timelines.size())) {
                auto& timing = task_timelines[task_id];
                
                timing.complete_time = complete_time;
                timing.computation_ms = computation_time;
                timing.total_response_ms = std::chrono::duration_cast<std::chrono::microseconds>(
                    complete_time - timing.post_time).count() / 1000.0;
                timing.thread_id = std::this_thread::get_id();
                timing.task_result = result;
                
                std::cout << "✅ " << task_type << " Task " << task_id << " COMPLETED - "
                         << "computation: " << std::fixed << std::setprecision(2) << computation_time << "ms, "
                         << "total_response: " << timing.total_response_ms << "ms" << std::endl;
            } else {
                std::cerr << "ERROR: task_id " << task_id << " out of bounds (size: " << task_timelines.size() << ")" << std::endl;
            }
        }
        
        int completed = ++completed_tasks;
        if (completed == total_tasks) {
            printDetailedResults();
            exportCSVData();
            
            // Schedule exit after short delay
            sendMessageDelayed(obtainMessage(MSG_EXIT_TEST), 1000);
        }
    }
    
    // 💡 ENHANCED LIGHT COMPUTATION - Target: 200-500ms
    double performLightComputation(int task_id) {
        double result = 0.0;
        
        // 1. Mathematical operations - tăng cường
        for(int i = 0; i < 1000000; i++) {  // Tăng từ 200K lên 1M
            result += std::sin(i * 0.00001) + std::cos(i * 0.00001) + std::sqrt(i + 1);
            if (i % 1000 == 0) {
                result += std::pow(i % 100, 1.5) + std::log(i + 1) * 0.1;
                result += std::atan(i * 0.0001) + std::tanh(i * 0.00001);
            }
        }
        
        // 2. Array operations - tăng kích thước
        std::vector<double> data(50000);  // Tăng từ 15K lên 50K
        for(size_t i = 0; i < data.size(); i++) {
            data[i] = std::sin(i + task_id) * std::cos(i * 0.001) + 
                     std::sqrt(i + 1) + std::pow(i % 50, 1.3);
        }
        
        // Multiple sorts để tăng CPU time
        for(int sort_round = 0; sort_round < 3; sort_round++) {
            std::sort(data.begin(), data.end());
            std::reverse(data.begin(), data.end());
        }
        result += std::accumulate(data.begin(), data.end(), 0.0) / data.size();
        
        // 3. Matrix operations - lớn hơn
        const int matrix_size = 120;  // Tăng từ 60 lên 120
        std::vector<std::vector<double>> matrix_a(matrix_size, std::vector<double>(matrix_size));
        std::vector<std::vector<double>> matrix_b(matrix_size, std::vector<double>(matrix_size));
        
        for(int i = 0; i < matrix_size; i++) {
            for(int j = 0; j < matrix_size; j++) {
                matrix_a[i][j] = std::sin(i + j + task_id) + std::cos(i * j);
                matrix_b[i][j] = std::cos(i - j + task_id) + std::sin(i + j);
                result += matrix_a[i][j] * matrix_b[i][j] * 0.001;
            }
        }
        
        // 4. Prime calculation
        int prime_count = 0;
        for(int n = 2; n < 8000; n++) {  // Tăng từ 5K lên 8K
            bool is_prime = true;
            for(int i = 2; i * i <= n; i++) {
                if(n % i == 0) {
                    is_prime = false;
                    break;
                }
            }
            if(is_prime) {
                prime_count++;
                result += std::sqrt(n) * 0.01;
            }
        }
        
        return result + static_cast<double>(prime_count);
    }
    
    // 🔥 ENHANCED HEAVY COMPUTATION - Target: 800-1500ms
    double performHeavyComputation(int task_id) {
        double result = 0.0;
        
        std::cout << "🔥 Starting HEAVY computation for task " << task_id << std::endl;
        
        // 1. Massive mathematical operations
        for(int i = 0; i < 3000000; i++) {  // Tăng từ 800K lên 3M
            result += std::sin(i * 0.000001) * std::cos(i * 0.000001) + 
                     std::sqrt(i + 1) + std::pow((i % 100), 1.6);
            
            if (i % 1000 == 0) {
                result += std::log(i + 1) * std::exp(i * 0.0000001) + 
                         std::atan(i * 0.0001) * std::tanh(i * 0.0001);
            }
        }
        
        // 2. Large matrix operations
        const int matrix_size = 250;  // Tăng từ 150 lên 250
        std::vector<std::vector<double>> matrix_a(matrix_size, std::vector<double>(matrix_size));
        std::vector<std::vector<double>> matrix_b(matrix_size, std::vector<double>(matrix_size));
        std::vector<std::vector<double>> matrix_c(matrix_size, std::vector<double>(matrix_size));
        
        // Matrix multiplication O(n³)
        for(int i = 0; i < matrix_size; i++) {
            for(int j = 0; j < matrix_size; j++) {
                matrix_a[i][j] = std::sin(i + j + task_id);
                matrix_b[i][j] = std::cos(i - j + task_id);
                matrix_c[i][j] = 0.0;
                for(int k = 0; k < matrix_size; k++) {
                    matrix_c[i][j] += matrix_a[i][k] * matrix_b[k][j];
                }
                result += matrix_c[i][j] * 0.0001;
            }
        }
        
        // 3. Prime calculation tăng cường
        int prime_count = 0;
        for(int n = 2; n < 50000; n++) {  // Tăng từ 16K lên 50K
            bool is_prime = true;
            for(int i = 2; i * i <= n; i++) {
                if(n % i == 0) {
                    is_prime = false;
                    break;
                }
            }
            if(is_prime) {
                prime_count++;
                result += std::sqrt(n) * std::log(n) * 0.001;
            }
        }
        
        std::cout << "🔥 Heavy task " << task_id << " computed " << prime_count << " primes" << std::endl;
        
        return result + static_cast<double>(prime_count) * 0.1;
    }
    
    void printDetailedResults() {
        auto test_end_time = std::chrono::high_resolution_clock::now();
        auto total_test_duration = std::chrono::duration_cast<std::chrono::milliseconds>(test_end_time - test_start_time);
        
        std::cout << "\n" << std::string(60, '=') << std::endl;
        std::cout << "TIGER LOOPER RESPONSE TIME TEST RESULTS" << std::endl;
        std::cout << std::string(60, '=') << std::endl;
        
        // ✅ Use TOTAL RESPONSE TIME instead of execution time
        std::vector<double> light_response_times, heavy_response_times;
        
        for (const auto& timing : task_timelines) {
            if (timing.task_type == "LIGHT") {
                light_response_times.push_back(timing.total_response_ms);
            } else if (timing.task_type == "HEAVY") {
                heavy_response_times.push_back(timing.total_response_ms);
            }
        }
        
        std::cout << "\n⚠️  EXECUTION MODEL: Sequential (Single Event Thread)" << std::endl;
        std::cout << "\n📊 OVERALL METRICS:" << std::endl;
        std::cout << "  Total test duration: " << total_test_duration.count() << " ms" << std::endl;
        std::cout << "  Tasks completed: " << completed_tasks.load() << "/" << total_tasks << std::endl;
        
        if (!light_response_times.empty()) {
            std::sort(light_response_times.begin(), light_response_times.end());
            std::cout << "\n💡 LIGHT TASK RESPONSE TIMES:" << std::endl;
            std::cout << "  Count: " << light_response_times.size() << std::endl;
            std::cout << "  Average: " << std::fixed << std::setprecision(2) << calculateMean(light_response_times) << " ms" << std::endl;
            std::cout << "  Min: " << light_response_times.front() << " ms" << std::endl;
            std::cout << "  Max: " << light_response_times.back() << " ms" << std::endl;
            std::cout << "  Median: " << calculateMedian(light_response_times) << " ms" << std::endl;
        }
        
        if (!heavy_response_times.empty()) {
            std::sort(heavy_response_times.begin(), heavy_response_times.end());
            std::cout << "\n🔥 HEAVY TASK RESPONSE TIMES:" << std::endl;
            std::cout << "  Count: " << heavy_response_times.size() << std::endl;
            std::cout << "  Average: " << calculateMean(heavy_response_times) << " ms" << std::endl;
            std::cout << "  Min: " << heavy_response_times.front() << " ms" << std::endl;
            std::cout << "  Max: " << heavy_response_times.back() << " ms" << std::endl;
            std::cout << "  Median: " << calculateMedian(heavy_response_times) << " ms" << std::endl;
            
            if (!light_response_times.empty()) {
                double ratio = calculateMean(heavy_response_times) / calculateMean(light_response_times);
                std::cout << "  Heavy/Light ratio: " << std::fixed << std::setprecision(1) << ratio << "x" << std::endl;
            }
        }
        
        // Task breakdown
        std::cout << "\n📋 TASK BREAKDOWN:" << std::endl;
        std::cout << std::setw(6) << "Task" << std::setw(8) << "Type" << std::setw(12) << "Response(ms)" << std::setw(12) << "Compute(ms)" << std::endl;
        std::cout << std::string(50, '-') << std::endl;
        
        for (const auto& timing : task_timelines) {
            std::cout << std::setw(6) << timing.task_id 
                     << std::setw(8) << timing.task_type
                     << std::setw(12) << std::fixed << std::setprecision(1) << timing.total_response_ms
                     << std::setw(12) << std::fixed << std::setprecision(1) << timing.computation_ms << std::endl;
        }
        
        std::cout << "\n🏁 Tiger Looper test completed!" << std::endl;
    }
    
    double calculateMean(const std::vector<double>& values) {
        if (values.empty()) return 0.0;
        return std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    }
    
    double calculateMedian(std::vector<double> values) {
        if (values.empty()) return 0.0;
        std::sort(values.begin(), values.end());
        size_t n = values.size();
        if (n % 2 == 0) {
            return (values[n/2 - 1] + values[n/2]) / 2.0;
        } else {
            return values[n/2];
        }
    }
    
    // ✅ SIMPLIFIED CSV Export - sử dụng Total Response Time
    void exportCSVData() {
        std::ofstream csv_file("tiger_execution_times.csv");
        csv_file << "Task_ID,Task_Type,Execution_Time_ms\n";  // ✅ Use Total Response Time as "Execution Time"
        
        for (const auto& timing : task_timelines) {
            csv_file << timing.task_id << ","
                    << timing.task_type << ","
                    << std::fixed << std::setprecision(3) << timing.total_response_ms << "\n";
        }
        
        csv_file.close();
        std::cout << "\n📊 Tiger CSV exported: tiger_execution_times.csv" << std::endl;
        std::cout << "   Format: Task_ID, Task_Type, Total_Response_Time_ms" << std::endl;
    }
    
public:
    // ✅ FIXED startLightTaskTest - initialize timeline với post_time
    void startLightTaskTest(int num_tasks) {
        {
            std::lock_guard<std::mutex> lock(timeline_mutex);
            task_timelines.clear();
            task_timelines.resize(num_tasks);
        }
        
        sendMessage(obtainMessage(MSG_START_TEST, num_tasks, 1)); // 1 = light
        
        // ✅ Post tất cả tasks và record post_time
        for(int i = 0; i < num_tasks; i++) {
            auto post_time = std::chrono::high_resolution_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(timeline_mutex);
                task_timelines[i].task_id = i;
                task_timelines[i].task_type = "LIGHT";
                task_timelines[i].post_time = post_time;  // ✅ Record post time
            }
            
            sendMessage(obtainMessage(MSG_LIGHT_TASK_BASE + i, i));
            std::cout << "Light Task " << i << " posted at " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(post_time - std::chrono::high_resolution_clock::now()).count() / 1000.0 << "ms" << std::endl;
        }
    }
    
    // ✅ FIXED startHeavyTaskTest - initialize timeline với post_time
    void startHeavyTaskTest(int num_tasks) {
        {
            std::lock_guard<std::mutex> lock(timeline_mutex);
            task_timelines.clear();
            task_timelines.resize(num_tasks);
        }
        
        sendMessage(obtainMessage(MSG_START_TEST, num_tasks, 2)); // 2 = heavy
        
        // ✅ Post tất cả tasks và record post_time
        for(int i = 0; i < num_tasks; i++) {
            auto post_time = std::chrono::high_resolution_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(timeline_mutex);
                task_timelines[i].task_id = i;
                task_timelines[i].task_type = "HEAVY";
                task_timelines[i].post_time = post_time;  // ✅ Record post time
            }
            
            sendMessage(obtainMessage(MSG_HEAVY_TASK_BASE + i, i));
            std::cout << "Heavy Task " << i << " posted" << std::endl;
        }
    }
    
    // ✅ FIXED startMixedTaskTest - initialize timeline với post_time
    void startMixedTaskTest(int light_tasks, int heavy_tasks) {
        int total = light_tasks + heavy_tasks;
        
        {
            std::lock_guard<std::mutex> lock(timeline_mutex);
            task_timelines.clear();
            task_timelines.resize(total);
        }
        
        sendMessage(obtainMessage(MSG_START_TEST, total, 3)); // 3 = mixed
        
        int task_counter = 0;
        
        // ✅ Post light tasks với post_time
        for(int i = 0; i < light_tasks; i++) {
            auto post_time = std::chrono::high_resolution_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(timeline_mutex);
                task_timelines[task_counter].task_id = task_counter;
                task_timelines[task_counter].task_type = "LIGHT";
                task_timelines[task_counter].post_time = post_time;
            }
            
            sendMessage(obtainMessage(MSG_LIGHT_TASK_BASE + task_counter, task_counter));
            std::cout << "Light Task " << task_counter << " posted" << std::endl;
            task_counter++;
        }
        
        // ✅ Post heavy tasks với post_time
        for(int i = 0; i < heavy_tasks; i++) {
            auto post_time = std::chrono::high_resolution_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(timeline_mutex);
                task_timelines[task_counter].task_id = task_counter;
                task_timelines[task_counter].task_type = "HEAVY";
                task_timelines[task_counter].post_time = post_time;
            }
            
            sendMessage(obtainMessage(MSG_HEAVY_TASK_BASE + task_counter, task_counter));
            std::cout << "Heavy Task " << task_counter << " posted" << std::endl;
            task_counter++;
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Tiger Looper Response Time Test - Fixed Total Response Measurement" << std::endl;
        std::cout << "Usage:" << std::endl;
        std::cout << "  " << argv[0] << " light [num]   # Light tasks (200-500ms each)" << std::endl;
        std::cout << "  " << argv[0] << " heavy [num]   # Heavy tasks (800-1500ms each)" << std::endl;
        std::cout << "  " << argv[0] << " mixed [L] [H] # Mixed tasks" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " light 8       # 8 light tasks (sequential)" << std::endl;
        std::cout << "  " << argv[0] << " heavy 5       # 5 heavy tasks (sequential)" << std::endl;
        std::cout << "  " << argv[0] << " mixed 6 3     # 6 light + 3 heavy (sequential)" << std::endl;
        std::cout << std::endl;
        std::cout << "⚠️  Note: Tiger Looper executes all tasks sequentially!" << std::endl;
        std::cout << "📊 Measures TOTAL RESPONSE TIME (post -> complete)" << std::endl;
        return 1;
    }
    
    std::string test_mode = argv[1];
    
    try {
        auto looper = std::make_shared<SLLooper>();
        auto handler = std::make_shared<TigerResponseTimeTestHandler>(looper);
        
        // Start event loop in separate thread
        std::thread looper_thread([&looper]() {
            looper->loop();
        });
        
        // Wait for loop to start
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        if (test_mode == "light") {
            int num_tasks = (argc >= 3) ? std::atoi(argv[2]) : 8;
            if (num_tasks <= 0 || num_tasks > 100) {
                std::cerr << "Light tasks: 1-100, got: " << num_tasks << std::endl;
                return 1;
            }
            handler->startLightTaskTest(num_tasks);
        }
        else if (test_mode == "heavy") {
            int num_tasks = (argc >= 3) ? std::atoi(argv[2]) : 5;
            if (num_tasks <= 0 || num_tasks > 100) {
                std::cerr << "Heavy tasks: 1-100, got: " << num_tasks << std::endl;
                return 1;
            }
            handler->startHeavyTaskTest(num_tasks);
        }
        else if (test_mode == "mixed") {
            int light_tasks = (argc >= 3) ? std::atoi(argv[2]) : 5;
            int heavy_tasks = (argc >= 4) ? std::atoi(argv[3]) : 3;
            if (light_tasks < 0 || light_tasks > 100 || heavy_tasks < 0 || heavy_tasks > 100) {
                std::cerr << "Light: 0-100 (got " << light_tasks << "), Heavy: 0-100 (got " << heavy_tasks << ")" << std::endl;
                return 1;
            }
            handler->startMixedTaskTest(light_tasks, heavy_tasks);
        }
        else {
            std::cerr << "Valid modes: light, heavy, mixed" << std::endl;
            return 1;
        }
        
        // Wait for test to complete
        looper_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}