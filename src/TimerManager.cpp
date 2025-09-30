#include "TimerManager.h"
#include "SLLooper.h"
#include "Debug.h"
#include <iostream>
#include <chrono>
#include <errno.h>
#include <cstring>

TimerManager::TimerManager(std::weak_ptr<SLLooper> looper) 
    : mLooper(looper) {
    
    TIMER_DEBUG("Initializing...");
    
    // Tạo epoll fd
    mEpollFd = epoll_create1(EPOLL_CLOEXEC);
    if (mEpollFd == -1) {
        TIMER_ERROR_STREAM("Failed to create epoll fd: " << strerror(errno));
        throw std::runtime_error("Failed to create epoll fd");
    }
    
    TIMER_DEBUG_STREAM("Created epoll fd: " << mEpollFd);
    
    // Khởi tạo timer thread
    mTimerThread = std::thread(&TimerManager::timerThreadFunc, this);
    TIMER_DEBUG("Timer thread started");
}

TimerManager::~TimerManager() {
    TIMER_INFO("Destructor called");
    mRunning = false;
    
    // Cleanup all timers
    {
        std::lock_guard<std::mutex> lock(mTimersMutex);
        TIMER_DEBUG_STREAM("Cleaning up " << mTimers.size() << " timers");
        for (auto& [id, timerInfo] : mTimers) {
            close(timerInfo.fd);
        }
        mTimers.clear();
    }
    
    if (mTimerThread.joinable()) {
        mTimerThread.join();
        TIMER_DEBUG("Timer thread joined");
    }
    
    if (mEpollFd != -1) {
        close(mEpollFd);
        TIMER_INFO("Epoll fd closed");
    }
}

TimerId TimerManager::createTimer(std::function<void()> callback, uint64_t delay_ms, 
                                 bool periodic, std::atomic<bool>* cancelled) {
    if (!callback) {
        TIMER_ERROR("callback is null!");
        return 0;
    }
    
    TimerId id = mNextId++;
    TIMER_DEBUG_STREAM("Creating timer " << id << " with delay " << delay_ms 
                      << "ms, periodic: " << periodic);
    
    // Tạo timerfd
    int timer_fd = createTimerFd(delay_ms, periodic);
    if (timer_fd == -1) {
        TIMER_ERROR_STREAM("Failed to create timerfd for timer " << id 
                          << ": " << strerror(errno));
        return 0;
    }
    
    TIMER_DEBUG_STREAM("Created timerfd " << timer_fd << " for timer " << id);
    
    // Thêm vào epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u64 = id; // Store timer id trong epoll data
    
    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, timer_fd, &ev) == -1) {
        TIMER_ERROR_STREAM("Failed to add timer " << id << " to epoll: " 
                          << strerror(errno));
        close(timer_fd);
        return 0;
    }
    
    TIMER_DEBUG_STREAM("Added timer " << id << " to epoll successfully");
    
    // Lưu timer info
    {
        std::lock_guard<std::mutex> lock(mTimersMutex);
        mTimers[id] = {timer_fd, std::move(callback), periodic, delay_ms, id, cancelled};
        TIMER_DEBUG_STREAM("Stored timer " << id << " info. Total timers: " 
                          << mTimers.size());
    }
    
    return id;
}

bool TimerManager::cancelTimer(TimerId id) {
    TIMER_DEBUG_STREAM("Cancelling timer " << id);
    std::lock_guard<std::mutex> lock(mTimersMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        TIMER_DEBUG_STREAM("Timer " << id << " not found for cancellation");
        return false;
    }
    
    // Remove từ epoll và close fd
    epoll_ctl(mEpollFd, EPOLL_CTL_DEL, it->second.fd, nullptr);
    close(it->second.fd);
    mTimers.erase(it);
    
    TIMER_DEBUG_STREAM("Timer " << id << " cancelled successfully. Remaining: " 
                      << mTimers.size());
    return true;
}

bool TimerManager::hasTimer(TimerId id) {
    std::lock_guard<std::mutex> lock(mTimersMutex);
    bool exists = mTimers.find(id) != mTimers.end();
    TIMER_DEBUG_STREAM("Timer " << id << " exists: " << exists);
    return exists;
}

bool TimerManager::restartTimer(TimerId id, uint64_t delay_ms) {
    TIMER_DEBUG_STREAM("Restarting timer " << id << " with delay " << delay_ms << "ms");
    std::lock_guard<std::mutex> lock(mTimersMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        TIMER_DEBUG_STREAM("Timer " << id << " not found for restart");
        return false;
    }
    
    // Reset cancelled flag
    if (it->second.cancelled) {
        it->second.cancelled->store(false);
    }
    
    // Update timer
    updateTimerFd(it->second.fd, delay_ms, false);
    it->second.interval_ms = delay_ms;
    it->second.periodic = false;
    
    TIMER_DEBUG_STREAM("Timer " << id << " restarted successfully");
    return true;
}

size_t TimerManager::getActiveTimerCount() {
    std::lock_guard<std::mutex> lock(mTimersMutex);
    return mTimers.size();
}

void TimerManager::timerThreadFunc() {
    TIMER_DEBUG("Timer thread starting...");
    const int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];
    
    int loop_count = 0;
    while (mRunning) {
        loop_count++;
        
        // Reduce timeout for better responsiveness
        int nfds = epoll_wait(mEpollFd, events, MAX_EVENTS, 100); // 100ms timeout
        
        if (nfds == -1) {
            if (errno == EINTR) {
                TIMER_DEBUG("epoll_wait interrupted, continuing...");
                continue;
            }
            TIMER_ERROR_STREAM("epoll_wait error: " << strerror(errno));
            break;
        }
        
        if (nfds > 0) {
            TIMER_DEBUG_STREAM("Got " << nfds << " timer events (loop " << loop_count << ")");
        }
        
        for (int i = 0; i < nfds; i++) {
            TimerId id = events[i].data.u64;
            TIMER_DEBUG_STREAM("Processing timer event for ID " << id);
            
            bool shouldProcess = false;
            TimerInfo timerInfo;
            
            {
                std::lock_guard<std::mutex> lock(mTimersMutex);
                auto it = mTimers.find(id);
                if (it != mTimers.end()) {
                    // Check cancelled first
                    if (it->second.cancelled && it->second.cancelled->load()) {
                        TIMER_DEBUG_STREAM("Timer " << id << " is cancelled, cleaning up");
                        
                        // Read để clear event
                        uint64_t exp;
                        ssize_t bytes = read(it->second.fd, &exp, sizeof(exp));
                        TIMER_DEBUG_STREAM("Read " << bytes << " bytes from cancelled timer " << id);
                        
                        // Cleanup ngay
                        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, it->second.fd, nullptr);
                        close(it->second.fd);
                        mTimers.erase(it);
                        continue;
                    }
                    
                    timerInfo = it->second;
                    shouldProcess = true;
                    TIMER_DEBUG_STREAM("Timer " << id << " should process, periodic: " 
                                      << timerInfo.periodic);
                }
            }
            
            if (shouldProcess) {
                // Read event
                uint64_t exp;
                ssize_t bytes = read(timerInfo.fd, &exp, sizeof(exp));
                TIMER_DEBUG_STREAM("Read " << bytes << " bytes from timer " << id 
                                  << ", expirations: " << exp);
                
                // Final check before callback
                if (timerInfo.cancelled && timerInfo.cancelled->load()) {
                    TIMER_DEBUG_STREAM("Timer " << id << " cancelled before callback");
                    cleanupTimer(id);
                    continue;
                }
                
                // Handle timer expired
                TIMER_DEBUG_STREAM("Calling handleTimerExpired for timer " << id);
                handleTimerExpired(timerInfo);
                
                // Cleanup non-periodic timers
                if (!timerInfo.periodic) {
                    TIMER_DEBUG_STREAM("Cleaning up one-shot timer " << id);
                    cleanupTimer(id);
                } else {
                    TIMER_DEBUG_STREAM("Keeping periodic timer " << id << " alive");
                }
            }
        }
        
        // Log active timer count periodically (only when debug enabled)
        if (TIMER_DEBUG_ENABLED && loop_count % 100 == 0) {
            std::lock_guard<std::mutex> lock(mTimersMutex);
            TIMER_DEBUG_STREAM("Loop " << loop_count << ", active timers: " 
                              << mTimers.size());
        }
    }
    
    TIMER_DEBUG_STREAM("Timer thread exiting after " << loop_count << " loops");
}

int TimerManager::createTimerFd(uint64_t delay_ms, bool periodic) {
    TIMER_DEBUG_STREAM("Creating timerfd with delay " << delay_ms 
                      << "ms, periodic: " << periodic);
              
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (timer_fd == -1) {
        TIMER_ERROR_STREAM("timerfd_create failed: " << strerror(errno));
        return -1;
    }
    
    struct itimerspec its;
    its.it_value.tv_sec = delay_ms / 1000;
    its.it_value.tv_nsec = (delay_ms % 1000) * 1000000;
    
    if (periodic) {
        its.it_interval = its.it_value; // Repeat interval
        TIMER_DEBUG_STREAM("Setting periodic timer: " << its.it_value.tv_sec 
                          << "s " << its.it_value.tv_nsec << "ns");
    } else {
        its.it_interval.tv_sec = 0;     // One-shot
        its.it_interval.tv_nsec = 0;
        TIMER_DEBUG_STREAM("Setting one-shot timer: " << its.it_value.tv_sec 
                          << "s " << its.it_value.tv_nsec << "ns");
    }
    
    if (timerfd_settime(timer_fd, 0, &its, nullptr) == -1) {
        TIMER_ERROR_STREAM("timerfd_settime failed: " << strerror(errno));
        close(timer_fd);
        return -1;
    }
    
    TIMER_DEBUG_STREAM("Timerfd " << timer_fd << " configured successfully");
    return timer_fd;
}

void TimerManager::updateTimerFd(int fd, uint64_t delay_ms, bool periodic) {
    TIMER_DEBUG_STREAM("Updating timerfd " << fd << " with delay " << delay_ms 
                      << "ms, periodic: " << periodic);
              
    struct itimerspec its;
    its.it_value.tv_sec = delay_ms / 1000;
    its.it_value.tv_nsec = (delay_ms % 1000) * 1000000;
    
    if (periodic) {
        its.it_interval = its.it_value;
    } else {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
    }
    
    if (timerfd_settime(fd, 0, &its, nullptr) == -1) {
        TIMER_ERROR_STREAM("Failed to update timerfd " << fd << ": " 
                          << strerror(errno));
    } else {
        TIMER_DEBUG_STREAM("Timerfd " << fd << " updated successfully");
    }
}

void TimerManager::handleTimerExpired(const TimerInfo& timerInfo) {
    TIMER_DEBUG_STREAM("Handling expired timer " << timerInfo.id);
    
    // Double-check cancelled trước khi post callback
    if (timerInfo.cancelled && timerInfo.cancelled->load()) {
        TIMER_DEBUG_STREAM("Timer " << timerInfo.id << " cancelled, skipping callback");
        return;
    }
    
    // Post callback vào SLLooper main thread
    if (auto looper = mLooper.lock()) {
        TIMER_DEBUG_STREAM("Posting callback for timer " << timerInfo.id << " to main thread");
        
        auto result = looper->post([callback = timerInfo.callback, cancelled = timerInfo.cancelled, id = timerInfo.id]() {
            TIMER_DEBUG_STREAM("Executing callback for timer " << id << " in main thread");
            
            // Triple-check cancelled trong main thread
            if (cancelled && cancelled->load()) {
                TIMER_DEBUG_STREAM("Timer " << id << " cancelled in main thread, skipping");
                return;
            }
            
            try {
                callback();
                TIMER_DEBUG_STREAM("Callback for timer " << id << " executed successfully");
            } catch (const std::exception& e) {
                TIMER_ERROR_STREAM("Timer " << id << " callback exception: " << e.what());
            }
        });
        
        TIMER_DEBUG_STREAM("Posted callback for timer " << timerInfo.id 
                          << " (future valid: " << result.valid() << ")");
    } else {
        TIMER_ERROR_STREAM("Failed to lock looper for timer " << timerInfo.id);
    }
}

void TimerManager::cleanupTimer(TimerId id) {
    TIMER_DEBUG_STREAM("Cleaning up timer " << id);
    std::lock_guard<std::mutex> lock(mTimersMutex);
    auto it = mTimers.find(id);
    if (it != mTimers.end()) {
        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, it->second.fd, nullptr);
        close(it->second.fd);
        mTimers.erase(it);
        TIMER_DEBUG_STREAM("Timer " << id << " cleaned up. Remaining: " 
                          << mTimers.size());
    } else {
        TIMER_DEBUG_STREAM("Timer " << id << " not found for cleanup");
    }
}

void TimerManager::updateCancelledPtr(TimerId id, std::atomic<bool>* newPtr) {
    TIMER_DEBUG_STREAM("Updating cancelled pointer for timer " << id);
    std::lock_guard<std::mutex> lock(mTimersMutex);
    
    auto it = mTimers.find(id);
    if (it != mTimers.end()) {
        it->second.cancelled = newPtr;
        TIMER_DEBUG_STREAM("Updated cancelled pointer for timer " << id << " successfully");
    } else {
        TIMER_DEBUG_STREAM("Timer " << id << " not found for pointer update");
    }
}