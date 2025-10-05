#include "SLLooper.h"
#include "Task.h"
#include "Awaitable.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <string>

using namespace swt;

/**
 * Simulate fetching data asynchronously using co_await
 */
Task<int> fetchData(std::shared_ptr<SLLooper> looper) {
    // Execute on background thread
    int result = co_await looper->awaitWork([]() -> int {
        std::cout << "Fetching data..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        return 7;
    });
    
    co_return result;
}

/**
 * Process step 1: multiply by 3
 */
Task<int> processStep1(std::shared_ptr<SLLooper> looper, int value) {
    // Execute on main thread
    int result = co_await looper->awaitPost([value]() -> int {
        std::cout << "Step 1: Got value = " << value << std::endl;
        return value * 3;
    });
    
    co_return result;
}

/**
 * Process step 2: convert to string
 */
Task<std::string> processStep2(std::shared_ptr<SLLooper> looper, int value) {
    // Execute on background thread
    std::string result = co_await looper->awaitWork([value]() -> std::string {
        std::cout << "Step 2: Value after multiply = " << value << std::endl;
        return std::to_string(value);
    });
    
    co_return result;
}

/**
 * Process step 3: final output
 */
Task<int> processStep3(std::shared_ptr<SLLooper> looper, const std::string& str) {
    // Execute on main thread
    int result = co_await looper->awaitPost([str]() -> int {
        std::cout << "Step 3: Final string = " << str << std::endl;
        return 0;
    });
    
    co_return result;
}

/**
 * Main workflow using coroutines
 */
Task<void> dataProcessingWorkflow(std::shared_ptr<SLLooper> looper) {
    try {
        // Step 0: Fetch data
        int data = co_await fetchData(looper);
        
        // Step 1: Process data (multiply)
        int processed1 = co_await processStep1(looper, data);
        
        // Step 2: Convert to string
        std::string processed2 = co_await processStep2(looper, processed1);
        
        // Step 3: Final output
        int final_result = co_await processStep3(looper, processed2);
        
        std::cout << "Workflow completed with result: " << final_result << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in workflow: " << e.what() << std::endl;
    }
}

/**
 * Alternative: Chain everything in one coroutine (more concise)
 */
Task<void> chainedWorkflow(std::shared_ptr<SLLooper> looper) {
    try {
        std::cout << "\n=== Chained Workflow ===" << std::endl;
        
        // Chain all operations in sequence
        int data = co_await looper->awaitWork([]() -> int {
            std::cout << "Fetching data..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(5000));
            return 7;
        });
        
        int step1 = co_await looper->awaitPost([data]() -> int {
            std::cout << "Step 1: Got value = " << data << std::endl;
            return data * 3;
        });
        std::cout<<"step1="<<step1<<std::endl;
        std::string step2 = co_await looper->awaitWork([step1]() -> std::string {
            std::cout << "Step 2: Value after multiply = " << step1 << std::endl;
            return std::to_string(step1);
        });
        
        int final_result = co_await looper->awaitPost([step2]() -> int {
            std::cout << "Step 3: Final string = " << step2 << std::endl;
            return 0;
        });
        
        std::cout << "Chained workflow completed with result: " << final_result << std::endl;
        
    } catch (const std::exception& e) {
        std::cout << "Error in chained workflow: " << e.what() << std::endl;
    }
}

int main() {
    std::cout << "=== Fetch Data Coroutine Example ===" << std::endl;
    
    auto looper = std::make_shared<SLLooper>();
    
    // Run both workflows
    // std::cout << "=== Step-by-Step Workflow ===" << std::endl;
    // auto workflow1 = dataProcessingWorkflow(looper);
    // workflow1.start();
    
    auto workflow2 = chainedWorkflow(looper);  
    workflow2.start();
    std::cout << "Workflow started, waiting for completion..." << std::endl;
    
    // Wait for completion
    
    std::this_thread::sleep_for(std::chrono::seconds(10));
    
    std::cout << "\nAll workflows completed!" << std::endl;
    return 0;
}