#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>
#include <vector>
#include <memory>
#include "SLLooper.h"
#include "Timer.h"

using namespace swt;

// Các hằng số có thể thay đổi - SỬA LẠI
const int HEAVY_TASK_COUNT = 0;
const int LIGHT_TASK_COUNT = 10;
const int PERIODIC_TIMER_COUNT = 50;     // Giảm từ 200 → 50
const int TIMER_INTERVAL_MS = 2000;      // Tăng từ 10ms → 2s
const int MAIN_DURATION_SEC = 30;
const int FINAL_WAIT_SEC = 10;

class TimerTaskApp {
private:
    std::shared_ptr<SLLooper> mLooper;
    std::atomic<int> mCompletedHeavyTasks{0};
    std::atomic<int> mCompletedLightTasks{0};
    std::atomic<int> mTimerExecutions{0};
    std::vector<Timer> mTimers;
    std::atomic<bool> mAllTasksCompleted{false};

public:
    TimerTaskApp() {
        mLooper = std::make_shared<SLLooper>();
        mTimers.reserve(PERIODIC_TIMER_COUNT);
    }

    // Heavy task simulation - CPU intensive work
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

    // Light task simulation - simple work
    void executeLightTask(int taskId) {
        std::cout << "Light Task " << taskId << " started" << std::endl;
        
        // Simulate light work
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        
        int completed = ++mCompletedLightTasks;
        std::cout << "Light Task " << taskId << " completed (" << completed << "/" << LIGHT_TASK_COUNT << ")" << std::endl;
        
        checkAllTasksCompleted();
    }

    // Periodic timer callback - SỬA LẠI: LIGHT CALLBACK
    void timerCallback(int timerId) {
        int executions = ++mTimerExecutions;
        
        // Chỉ print mỗi 10 lần để giảm I/O
        if (executions % 10 == 0) {
            std::cout << "Timer " << timerId << " executed (total executions: " << executions << ")" << std::endl;
        }
        
        // REMOVE heavy computation - chỉ để light work
        // volatile long sum = 0;
        // for (int i = 0; i < 1000000; i++) {
        //     sum += i * i;
        // }
    }

    void checkAllTasksCompleted() {
        if (mCompletedHeavyTasks.load() == HEAVY_TASK_COUNT && 
            mCompletedLightTasks.load() == LIGHT_TASK_COUNT) {
            
            if (!mAllTasksCompleted.exchange(true)) {
                std::cout << "\n=== All tasks completed! Starting final wait period ===" << std::endl;
                
                // Schedule final exit after 10 seconds
                mLooper->postDelayed(FINAL_WAIT_SEC * 1000, [this]() {
                    std::cout << "Final wait period completed. Exiting..." << std::endl;
                    mLooper->exit();
                });
            }
        }
    }

    void run() {
        std::cout << "Starting Timer Task App..." << std::endl;
        std::cout << "Configuration:" << std::endl;
        std::cout << "- Heavy Tasks: " << HEAVY_TASK_COUNT << std::endl;
        std::cout << "- Light Tasks: " << LIGHT_TASK_COUNT << std::endl;
        std::cout << "- Periodic Timers: " << PERIODIC_TIMER_COUNT << std::endl;
        std::cout << "- Timer Interval: " << TIMER_INTERVAL_MS << "ms" << std::endl;
        std::cout << "- Main Duration: " << MAIN_DURATION_SEC << " seconds" << std::endl;
        std::cout << "- Final Wait: " << FINAL_WAIT_SEC << " seconds" << std::endl;
        std::cout << std::endl;

        // Start heavy tasks using postWork()
        std::cout << "Starting " << HEAVY_TASK_COUNT << " heavy tasks..." << std::endl;
        for (int i = 1; i <= HEAVY_TASK_COUNT; i++) {
            mLooper->postWork([this, i]() {
                executeHeavyTask(i);
            });
        }

        // Start light tasks using post()
        std::cout << "Starting " << LIGHT_TASK_COUNT << " light tasks..." << std::endl;
        for (int i = 1; i <= LIGHT_TASK_COUNT; i++) {
            mLooper->post([this, i]() {
                executeLightTask(i);
            });
        }

        // Create periodic timers
        std::cout << "Creating " << PERIODIC_TIMER_COUNT << " periodic timers..." << std::endl;
        for (int i = 1; i <= PERIODIC_TIMER_COUNT; i++) {
            Timer timer = mLooper->addPeriodicTimer([this, i]() {
                timerCallback(i);
            }, TIMER_INTERVAL_MS);
            
            mTimers.push_back(std::move(timer));
        }

        // Schedule main duration timeout
        mLooper->postDelayed(MAIN_DURATION_SEC * 1000, [this]() {
            std::cout << "\n=== Main duration (" << MAIN_DURATION_SEC << "s) completed ===" << std::endl;
            
            // Cancel all timers
            std::cout << "Cancelling all periodic timers..." << std::endl;
            for (auto& timer : mTimers) {
                timer.cancel();
            }
            
            // If tasks are not completed yet, wait for them
            if (!mAllTasksCompleted.load()) {
                std::cout << "Waiting for remaining tasks to complete..." << std::endl;
            } else {
                std::cout << "Tasks already completed, continuing final wait..." << std::endl;
            }
        });

        std::cout << "\nStarting event loop..." << std::endl;
        
        // Start the event loop
        while (mLooper->loop()) {
            // Event loop running
        }

        std::cout << "\nFinal Statistics:" << std::endl;
        std::cout << "- Heavy tasks completed: " << mCompletedHeavyTasks.load() << "/" << HEAVY_TASK_COUNT << std::endl;
        std::cout << "- Light tasks completed: " << mCompletedLightTasks.load() << "/" << LIGHT_TASK_COUNT << std::endl;
        std::cout << "- Total timer executions: " << mTimerExecutions.load() << std::endl;
        std::cout << "Program finished." << std::endl;
    }
};

int main() {
    try {
        TimerTaskApp app;
        app.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}