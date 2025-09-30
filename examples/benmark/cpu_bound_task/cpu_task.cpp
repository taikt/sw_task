// sw_task_cpu_test.cpp - CPU-bound test using SW Task framework
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

class CPUTaskHandler : public Handler {
private:
    std::shared_ptr<SLLooper> mSLLooper;  // Lưu reference đến looper
    std::atomic<int> completed_tasks{0};
    double total_result{0.0};
    std::mutex result_mutex; 
    std::chrono::high_resolution_clock::time_point start_time;
    int total_tasks;
    
    // Heavy CPU computation: Fibonacci
    uint64_t fibonacci(int n) {
        if (n <= 1) return n;
        return fibonacci(n-1) + fibonacci(n-2);
    }
    
    // Matrix multiplication
    double matrixMultiply(int size = 200) {
        std::vector<std::vector<double>> a(size, std::vector<double>(size));
        std::vector<std::vector<double>> b(size, std::vector<double>(size));
        std::vector<std::vector<double>> c(size, std::vector<double>(size, 0.0));
        
        // Initialize matrices
        for(int i = 0; i < size; i++) {
            for(int j = 0; j < size; j++) {
                a[i][j] = sin(i + j);
                b[i][j] = cos(i * j + 1);
            }
        }
        
        // Matrix multiplication
        for(int i = 0; i < size; i++) {
            for(int j = 0; j < size; j++) {
                for(int k = 0; k < size; k++) {
                    c[i][j] += a[i][k] * b[k][j];
                }
            }
        }
        
        return c[size/2][size/2];
    }
    
    // Prime number calculation
    bool isPrime(uint64_t n) {
        if (n < 2) return false;
        for (uint64_t i = 2; i * i <= n; i++) {
            if (n % i == 0) return false;
        }
        return true;
    }
    
    int countPrimes(uint64_t start, uint64_t end) {
        int count = 0;
        for (uint64_t i = start; i < end; i++) {
            if (isPrime(i)) count++;
        }
        return count;
    }
    
public:
    CPUTaskHandler(std::shared_ptr<SLLooper>& looper) : Handler(looper) {
        mSLLooper = looper;  // Lưu reference để sử dụng sau
    }
    
    void startTest(int num_tasks) {
        total_tasks = num_tasks;
        start_time = std::chrono::high_resolution_clock::now();
        
        std::cout << "SW Task CPU-bound test starting..." << std::endl;
        std::cout << "Tasks: " << num_tasks << std::endl;
        std::cout << "PID: " << getpid() << std::endl;
        
        // Send messages to trigger CPU tasks
        for(int i = 0; i < num_tasks; i++) {
            auto msg = obtainMessage(1, i); // what=1 for CPU task, arg1=task_id
            sendMessage(msg);
        }
    }
    
    void handleMessage(const std::shared_ptr<Message>& message) override {
        if (message->what == 1) {
            // CPU task execution
            int task_id = message->arg1;
            
            std::cout << "Task " << task_id << " starting on thread " 
                     << std::this_thread::get_id() << std::endl;
            
            // Heavy CPU work combination
            auto fib = fibonacci(35);              // ~2-3 seconds of CPU
            auto matrix = matrixMultiply(150);     // Matrix ops
            auto primes = countPrimes(10000 + task_id*1000, 11000 + task_id*1000);  // Prime counting
            
            // Additional mathematical operations
            double result = 0.0;
            for(int j = 0; j < 50000; j++) {
                result += sin(j) * cos(j) + sqrt(j + 1) + log(j + 2);
            }
            
            double task_result = fib + matrix + primes + result;
            
            std::cout << "Task " << task_id << " completed on thread " << std::this_thread::get_id() 
                     << " (Fib: " << fib << ", Matrix: " << matrix 
                     << ", Primes: " << primes << ")" << std::endl;
            
            // Update results thread-safely
            {
                std::lock_guard<std::mutex> lock(result_mutex);
                total_result += task_result;
            }
            
            int completed = ++completed_tasks;
            std::cout << "Task result: " << task_result 
                     << " (completed: " << completed << "/" << total_tasks << ")" << std::endl;
            
            // When all tasks are completed, print results
            if (completed == total_tasks) {
                printResults();
                mSLLooper->exit();  // Sử dụng mSLLooper thay vì mLooper.lock()
            }
        }
    }
    
private:
    void printResults() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
        
        // Print results
        std::cout << "\n=== SW Task CPU-bound Test Results ===" << std::endl;
        std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;
        std::cout << "Tasks completed: " << completed_tasks.load() << "/" << total_tasks << std::endl;
        std::cout << "Average time per task: " << duration.count() / (double)total_tasks << " ms" << std::endl;
        std::cout << "Throughput: " << (total_tasks * 1000.0) / duration.count() << " tasks/second" << std::endl;
        std::cout << "Total result sum: " << total_result << std::endl;
        std::cout << "Test completed!" << std::endl;
    }
};

int main(int argc, char* argv[]) {
    int num_tasks = 20;  // Default
    
    if (argc >= 2) {
        num_tasks = std::atoi(argv[1]);
    }
    
    std::cout << "SW Task CPU-bound Performance Test" << std::endl;
    std::cout << "Usage: " << argv[0] << " [num_tasks]" << std::endl;
    std::cout << "Using Handler messages for CPU-bound task execution" << std::endl;
    std::cout << "Hardware threads available: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << std::endl;
    
    try {
        auto looper = std::make_shared<SLLooper>();
        auto handler = std::make_shared<CPUTaskHandler>(looper);
        
        handler->startTest(num_tasks);
        looper->loop();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}