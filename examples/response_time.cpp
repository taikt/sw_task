// response_time_test.cpp - Response time measurement using SLLooper::post() and postWork()
#include "SLLooper.h"
#include "Handler.h"
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
using namespace swt;

class ResponseTimeTestHandler : public Handler {
private:
    std::shared_ptr<SLLooper> mSLLooper;
    std::atomic<int> completed_tasks{0};
    int total_tasks;
    
    // ✅ SIMPLIFIED Timeline tracking - chỉ cần Total Response Time
    struct TaskTiming {
        int task_id;
        std::string task_type;
        std::chrono::high_resolution_clock::time_point post_time;      // Khi post vào looper
        std::chrono::high_resolution_clock::time_point complete_time;  // Khi task hoàn thành
        double total_response_ms;     // Tổng thời gian response (post -> complete)
        double computation_ms;        // Thời gian computation (for debug)
        std::thread::id thread_id;
        double task_result;
    };
    
    std::vector<TaskTiming> task_timelines;
    std::mutex timeline_mutex;
    std::chrono::high_resolution_clock::time_point test_start_time;
    
    enum TaskType {
        LIGHT_TASK = 1,
        HEAVY_TASK = 2
    };
    
public:
    ResponseTimeTestHandler(std::shared_ptr<SLLooper>& looper) : Handler(looper) {
        mSLLooper = looper;
    }
    
    // ✅ IMPLEMENT PURE VIRTUAL FUNCTION
    void handleMessage(const std::shared_ptr<Message>& msg) override {
        // Empty implementation - test sử dụng SLLooper::post() thay vì message passing
        std::cout << "Message received: what=" << msg->what << std::endl;
    }
    
    // ✅ NEW: Mixed task test with simple parameters
    void startMixedTaskTest(int heavy_tasks, int light_tasks) {
        int total = light_tasks + heavy_tasks;
        
        std::cout << "\n=== MIXED TASK RESPONSE TIME TEST ===" << std::endl;
        std::cout << "Testing " << heavy_tasks << " heavy + " << light_tasks << " light tasks" << std::endl;
        std::cout << "🔥 Heavy: 800-1500ms, 💡 Light: 200-500ms" << std::endl;
        std::cout << "Total: " << total << " tasks" << std::endl;
        std::cout << "PID: " << getpid() << std::endl;
        std::cout << "Hardware threads: " << std::thread::hardware_concurrency() << std::endl;
        std::cout << std::endl;
        
        total_tasks = total;
        completed_tasks = 0;
        task_timelines.clear();
        task_timelines.resize(total);
        
        test_start_time = std::chrono::high_resolution_clock::now();
        
        int task_counter = 0;
        
        // ✅ Post tất cả heavy tasks trước
        for(int i = 0; i < heavy_tasks; i++) {
            int heavy_id = task_counter++;
            auto post_time = std::chrono::high_resolution_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(timeline_mutex);
                task_timelines[heavy_id].task_id = heavy_id;
                task_timelines[heavy_id].task_type = "HEAVY";
                task_timelines[heavy_id].post_time = post_time;
            }
            
            auto promise = mSLLooper->postWork([this, heavy_id]() -> double {
                auto computation_start = std::chrono::high_resolution_clock::now();
                
                std::cout << "Heavy Task " << heavy_id << " STARTED on worker thread " 
                         << std::this_thread::get_id() << std::endl;
                
                double result = performHeavyComputation(heavy_id);
                
                auto computation_end = std::chrono::high_resolution_clock::now();
                double computation_time = std::chrono::duration_cast<std::chrono::microseconds>(
                    computation_end - computation_start).count() / 1000.0;
                
                {
                    std::lock_guard<std::mutex> lock(timeline_mutex);
                    if (heavy_id < static_cast<int>(task_timelines.size())) {
                        task_timelines[heavy_id].computation_ms = computation_time;
                        task_timelines[heavy_id].thread_id = std::this_thread::get_id();
                    }
                }
                
                std::cout << "Heavy Task " << heavy_id << " computation FINISHED in " 
                         << std::fixed << std::setprecision(2) << computation_time << "ms" << std::endl;
                
                return result;
            }, std::chrono::milliseconds(10000));
            
            promise.then(mSLLooper, [this, heavy_id](double result) {
                this->handleHeavyTaskComplete(heavy_id, result);
            }).catchError(mSLLooper, [this, heavy_id](std::exception_ptr ex) {
                std::cout << "Heavy Task " << heavy_id << " FAILED/TIMEOUT" << std::endl;
                int completed = ++completed_tasks;
                if (completed == total_tasks) {
                    printDetailedResults();
                    exportCSVData();
                    mSLLooper->exit();
                }
            });
            
            std::cout << "Heavy Task " << heavy_id << " posted at " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(post_time - test_start_time).count() / 1000.0 << "ms" << std::endl;
        }
        
        // ✅ Post tất cả light tasks
        for(int i = 0; i < light_tasks; i++) {
            int light_id = task_counter++;
            auto post_time = std::chrono::high_resolution_clock::now();
            
            {
                std::lock_guard<std::mutex> lock(timeline_mutex);
                task_timelines[light_id].task_id = light_id;
                task_timelines[light_id].task_type = "LIGHT";
                task_timelines[light_id].post_time = post_time;
            }
            
            mSLLooper->post([this, light_id]() {
                this->executeLightTask(light_id);
            });
            
            std::cout << "Light Task " << light_id << " posted at " 
                     << std::chrono::duration_cast<std::chrono::microseconds>(post_time - test_start_time).count() / 1000.0 << "ms" << std::endl;
        }
        
        std::cout << "All mixed tasks posted!" << std::endl;
    }
    
private:
    // ✅ FIXED executeLightTask - đo đúng Total Response Time
    void executeLightTask(int task_id) {
        auto computation_start = std::chrono::high_resolution_clock::now();
        
        std::cout << "Light Task " << task_id << " STARTED execution" << std::endl;
        
        double result = performLightComputation(task_id);
        
        auto computation_end = std::chrono::high_resolution_clock::now();
        auto complete_time = std::chrono::high_resolution_clock::now();
        
        double computation_time = std::chrono::duration_cast<std::chrono::microseconds>(
            computation_end - computation_start).count() / 1000.0;
        
        {
            std::lock_guard<std::mutex> lock(timeline_mutex);
            if (task_id < static_cast<int>(task_timelines.size())) {
                auto& timing = task_timelines[task_id];
                timing.complete_time = complete_time;
                timing.computation_ms = computation_time;
                timing.total_response_ms = std::chrono::duration_cast<std::chrono::microseconds>(
                    complete_time - timing.post_time).count() / 1000.0;
                timing.thread_id = std::this_thread::get_id();
                timing.task_result = result;
            }
        }
        
        std::cout << "Light Task " << task_id << " COMPLETED - " 
                 << "computation: " << std::fixed << std::setprecision(2) << computation_time << "ms, "
                 << "total_response: " << task_timelines[task_id].total_response_ms << "ms" << std::endl;
        
        int completed = ++completed_tasks;
        if (completed == total_tasks) {
            printDetailedResults();
            exportCSVData();
            mSLLooper->exit();
        }
    }
    
    // ✅ NEW handleHeavyTaskComplete - đo đúng Total Response Time
    void handleHeavyTaskComplete(int task_id, double result) {
        auto complete_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "Heavy Task " << task_id << " COMPLETED in main thread" << std::endl;
        
        {
            std::lock_guard<std::mutex> lock(timeline_mutex);
            if (task_id < static_cast<int>(task_timelines.size())) {
                auto& timing = task_timelines[task_id];
                timing.complete_time = complete_time;
                timing.total_response_ms = std::chrono::duration_cast<std::chrono::microseconds>(
                    complete_time - timing.post_time).count() / 1000.0;
                timing.task_result = result;
            }
        }
        
        std::cout << "Heavy Task " << task_id << " total_response: " 
                 << std::fixed << std::setprecision(2) << task_timelines[task_id].total_response_ms << "ms, "
                 << "computation: " << task_timelines[task_id].computation_ms << "ms" << std::endl;
        
        int completed = ++completed_tasks;
        if (completed == total_tasks) {
            printDetailedResults();
            exportCSVData();
            mSLLooper->exit();
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
        std::cout << "RESPONSE TIME TEST RESULTS" << std::endl;
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
        
        std::cout << "\n🏁 Test completed!" << std::endl;
    }
    
    // ✅ SIMPLIFIED CSV Export - chỉ cần 3 cột quan trọng
    void exportCSVData() {
        std::ofstream csv_file("execution_times.csv");
        csv_file << "Task_ID,Task_Type,Execution_Time_ms\n";  // ✅ Use Total Response Time as "Execution Time"
        
        for (const auto& timing : task_timelines) {
            csv_file << timing.task_id << ","
                    << timing.task_type << ","
                    << std::fixed << std::setprecision(3) << timing.total_response_ms << "\n";
        }
        
        csv_file.close();
        std::cout << "\n📊 CSV exported: execution_times.csv" << std::endl;
        std::cout << "   Format: Task_ID, Task_Type, Total_Response_Time_ms" << std::endl;
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
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "SW Task Response Time Test" << std::endl;
        std::cout << "Usage: " << argv[0] << " <heavy_tasks> <light_tasks>" << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " 10 2     # 10 heavy + 2 light tasks" << std::endl;
        std::cout << "  " << argv[0] << " 5 8      # 5 heavy + 8 light tasks" << std::endl;
        std::cout << "  " << argv[0] << " 0 10     # Only 10 light tasks" << std::endl;
        std::cout << "  " << argv[0] << " 5 0      # Only 5 heavy tasks" << std::endl;
        std::cout << std::endl;
        std::cout << "🔥 Heavy tasks: 800-1500ms each (postWork)" << std::endl;
        std::cout << "💡 Light tasks: 200-500ms each (post)" << std::endl;
        std::cout << "📊 Measures TOTAL RESPONSE TIME (post -> complete)" << std::endl;
        return 1;
    }
    
    int heavy_tasks = std::atoi(argv[1]);
    int light_tasks = std::atoi(argv[2]);
    
    if (heavy_tasks < 0 || heavy_tasks > 100 || light_tasks < 0 || light_tasks > 100) {
        std::cerr << "Error: Tasks must be 0-100" << std::endl;
        return 1;
    }
    
    if (heavy_tasks == 0 && light_tasks == 0) {
        std::cerr << "Error: At least one task required" << std::endl;
        return 1;
    }
    
    try {
        auto looper = std::make_shared<SLLooper>();
        auto handler = std::make_shared<ResponseTimeTestHandler>(looper);
        
        handler->startMixedTaskTest(heavy_tasks, light_tasks);
        
        looper->loop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}