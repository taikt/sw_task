#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <vector>
#include <chrono>
#include "Awaitable.h"
#include <thread> 

using namespace swt;

/**
 * Simulate fetching user data
 */
Task<std::string> fetchUser(std::shared_ptr<SLLooper> looper, int userId) {
    co_await looper->awaitDelay(100); // Simulate network delay
    
    std::string userData = co_await looper->awaitWork([userId]() -> std::string {
        std::cout << "Fetching user " << userId << " from database..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return "User" + std::to_string(userId);
    });
    
    co_return userData;
}

/**
 * Process user data
 */
Task<std::string> processUser(std::shared_ptr<SLLooper> looper, const std::string& userData) {
    std::string processed = co_await looper->awaitWork([userData]() -> std::string {
        std::cout << "Processing " << userData << "..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
        return "Processed_" + userData;
    });
    
    co_return processed;
}

/**
 * Complex workflow with error handling
 */
Task<void> complexWorkflow(std::shared_ptr<SLLooper> looper) {
    try {
        std::cout << "\n=== Complex Workflow ===" << std::endl;
        
        std::vector<std::string> results;
        
        // Process multiple users concurrently (simulate)
        for (int i = 1; i <= 3; ++i) {
            std::cout << "Starting workflow for user " << i << std::endl;
            
            // Fetch user
            std::string user = co_await fetchUser(looper, i);
            
            // Process user
            std::string processed = co_await processUser(looper, user);
            
            results.push_back(processed);
            
            std::cout << "Completed workflow for: " << processed << std::endl;
        }
        
        // Final processing
        co_await looper->awaitWork([&results]() {
            std::cout << "Final processing of " << results.size() << " users" << std::endl;
            for (const auto& result : results) {
                std::cout << "  - " << result << std::endl;
            }
        });
        
        std::cout << "Complex workflow completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in complex workflow: " << e.what() << std::endl;
    }
}

/**
 * Error handling example
 */
Task<void> errorHandlingExample(std::shared_ptr<SLLooper> looper) {
    std::cout << "\n=== Error Handling Example ===" << std::endl;
    
    bool hasError = false;
    std::string errorMessage;
    
    try {
        // This will throw an exception
        int result = co_await looper->awaitWork([]() -> int {
            std::cout << "This work will fail..." << std::endl;
            throw std::runtime_error("Simulated error in background work");
            return 42;
        });
        
        std::cout << "This should not be reached: " << result << std::endl;
        
    } catch (const std::exception& e) {
        hasError = true;
        errorMessage = e.what();
        // ❌ KHÔNG được co_await ở đây
    }
    
    // ✅ co_await BÊN NGOÀI catch block
    if (hasError) {
        std::cout << "Caught expected error: " << errorMessage << std::endl;
        
        // Recovery work
        co_await looper->awaitDelay(100);
        std::cout << "Recovery completed" << std::endl;
    }
}

int main() {
    std::cout << "=== SW Task Advanced Coroutine Example ===" << std::endl;
    
    auto looper = std::make_shared<SLLooper>();
    
    // Start all tasks
    auto complexTask = complexWorkflow(looper);
    complexTask.start();
    
    auto errorTask = errorHandlingExample(looper);
    errorTask.start();
    
    // ✅ CHỈ chờ tasks hoàn thành, KHÔNG gọi looper->loop()
    auto startTime = std::chrono::steady_clock::now();
    while ((!complexTask.done() || !errorTask.done()) && 
           std::chrono::steady_clock::now() - startTime < std::chrono::seconds(15)) {        
        // ❌ XÓA DÒNG NÀY: if (!looper->loop()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        // ❌ XÓA: }
    }
    
    // ✅ Thoát looper khi xong
    looper->exit();
    
    std::cout << "\nAll examples completed!" << std::endl;
}