#include "SLLooper.h"
#include "Handler.h"
#include "Message.h"
#include "TimerManager.h"
#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <atomic>
#include <memory>

// Messages
#define HEAVY_TASK_MSG 1001
#define LIGHT_TASK_MSG 1002  
#define TIMER_MSG 1003
#define FINAL_EXIT_MSG 1004
#define MAIN_TIMEOUT_MSG 1005

// Constants - SỬA LẠI giống SW Task
const int HEAVY_TASK_COUNT = 0;
const int LIGHT_TASK_COUNT = 10;
const int PERIODIC_TIMER_COUNT = 50;     // Giảm từ 200 → 50
const int TIMER_INTERVAL_MS = 2000;      // Tăng từ 10ms → 2s
const int MAIN_DURATION_SEC = 30;
const int FINAL_WAIT_SEC = 10;

class TigerTaskHandler : public Handler {
private:
    std::atomic<int> mCompletedHeavyTasks{0};
    std::atomic<int> mCompletedLightTasks{0};
    std::atomic<int> mTimerExecutions{0};
    std::atomic<bool> mAllTasksCompleted{false};
    std::atomic<bool> mShouldExit{false}; // Thêm flag riêng để kiểm soát exit
    std::shared_ptr<TimerManager> mTimerManager;
    std::vector<timer_t> mActiveTimers;
    std::shared_ptr<SLLooper> mLooperRef; // Lưu lại looper

public:
    TigerTaskHandler(std::shared_ptr<SLLooper>& looper) : Handler(looper), mLooperRef(looper) {
        // DON'T create TimerManager in constructor - will cause bad_weak_ptr
        mActiveTimers.reserve(PERIODIC_TIMER_COUNT);
    }

    // Initialize TimerManager after object is fully constructed
    void initialize() {
        mTimerManager = std::make_shared<TimerManager>(shared_from_this());
    }

    void handleMessage(const std::shared_ptr<Message>& msg) override {
        switch (msg->what) {
            case HEAVY_TASK_MSG:
                executeHeavyTask(msg->arg1);
                break;
            case LIGHT_TASK_MSG:
                executeLightTask(msg->arg1);
                break;
            case TIMER_MSG:
                timerCallback(msg->arg1);
                break;
            case FINAL_EXIT_MSG:
                std::cout << "Final wait period completed. Exiting..." << std::endl;
                mShouldExit = true; // Set flag để exit
                mLooperRef->exit(); // Vẫn gọi exit() của framework
                break;
            case MAIN_TIMEOUT_MSG:
                handleMainTimeout();
                break;
            default:
                break;
        }
    }

    void startApp() {
        std::cout << "Starting Tiger Timer Task App..." << std::endl;
        std::cout << "Configuration:" << std::endl;
        std::cout << "- Heavy Tasks: " << HEAVY_TASK_COUNT << std::endl;
        std::cout << "- Light Tasks: " << LIGHT_TASK_COUNT << std::endl;
        std::cout << "- Periodic Timers: " << PERIODIC_TIMER_COUNT << std::endl;
        std::cout << "- Timer Interval: " << TIMER_INTERVAL_MS << "ms" << std::endl;
        std::cout << "- Main Duration: " << MAIN_DURATION_SEC << " seconds" << std::endl;
        std::cout << "- Final Wait: " << FINAL_WAIT_SEC << " seconds" << std::endl;
        std::cout << std::endl;

        // Start heavy tasks
        std::cout << "Starting " << HEAVY_TASK_COUNT << " heavy tasks..." << std::endl;
        for (int i = 1; i <= HEAVY_TASK_COUNT; i++) {
            auto msg = obtainMessage(HEAVY_TASK_MSG, i);
            sendMessage(msg);
        }

        // Start light tasks  
        std::cout << "Starting " << LIGHT_TASK_COUNT << " light tasks..." << std::endl;
        for (int i = 1; i <= LIGHT_TASK_COUNT; i++) {
            auto msg = obtainMessage(LIGHT_TASK_MSG, i);
            sendMessage(msg);
        }

        // Create periodic timers
        std::cout << "Creating " << PERIODIC_TIMER_COUNT << " periodic timers..." << std::endl;
        for (int i = 1; i <= PERIODIC_TIMER_COUNT; i++) {
            // Schedule first timer execution
            scheduleTimerExecution(i);
        }

        // Schedule main timeout
        auto timeoutMsg = obtainMessage(MAIN_TIMEOUT_MSG);
        sendMessageDelayed(timeoutMsg, MAIN_DURATION_SEC * 1000);

        std::cout << "\nStarting event loop..." << std::endl;
    }

    // Thêm hàm check shouldExit
    bool shouldExit() const {
        return mShouldExit.load();
    }

private:
    void executeHeavyTask(int taskId) {
        std::cout << "Heavy Task " << taskId << " started" << std::endl;
        
        // Simulate CPU intensive work
        volatile long sum = 0;
        for (int i = 0; i < 50000000; i++) {
            sum += i * i;
        }
        
        int completed = ++mCompletedHeavyTasks;
        std::cout << "Heavy Task " << taskId << " completed (" << completed << "/" << HEAVY_TASK_COUNT << ")" << std::endl;
        
        checkAllTasksCompleted();
    }

    void executeLightTask(int taskId) {
        std::cout << "Light Task " << taskId << " started" << std::endl;
        
        // Simulate light work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        int completed = ++mCompletedLightTasks;
        std::cout << "Light Task " << taskId << " completed (" << completed << "/" << LIGHT_TASK_COUNT << ")" << std::endl;
        
        checkAllTasksCompleted();
    }

    void timerCallback(int timerId) {
        int executions = ++mTimerExecutions;
        
        // Chỉ print mỗi 10 lần để giảm I/O - SỬA LẠI
        if (executions % 10 == 0) {
            std::cout << "Timer " << timerId << " executed (total executions: " << executions << ")" << std::endl;
        }
        
        // REMOVE heavy computation - SỬA LẠI: chỉ để light work
        // volatile long sum = 0;
        // for (int i = 0; i < 1000000; i++) {
        //     sum += i * i;
        // }

        // Schedule next execution
        scheduleTimerExecution(timerId);
    }

    void scheduleTimerExecution(int timerId) {
        auto msg = obtainMessage(TIMER_MSG, timerId);
        sendMessageDelayed(msg, TIMER_INTERVAL_MS);
    }

    void checkAllTasksCompleted() {
        if (mCompletedHeavyTasks.load() == HEAVY_TASK_COUNT && 
            mCompletedLightTasks.load() == LIGHT_TASK_COUNT) {
            
            if (!mAllTasksCompleted.exchange(true)) {
                std::cout << "\n=== All tasks completed! Starting final wait period ===" << std::endl;
                
                // Schedule final exit
                auto exitMsg = obtainMessage(FINAL_EXIT_MSG);
                sendMessageDelayed(exitMsg, FINAL_WAIT_SEC * 1000);
            }
        }
    }

    void handleMainTimeout() {
        std::cout << "\n=== Main duration (" << MAIN_DURATION_SEC << "s) completed ===" << std::endl;
        std::cout << "Stopping periodic timers..." << std::endl;
        
        // In Tiger framework, we just stop scheduling new timer messages
        // The existing delayed messages will still execute
        
        if (!mAllTasksCompleted.load()) {
            std::cout << "Waiting for remaining tasks to complete..." << std::endl;
        } else {
            std::cout << "Tasks already completed, continuing final wait..." << std::endl;
        }
    }

public:
    void printFinalStats() {
        std::cout << "\nFinal Statistics:" << std::endl;
        std::cout << "- Heavy tasks completed: " << mCompletedHeavyTasks.load() << "/" << HEAVY_TASK_COUNT << std::endl;
        std::cout << "- Light tasks completed: " << mCompletedLightTasks.load() << "/" << LIGHT_TASK_COUNT << std::endl;
        std::cout << "- Total timer executions: " << mTimerExecutions.load() << std::endl;
        std::cout << "Program finished." << std::endl;
    }
};

int main() {
    try {
        auto looper = std::make_shared<SLLooper>();
        auto handler = std::make_shared<TigerTaskHandler>(looper);
        
        // Initialize TimerManager AFTER handler is managed by shared_ptr
        handler->initialize();
        
        // Start the application
        handler->startApp();
        
        // Run the event loop với điều kiện kép để tránh "start looper" sau exit
        while (looper->loop() && !handler->shouldExit()) {
            // Event loop running
        }
        
        // Print final statistics
        handler->printFinalStats();
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}