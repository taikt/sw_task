#include "TimerManager.h"
#include "SLLooper.h"
#include "Debug.h"
#include <iostream>
#include <chrono>
#include <errno.h>
#include <cstring>

namespace swt {

#ifndef TIMER_USE_TIMERFD_EPOLL
// Static members for sigev_thread backend
std::unordered_map<TimerId, TimerManager*> TimerManager::sTimerManagerMap;
std::mutex TimerManager::sManagerMapMutex;
#endif

TimerManager::TimerManager(std::weak_ptr<SLLooper> looper) 
    : mLooper(looper) {
    
    TIMER_DEBUG_STREAM("Initializing TimerManager with backend: " << getBackendName());
    
#ifdef TIMER_USE_TIMERFD_EPOLL
    // Initialize timerfd+epoll backend
    mEpollFd = epoll_create1(EPOLL_CLOEXEC);
    if (mEpollFd == -1) {
        TIMER_ERROR_STREAM("Failed to create epoll fd: " << strerror(errno));
        throw std::runtime_error("Failed to create epoll fd");
    }
    
    TIMER_DEBUG_STREAM("Created epoll fd: " << mEpollFd);
    
    // Start timer thread
    mTimerThread = std::thread(&TimerManager::timerThreadFunc, this);
    TIMER_DEBUG("Timer thread started");
#else
    // Sigev_thread backend doesn't need special initialization
    TIMER_DEBUG("Sigev_thread backend initialized");
#endif
}

TimerManager::~TimerManager() {
    TIMER_INFO("Destructor called");
    mRunning = false;
    
    // Cleanup all timers
    {
        std::lock_guard<std::mutex> lock(mTimersMutex);
        TIMER_DEBUG_STREAM("Cleaning up " << mTimers.size() << " timers");
        for (auto& [id, timerInfo] : mTimers) {
#ifdef TIMER_USE_TIMERFD_EPOLL
            close(timerInfo.fd);
#else
            timer_delete(timerInfo.timer);
            // Remove from global map
            std::lock_guard<std::mutex> mapLock(sManagerMapMutex);
            sTimerManagerMap.erase(id);
#endif
        }
        mTimers.clear();
    }
    
#ifdef TIMER_USE_TIMERFD_EPOLL
    if (mTimerThread.joinable()) {
        mTimerThread.join();
        TIMER_DEBUG("Timer thread joined");
    }
    
    if (mEpollFd != -1) {
        close(mEpollFd);
        TIMER_INFO("Epoll fd closed");
    }
#else
    TIMER_DEBUG("Sigev_thread cleanup completed");
#endif
}

TimerId TimerManager::createTimer(std::function<void()> callback, uint64_t delay_ms, 
                                 bool periodic, std::atomic<bool>* cancelled) {
    if (!callback) {
        TIMER_ERROR("callback is null!");
        return 0;
    }
    
    TimerId id = mNextId++;
    TIMER_DEBUG_STREAM("Creating " << getBackendName() << " timer " << id 
                      << " with delay " << delay_ms << "ms, periodic: " << periodic);
    
#ifdef TIMER_USE_TIMERFD_EPOLL
    // Timerfd+epoll implementation
    int timer_fd = createTimerFd(delay_ms, periodic);
    if (timer_fd == -1) {
        TIMER_ERROR_STREAM("Failed to create timerfd for timer " << id 
                          << ": " << strerror(errno));
        return 0;
    }
    
    TIMER_DEBUG_STREAM("Created timerfd " << timer_fd << " for timer " << id);
    
    // Add to epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.u64 = id;
    
    if (epoll_ctl(mEpollFd, EPOLL_CTL_ADD, timer_fd, &ev) == -1) {
        TIMER_ERROR_STREAM("Failed to add timer " << id << " to epoll: " 
                          << strerror(errno));
        close(timer_fd);
        return 0;
    }
    
    TIMER_DEBUG_STREAM("Added timer " << id << " to epoll successfully");
    
    // Store timer info
    {
        std::lock_guard<std::mutex> lock(mTimersMutex);
        TimerInfo info;
        info.fd = timer_fd;
        info.callback = std::move(callback);
        info.periodic = periodic;
        info.interval_ms = delay_ms;
        info.id = id;
        info.cancelled = cancelled;
        
        mTimers[id] = std::move(info);
        TIMER_DEBUG_STREAM("Stored timer " << id << " info. Total timers: " 
                          << mTimers.size());
    }
    
#else
    // Sigev_thread implementation
    // Store timer info first (needed for callback)
    {
        std::lock_guard<std::mutex> lock(mTimersMutex);
        TimerInfo info;
        info.timer = (timer_t)0;  // Will be set later
        info.timerId = id;
        info.callback = std::move(callback);
        info.periodic = periodic;
        info.interval_ms = delay_ms;
        info.id = id;
        info.cancelled = cancelled;
        
        mTimers[id] = std::move(info);
        
        // Add to global map for callback lookup
        std::lock_guard<std::mutex> mapLock(sManagerMapMutex);
        sTimerManagerMap[id] = this;
    }
    
    // Create timer
    timer_t timer = createSigevTimer(id);
    if (timer == (timer_t)-1) {
        TIMER_ERROR_STREAM("Failed to create sigev timer for timer " << id);
        std::lock_guard<std::mutex> lock(mTimersMutex);
        mTimers.erase(id);
        std::lock_guard<std::mutex> mapLock(sManagerMapMutex);
        sTimerManagerMap.erase(id);
        return 0;
    }
    
    // Update timer_t in TimerInfo
    {
        std::lock_guard<std::mutex> lock(mTimersMutex);
        mTimers[id].timer = timer;
    }
    
    // Start timer
    updateSigevTimer(timer, delay_ms, periodic);
    
    TIMER_DEBUG_STREAM("Created and started sigev timer " << id);
#endif
    
    return id;
}

#ifdef TIMER_USE_TIMERFD_EPOLL

void TimerManager::timerThreadFunc() {
    TIMER_DEBUG("Timer thread starting...");
    const int MAX_EVENTS = 64;
    struct epoll_event events[MAX_EVENTS];
    
    int loop_count = 0;
    while (mRunning) {
        loop_count++;
        
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
                        
                        // Read to clear event
                        uint64_t exp;
                        ssize_t bytes = read(it->second.fd, &exp, sizeof(exp));
                        TIMER_DEBUG_STREAM("Read " << bytes << " bytes from cancelled timer " << id);
                        
                        // Cleanup immediately
                        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, it->second.fd, nullptr);
                        close(it->second.fd);
                        mTimers.erase(it);
                        continue;
                    }
                    
                    timerInfo = it->second;
                    shouldProcess = true;
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
                handleTimerExpired(timerInfo);
                
                // Cleanup non-periodic timers
                if (!timerInfo.periodic) {
                    TIMER_DEBUG_STREAM("Cleaning up one-shot timer " << id);
                    cleanupTimer(id);
                }
            }
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
        its.it_interval = its.it_value;
        TIMER_DEBUG_STREAM("Setting periodic timer: " << its.it_value.tv_sec 
                          << "s " << its.it_value.tv_nsec << "ns");
    } else {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        TIMER_DEBUG_STREAM("Setting one-shot timer: " << its.it_value.tv_sec 
                          << "s " << its.it_value.tv_nsec << "ns");
    }
    
    if (timerfd_settime(timer_fd, 0, &its, nullptr) == -1) {
        TIMER_ERROR_STREAM("timerfd_settime failed: " << strerror(errno));
        close(timer_fd);
        return -1;
    }
    
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
    }
}

#else

timer_t TimerManager::createSigevTimer(TimerId id) {
    timer_t timer;
    
    struct sigevent sev;
    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = &TimerManager::sigevTimerCallback;
    sev.sigev_notify_attributes = nullptr;
    sev.sigev_value.sival_ptr = reinterpret_cast<void*>(id); // Pass timer ID
    
    if (timer_create(CLOCK_MONOTONIC, &sev, &timer) == -1) {
        TIMER_ERROR_STREAM("timer_create failed: " << strerror(errno));
        return (timer_t)-1;
    }
    
    return timer;
}

void TimerManager::updateSigevTimer(timer_t timer, uint64_t delay_ms, bool periodic) {
    struct itimerspec its;
    its.it_value.tv_sec = delay_ms / 1000;
    its.it_value.tv_nsec = (delay_ms % 1000) * 1000000;
    
    if (periodic) {
        its.it_interval = its.it_value;
        TIMER_DEBUG_STREAM("Setting periodic sigev timer: " << its.it_value.tv_sec 
                          << "s " << its.it_value.tv_nsec << "ns");
    } else {
        its.it_interval.tv_sec = 0;
        its.it_interval.tv_nsec = 0;
        TIMER_DEBUG_STREAM("Setting one-shot sigev timer: " << its.it_value.tv_sec 
                          << "s " << its.it_value.tv_nsec << "ns");
    }
    
    if (timer_settime(timer, 0, &its, nullptr) == -1) {
        TIMER_ERROR_STREAM("timer_settime failed: " << strerror(errno));
    }
}

void TimerManager::sigevTimerCallback(sigval_t sv) {
    TimerId id = reinterpret_cast<TimerId>(sv.sival_ptr);
    
    TIMER_DEBUG_STREAM("Sigev timer callback for timer " << id);
    
    // Find TimerManager for this timer
    TimerManager* manager = nullptr;
    {
        std::lock_guard<std::mutex> lock(sManagerMapMutex);
        auto it = sTimerManagerMap.find(id);
        if (it != sTimerManagerMap.end()) {
            manager = it->second;
        }
    }
    
    if (!manager) {
        TIMER_DEBUG_STREAM("Timer manager not found for timer " << id);
        return;
    }
    
    TimerInfo timerInfo;
    bool shouldProcess = false;
    
    {
        std::lock_guard<std::mutex> lock(manager->mTimersMutex);
        auto it = manager->mTimers.find(id);
        if (it != manager->mTimers.end()) {
            // Check cancelled
            if (it->second.cancelled && it->second.cancelled->load()) {
                TIMER_DEBUG_STREAM("Sigev timer " << id << " is cancelled, cleaning up");
                timer_delete(it->second.timer);
                manager->mTimers.erase(it);
                std::lock_guard<std::mutex> mapLock(sManagerMapMutex);
                sTimerManagerMap.erase(id);
                return;
            }
            
            timerInfo = it->second;
            shouldProcess = true;
            
            // Cleanup non-periodic timers
            if (!timerInfo.periodic) {
                TIMER_DEBUG_STREAM("Cleaning up one-shot sigev timer " << id);
                timer_delete(it->second.timer);
                manager->mTimers.erase(it);
                std::lock_guard<std::mutex> mapLock(sManagerMapMutex);
                sTimerManagerMap.erase(id);
            }
        }
    }
    
    if (shouldProcess) {
        manager->handleTimerExpired(timerInfo);
    }
}

#endif

bool TimerManager::cancelTimer(TimerId id) {
    TIMER_DEBUG_STREAM("Cancelling " << getBackendName() << " timer " << id);
    std::lock_guard<std::mutex> lock(mTimersMutex);
    
    auto it = mTimers.find(id);
    if (it == mTimers.end()) {
        TIMER_DEBUG_STREAM("Timer " << id << " not found for cancellation");
        return false;
    }
    
#ifdef TIMER_USE_TIMERFD_EPOLL
    epoll_ctl(mEpollFd, EPOLL_CTL_DEL, it->second.fd, nullptr);
    close(it->second.fd);
#else
    timer_delete(it->second.timer);
    std::lock_guard<std::mutex> mapLock(sManagerMapMutex);
    sTimerManagerMap.erase(id);
#endif
    
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
    TIMER_DEBUG_STREAM("Restarting " << getBackendName() << " timer " << id 
                      << " with delay " << delay_ms << "ms");
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
    
#ifdef TIMER_USE_TIMERFD_EPOLL
    updateTimerFd(it->second.fd, delay_ms, false);
#else
    updateSigevTimer(it->second.timer, delay_ms, false);
#endif
    
    it->second.interval_ms = delay_ms;
    it->second.periodic = false;
    
    TIMER_DEBUG_STREAM("Timer " << id << " restarted successfully");
    return true;
}

size_t TimerManager::getActiveTimerCount() {
    std::lock_guard<std::mutex> lock(mTimersMutex);
    return mTimers.size();
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

void TimerManager::handleTimerExpired(const TimerInfo& timerInfo) {
    TIMER_DEBUG_STREAM("Handling expired timer " << timerInfo.id);
    
    // Double-check cancelled before post callback
    if (timerInfo.cancelled && timerInfo.cancelled->load()) {
        TIMER_DEBUG_STREAM("Timer " << timerInfo.id << " cancelled, skipping callback");
        return;
    }
    
    // Post callback to SLLooper main thread
    if (auto looper = mLooper.lock()) {
        TIMER_DEBUG_STREAM("Posting callback for timer " << timerInfo.id << " to main thread");
        
        auto result = looper->post([callback = timerInfo.callback, cancelled = timerInfo.cancelled, id = timerInfo.id]() {
            TIMER_DEBUG_STREAM("Executing callback for timer " << id << " in main thread");
            
            // Triple-check cancelled in main thread
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
#ifdef TIMER_USE_TIMERFD_EPOLL
        epoll_ctl(mEpollFd, EPOLL_CTL_DEL, it->second.fd, nullptr);
        close(it->second.fd);
#else
        timer_delete(it->second.timer);
        std::lock_guard<std::mutex> mapLock(sManagerMapMutex);
        sTimerManagerMap.erase(id);
#endif
        mTimers.erase(it);
        TIMER_DEBUG_STREAM("Timer " << id << " cleaned up. Remaining: " 
                          << mTimers.size());
    }
}

} // namespace swt