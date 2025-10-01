// tiger_cpu_test.cpp - CPU-bound test using Tiger Looper (single-threaded)
#include "SLLooper.h"
#include "Handler.h"
#include "Message.h"
#include "Refbase.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <cmath>
#include <unistd.h>

#define MSG_CPU_TASK 1001
#define MSG_START_TEST 1002

class TigerCPUTestHandler : public Handler {
private:
    std::atomic<int> completed_tasks{0};
    std::chrono::high_resolution_clock::time_point start_time;
    int total_tasks;
    double total_result = 0.0;
    
    // Same heavy CPU functions as SW Task
    uint64_t fibonacci(int n) {
        if (n <= 1) return n;
        return fibonacci(n-1) + fibonacci(n-2);
    }
    
    double matrixMultiply(int size = 200) {
        std::vector<std::vector<double>> a(size, std::vector<double>(size));
        std::vector<std::vector<double>> b(size, std::vector<double>(size));
        std::vector<std::vector<double>> c(size, std::vector<double>(size, 0.0));
        
        for(int i = 0; i < size; i++) {
            for(int j = 0; j < size; j++) {
                a[i][j] = sin(i + j);
                b[i][j] = cos(i * j + 1);
            }
        }
        
        for(int i = 0; i < size; i++) {
            for(int j = 0; j < size; j++) {
                for(int k = 0; k < size; k++) {
                    c[i][j] += a[i][k] * b[k][j];
                }
            }
        }
        
        return c[size/2][size/2];
    }
    
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
    TigerCPUTestHandler(std::shared_ptr<SLLooper>& looper) : Handler(looper) {}
    
    void handleMessage(const std::shared_ptr<Message>& msg) override {
        switch(msg->what) {
            case MSG_START_TEST: {
                total_tasks = msg->arg1;
                start_time = std::chrono::high_resolution_clock::now();
                
                std::cout << "Tiger Looper CPU-bound test starting..." << std::endl;
                std::cout << "Tasks: " << total_tasks << std::endl;
                std::cout << "PID: " << getpid() << std::endl;
                std::cout << "All tasks will run sequentially on single event thread" << std::endl;
                
                // Send first task message
                sendMessage(obtainMessage(MSG_CPU_TASK, 0));
                break;
            }
            
            case MSG_CPU_TASK: {
                int task_id = msg->arg1;
                
                // Perform same heavy CPU work as SW Task
                // BUT everything runs on single event thread (sequential)
                auto fib = fibonacci(35);              // ~2-3 seconds of CPU
                auto matrix = matrixMultiply(150);     // Matrix ops  
                auto primes = countPrimes(10000 + task_id*1000, 11000 + task_id*1000);
                
                // Additional mathematical operations
                double result = 0.0;
                for(int j = 0; j < 50000; j++) {
                    result += sin(j) * cos(j) + sqrt(j + 1) + log(j + 2);
                }
                
                total_result += fib + matrix + primes + result;
                completed_tasks++;
                
                std::cout << "Task " << task_id << " completed on event thread " 
                         << std::this_thread::get_id() 
                         << " (Fib: " << fib << ", Matrix: " << matrix 
                         << ", Primes: " << primes << ")" << std::endl;
                
                // Send next task or finish
                if (task_id + 1 < total_tasks) {
                    sendMessage(obtainMessage(MSG_CPU_TASK, task_id + 1));
                } else {
                    // All tasks completed
                    auto end_time = std::chrono::high_resolution_clock::now();
                    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
                    
                    std::cout << "\n=== Tiger Looper CPU-bound Test Results ===" << std::endl;
                    std::cout << "Total execution time: " << duration.count() << " ms" << std::endl;
                    std::cout << "Tasks completed: " << completed_tasks.load() << "/" << total_tasks << std::endl;
                    std::cout << "Average time per task: " << duration.count() / (double)total_tasks << " ms" << std::endl;
                    std::cout << "Throughput: " << (total_tasks * 1000.0) / duration.count() << " tasks/second" << std::endl;
                    std::cout << "Total result sum: " << total_result << std::endl;
                    std::cout << "Test completed!" << std::endl;
                    
                    // Exit after short delay
                    sendMessageDelayed(obtainMessage(9999), 1000);
                }
                break;
            }
            
            case 9999:
                // Exit signal
                exit(0);
                break;
        }
    }
    
    void startTest(int num_tasks) {
        sendMessage(obtainMessage(MSG_START_TEST, num_tasks));
    }
};

int main(int argc, char* argv[]) {
    int num_tasks = 20;  // Default
    
    if (argc >= 2) {
        num_tasks = std::atoi(argv[1]);
    }
    
    std::cout << "Tiger Looper CPU-bound Performance Test" << std::endl;
    std::cout << "Usage: " << argv[0] << " [num_tasks]" << std::endl;
    std::cout << "All tasks run sequentially on single event thread" << std::endl;
    std::cout << "Hardware threads available: " << std::thread::hardware_concurrency() << std::endl;
    std::cout << std::endl;
    
    try {
        // Initialize Tiger Looper framework
        auto looper = std::make_shared<SLLooper>();
        auto handler = std::make_shared<TigerCPUTestHandler>(looper);
        
        // Start event loop in separate thread
        std::thread looper_thread([&looper]() {
            looper->loop();
        });
        
        // Wait a bit for loop to start
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        // Start the test
        handler->startTest(num_tasks);
        
        // Join the looper thread (will exit when test completes)
        looper_thread.join();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}