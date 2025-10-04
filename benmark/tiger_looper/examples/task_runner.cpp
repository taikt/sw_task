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

#define LIGHT_TASK_MSG 2001
#define HEAVY_TASK_MSG 2002
#define HEAVY_TASK_COMPLETE_MSG 2003
#define KEEP_ALIVE_MSG 2004

class TigerTaskHandler : public Handler {
private:
    std::atomic<int> completed_tasks{0};
    int total_tasks;
    std::shared_ptr<SLLooper> stored_looper; // Store looper reference
    
public:
    TigerTaskHandler(std::shared_ptr<SLLooper>& looper) : Handler(looper), stored_looper(looper) {}
    
    void handleMessage(const std::shared_ptr<Message>& msg) override {
        switch (msg->what) {
            case LIGHT_TASK_MSG:
                executeLightTask(msg->arg1);
                break;
            case HEAVY_TASK_MSG:
                executeHeavyTask(msg->arg1);
                break;
            case HEAVY_TASK_COMPLETE_MSG:
                handleHeavyTaskComplete(msg->arg1, *static_cast<double*>(msg->obj));
                // Clean up memory
                delete static_cast<double*>(msg->obj);
                break;
            case KEEP_ALIVE_MSG:
                std::this_thread::sleep_for(std::chrono::seconds(10));
                if (stored_looper) {
                    stored_looper->exit();
                }
                break;
            default:
                break;
        }
    }
    
    void startMixedTaskTest(int heavy_tasks, int light_tasks) {
        int total = heavy_tasks + light_tasks;
        total_tasks = total;
        completed_tasks = 0;
        
        // Log số lượng task khi bắt đầu
        std::cout << "=== TIGER TASK RUNNER STARTING ===" << std::endl;
        std::cout << "Heavy tasks: " << heavy_tasks << std::endl;
        std::cout << "Light tasks: " << light_tasks << std::endl;
        std::cout << "Total tasks: " << total << std::endl;
        std::cout << "PID: " << getpid() << std::endl;
        std::cout << "Starting execution..." << std::endl;
        std::cout << std::endl;
        
        // Post tất cả heavy tasks trước
        for(int i = 0; i < heavy_tasks; i++) {
            auto msg = Message::obtain(shared_from_this(), HEAVY_TASK_MSG, i, 0, 0);
            sendMessage(msg);
        }
        
        // Post tất cả light tasks sau
        for(int i = 0; i < light_tasks; i++) {
            int light_id = heavy_tasks + i;  // ID sau heavy tasks
            auto msg = Message::obtain(shared_from_this(), LIGHT_TASK_MSG, light_id, 0, 0);
            sendMessage(msg);
        }
    }
    
private:
    void executeLightTask(int task_id) {
        // Chỉ thực hiện computation, không đo thời gian, không log
        double result = performLightComputation(task_id);
        
        int completed = ++completed_tasks;
        if (completed == total_tasks) {
            // Post keep-alive task để sleep trong event loop
            auto msg = Message::obtain(shared_from_this(), KEEP_ALIVE_MSG, 0, 0, 0);
            sendMessage(msg);
        }
    }
    
    void executeHeavyTask(int task_id) {
        // Thực hiện heavy computation
        double result = performHeavyComputation(task_id);
        
        // Tạo message để báo hoàn thành
        double* result_ptr = new double(result);
        auto msg = Message::obtain(shared_from_this(), HEAVY_TASK_COMPLETE_MSG, task_id, 0, 0);
        msg->obj = result_ptr;
        msg->obj_size = sizeof(double);
        sendMessage(msg);
    }
    
    void handleHeavyTaskComplete(int task_id, double result) {
        int completed = ++completed_tasks;
        if (completed == total_tasks) {
            // Post keep-alive task để sleep trong event loop
            auto msg = Message::obtain(shared_from_this(), KEEP_ALIVE_MSG, 0, 0, 0);
            sendMessage(msg);
        }
    }
    
    // LIGHT COMPUTATION - Target: 200-500ms
    double performLightComputation(int task_id) {
        double result = 0.0;
        
        // 1. Mathematical operations
        for(int i = 0; i < 1000000; i++) {
            result += std::sin(i * 0.00001) + std::cos(i * 0.00001) + std::sqrt(i + 1);
            if (i % 1000 == 0) {
                result += std::pow(i % 100, 1.5) + std::log(i + 1) * 0.1;
                result += std::atan(i * 0.0001) + std::tanh(i * 0.00001);
            }
        }
        
        // 2. Array operations
        std::vector<double> data(50000);
        for(size_t i = 0; i < data.size(); i++) {
            data[i] = std::sin(i + task_id) * std::cos(i * 0.001) + 
                     std::sqrt(i + 1) + std::pow(i % 50, 1.3);
        }
        
        // Multiple sorts
        for(int sort_round = 0; sort_round < 3; sort_round++) {
            std::sort(data.begin(), data.end());
            std::reverse(data.begin(), data.end());
        }
        result += std::accumulate(data.begin(), data.end(), 0.0) / data.size();
        
        // 3. Matrix operations
        const int matrix_size = 120;
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
        for(int n = 2; n < 8000; n++) {
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
    
    // HEAVY COMPUTATION - Target: 800-1500ms
    double performHeavyComputation(int task_id) {
        double result = 0.0;
        
        // 1. Massive mathematical operations
        for(int i = 0; i < 3000000; i++) {
            result += std::sin(i * 0.000001) * std::cos(i * 0.000001) + 
                     std::sqrt(i + 1) + std::pow((i % 100), 1.6);
            
            if (i % 1000 == 0) {
                result += std::log(i + 1) * std::exp(i * 0.0000001) + 
                         std::atan(i * 0.0001) * std::tanh(i * 0.0001);
            }
        }
        
        // 2. Large matrix operations
        const int matrix_size = 250;
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
        
        // 3. Prime calculation
        int prime_count = 0;
        for(int n = 2; n < 50000; n++) {
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
        
        return result + static_cast<double>(prime_count) * 0.1;
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cout << "Tiger Task Runner" << std::endl;
        std::cout << "Usage: " << argv[0] << " <heavy_tasks> <light_tasks>" << std::endl;
        std::cout << std::endl;
        std::cout << "Examples:" << std::endl;
        std::cout << "  " << argv[0] << " 10 2    # 10 heavy tasks + 2 light tasks" << std::endl;
        std::cout << "  " << argv[0] << " 5 8     # 5 heavy tasks + 8 light tasks" << std::endl;
        std::cout << "  " << argv[0] << " 0 10    # Only 10 light tasks" << std::endl;
        std::cout << "  " << argv[0] << " 3 0     # Only 3 heavy tasks" << std::endl;
        std::cout << std::endl;
        std::cout << "Program will sleep 10 seconds after all tasks complete" << std::endl;
        return 1;
    }
    
    int heavy_tasks = std::atoi(argv[1]);
    int light_tasks = std::atoi(argv[2]);
    
    // Validation
    if (heavy_tasks < 0 || heavy_tasks > 100) {
        std::cerr << "Heavy tasks must be 0-100" << std::endl;
        return 1;
    }
    if (light_tasks < 0 || light_tasks > 100) {
        std::cerr << "Light tasks must be 0-100" << std::endl;
        return 1;
    }
    if (heavy_tasks == 0 && light_tasks == 0) {
        std::cerr << "Must have at least 1 task" << std::endl;
        return 1;
    }
    
    try {
        auto looper = std::make_shared<SLLooper>();
        auto handler = std::make_shared<TigerTaskHandler>(looper);
        
        // Start looper in background thread
        std::thread looper_thread([looper]() {
            looper->loop();
        });
        
        // Give looper time to initialize
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        handler->startMixedTaskTest(heavy_tasks, light_tasks);
        
        // Wait for looper thread to complete
        if (looper_thread.joinable()) {
            looper_thread.join();
        }
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}