/**
 * @file SLLooper.cpp
 * @brief Implementation of SLLooper - main event loop coordinator
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #include <cstring>
 #include "SLLooper.h"
 #include "Handler.h"
 #include "Message.h"
 #include "EventQueue.h"
 #include "Debug.h"
 #include <iostream>
 #include <thread>
 #include <string>
 #include <atomic>
 #include "TimerManager.h"
 #include "Timer.h"
 #include "Awaitable.h"
 
 using namespace std;
 
 namespace swt {
 
 // ================= SLLooper Core Implementation =================
 
 SLLooper::SLLooper()
 {
     SLLOOPER_INFO("Constructor called");
     mEventQueue = std::make_shared<EventQueue>();
     mStarted = false;
     t1 = std::thread(&SLLooper::loop, this);
     SLLOOPER_INFO("Constructor finished");
 }
 
 void SLLooper::initializeTimerManager() {
     if (!mTimerManager) {
         SLLOOPER_DEBUG("Initializing TimerManager...");
         try {
             mTimerManager = std::make_unique<TimerManager>(weak_from_this());
             SLLOOPER_DEBUG("TimerManager initialized successfully");
         } catch (const std::exception& e) {
             SLLOOPER_ERROR_STREAM("Failed to initialize TimerManager: " << e.what());
             throw;
         }
     }
 }
 
 SLLooper::~SLLooper() {
     try {
         SLLOOPER_INFO("Destructor called");
         mStarted = false;
         if (mTimerManager) {
             SLLOOPER_DEBUG("Resetting TimerManager...");
             mTimerManager.reset();
         }
         mEventQueue->quit();
         if (t1.joinable()) {
             if (std::this_thread::get_id() != t1.get_id()) {
                 t1.join();
             } else {
                 t1.detach();
             }
         }
         SLLOOPER_INFO("Destructor finished");
     } catch (const std::exception& e) {
         SLLOOPER_ERROR_STREAM("Exception in destructor: " << e.what());
     } catch (...) {
         SLLOOPER_ERROR("Unknown exception in destructor");
     }
 }
 
 void SLLooper::exit() {
     SLLOOPER_DEBUG("exit() called");
     mStarted = false;
     mEventQueue->quit();
     SLLOOPER_DEBUG("exit() finished");
 }
 
 std::shared_ptr<EventQueue> SLLooper::getEventQueue() {
     return mEventQueue;
 }
 
 bool SLLooper::loop()
 {
     try {
         mStarted.store(true);
         SLLOOPER_INFO("start looper");
 
         int loop_count = 0;
         while (mStarted.load()) {
             loop_count++;
             auto item = mEventQueue->pollNext();
 
             if (!item) {
                 if (SLLOOPER_DEBUG_ENABLED && loop_count % 50 == 0) {
                     SLLOOPER_DEBUG_STREAM("pollNext timeout (cycle " << loop_count << ")");
                 }
                 if (mEventQueue->isQuit()) { 
                     break;
                 } else {
                     continue;
                 }
             }
 
             SLLOOPER_DEBUG_STREAM("Processing item (cycle " << loop_count 
                                  << ", type: " << (int)item->type << ")");
 
             auto& itemRef = item.value();
             if (itemRef.type == EventQueue::QueueItemType::MESSAGE) {
                 SLLOOPER_DEBUG("Processing message");
                 if (itemRef.message && itemRef.message->mHandler) {
                     itemRef.message->mHandler->dispatchMessage(itemRef.message);
                 }
             } else if (itemRef.type == EventQueue::QueueItemType::FUNCTION) {
                 SLLOOPER_DEBUG_STREAM("Executing function, task valid: " << itemRef.task.valid());
                 if (itemRef.task.valid()) {
                     try {
                         itemRef.task();
                         SLLOOPER_DEBUG("Function executed successfully");
                     } catch (const std::exception& e) {
                         SLLOOPER_ERROR_STREAM("Exception in function execution: " << e.what());
                     }
                 }
             }
 
             std::this_thread::sleep_for(std::chrono::milliseconds(1));
         }
 
         SLLOOPER_INFO("Exited main loop");
 
     } catch (const std::exception& e) {
         SLLOOPER_ERROR_STREAM("Exception in loop: " << e.what());
     }
 
     mStarted.store(false);
     SLLOOPER_INFO("Loop finished");
     return false;
 }
 
 // ========== TIMER API IMPLEMENTATIONS ==========
 
 Timer SLLooper::addTimer(std::function<void()> callback, uint64_t delay_ms) {
     SLLOOPER_DEBUG_STREAM("addTimer called with delay " << delay_ms << "ms");
     if (!callback) {
         SLLOOPER_ERROR("callback is null!");
         return Timer(0, weak_from_this());
     }
     initializeTimerManager();
     Timer timer(0, weak_from_this());
     TimerId id = createTimerInternal(std::move(callback), delay_ms, false, &timer.mCancelled);
     if (id == 0) {
         SLLOOPER_ERROR("Failed to create timer!");
         return timer;
     }
     timer.mId = id;
     SLLOOPER_DEBUG_STREAM("Created timer with ID " << id << ", active: " << timer.isActive());
     return timer;
 }
 
 Timer SLLooper::addPeriodicTimer(std::function<void()> callback, uint64_t interval_ms) {
     SLLOOPER_DEBUG_STREAM("addPeriodicTimer called with interval " << interval_ms << "ms");
     if (!callback) {
         SLLOOPER_ERROR("callback is null!");
         return Timer(0, weak_from_this());
     }
     initializeTimerManager();
     Timer timer(0, weak_from_this());
     TimerId id = createTimerInternal(std::move(callback), interval_ms, true, &timer.mCancelled);
     if (id == 0) {
         SLLOOPER_ERROR("Failed to create periodic timer!");
         return timer;
     }
     timer.mId = id;
     SLLOOPER_DEBUG_STREAM("Created periodic timer with ID " << id << ", active: " << timer.isActive());
     return timer;
 }
 
 void SLLooper::updateTimerCancelledPtr(TimerId id, std::atomic<bool>* newPtr) {
     SLLOOPER_DEBUG_STREAM("Updating cancelled pointer for timer " << id);
     if (mTimerManager) {
         mTimerManager->updateCancelledPtr(id, newPtr);
     } else {
         SLLOOPER_ERROR("TimerManager is null in updateTimerCancelledPtr!");
     }
 }
 
 TimerId SLLooper::createTimerInternal(std::function<void()> callback, uint64_t delay_ms, 
                                       bool periodic, std::atomic<bool>* cancelled) {
     SLLOOPER_DEBUG_STREAM("createTimerInternal called with delay " << delay_ms 
                          << "ms, periodic: " << periodic);
     if (!mTimerManager) {
         SLLOOPER_DEBUG("TimerManager is null, initializing...");
         initializeTimerManager();
     }
     if (!mTimerManager) {
         SLLOOPER_ERROR("TimerManager still null after initialization!");
         return 0;
     }
     TimerId id = mTimerManager->createTimer(std::move(callback), delay_ms, periodic, cancelled);
     SLLOOPER_DEBUG_STREAM("createTimerInternal returning ID " << id);
     return id;
 }
 
 bool SLLooper::cancelTimerInternal(TimerId id) {
     SLLOOPER_DEBUG_STREAM("cancelTimerInternal called for ID " << id);
     if (!mTimerManager) {
         SLLOOPER_ERROR("TimerManager is null in cancelTimerInternal!");
         return false;
     }
     bool result = mTimerManager->cancelTimer(id);
     SLLOOPER_DEBUG_STREAM("cancelTimerInternal result: " << result);
     return result;
 }
 
 bool SLLooper::hasTimerInternal(TimerId id) {
     if (!mTimerManager) return false;
     return mTimerManager->hasTimer(id);
 }
 
 bool SLLooper::restartTimerInternal(TimerId id, uint64_t delay_ms) {
     SLLOOPER_DEBUG_STREAM("restartTimerInternal called for ID " << id 
                          << " with delay " << delay_ms << "ms");
     if (!mTimerManager) {
         SLLOOPER_ERROR("TimerManager is null in restartTimerInternal!");
         return false;
     }
     bool result = mTimerManager->restartTimer(id, delay_ms);
     SLLOOPER_DEBUG_STREAM("restartTimerInternal result: " << result);
     return result;
 }
 
 size_t SLLooper::getActiveTimerCount() {
     if (!mTimerManager) {
         return 0;
     }
     size_t count = mTimerManager->getActiveTimerCount();
     return count;
 }
 
 // ================== Awaitable Implementations ===================
 
 // DelayAwaitable
void DelayAwaitable::await_suspend(std::coroutine_handle<> handle) const noexcept {
    auto self = const_cast<DelayAwaitable*>(this);
    SLLOOPER_DEBUG_STREAM("DelayAwaitable: Starting delay for " << mDelayMs << "ms");
    mLooper->postDelayed(mDelayMs, [self, handle]() {
        SLLOOPER_DEBUG("DelayAwaitable: Delay completed, resuming coroutine");
        self->setReady();
        handle.resume();
    });
}

// WorkAwaitable generic template
template<typename T>
void WorkAwaitable<T>::await_suspend(std::coroutine_handle<> handle) const noexcept {
    auto self = const_cast<WorkAwaitable*>(this);
    SLLOOPER_DEBUG("WorkAwaitable: Starting background work");
    std::thread([self, handle]() {
        try {
            auto result = self->getFunc()();
            self->setResult(result);
            SLLOOPER_DEBUG("WorkAwaitable: Work completed successfully with result");
        } catch (const std::exception& e) {
            SLLOOPER_ERROR_STREAM("WorkAwaitable: Exception in background work: " << e.what());
            self->setException(std::current_exception());
        } catch (...) {
            SLLOOPER_ERROR("WorkAwaitable: Unknown exception in background work");
            self->setException(std::current_exception());
        }
        self->getLooper()->post([handle]() {
            SLLOOPER_DEBUG("WorkAwaitable: Resuming coroutine on main thread");
            handle.resume();
        });
    }).detach();
}

// ✅ ĐÚNG SYNTAX cho specialization - KHÔNG có template<>
void WorkAwaitable<void>::await_suspend(std::coroutine_handle<> handle) const noexcept {
    auto self = const_cast<WorkAwaitable<void>*>(this);
    SLLOOPER_DEBUG("WorkAwaitable<void>: Starting background work");
    std::thread([self, handle]() {
        try {
            self->getFunc()();
            self->setResult();
            SLLOOPER_DEBUG("WorkAwaitable<void>: Work completed successfully");
        } catch (const std::exception& e) {
            SLLOOPER_ERROR_STREAM("WorkAwaitable<void>: Exception: " << e.what());
            self->setException(std::current_exception());
        } catch (...) {
            SLLOOPER_ERROR("WorkAwaitable<void>: Unknown exception");
            self->setException(std::current_exception());
        }
        self->getLooper()->post([handle]() {
            SLLOOPER_DEBUG("WorkAwaitable<void>: Resuming coroutine on main thread");
            handle.resume();
        });
    }).detach();
}

// PostAwaitable generic template
template<typename T>
void PostAwaitable<T>::await_suspend(std::coroutine_handle<> handle) const noexcept {
    auto self = const_cast<PostAwaitable<T>*>(this);
    SLLOOPER_DEBUG("PostAwaitable: Posting to main thread");
    mLooper->post([self, handle]() {
        try {
            auto result = self->getFunc()();
            self->setResult(result);
            SLLOOPER_DEBUG("PostAwaitable: Function executed successfully with result");
        } catch (const std::exception& e) {
            SLLOOPER_ERROR_STREAM("PostAwaitable: Exception: " << e.what());
            self->setException(std::current_exception());
        } catch (...) {
            SLLOOPER_ERROR("PostAwaitable: Unknown exception");
            self->setException(std::current_exception());
        }
        SLLOOPER_DEBUG("PostAwaitable: Resuming coroutine");
        handle.resume();
    });
}

// ✅ ĐÚNG SYNTAX cho specialization - KHÔNG có template<>
void PostAwaitable<void>::await_suspend(std::coroutine_handle<> handle) const noexcept {
    auto self = const_cast<PostAwaitable<void>*>(this);
    SLLOOPER_DEBUG("PostAwaitable<void>: Posting to main thread");
    mLooper->post([self, handle]() {
        try {
            self->getFunc()();
            self->setResult();
            SLLOOPER_DEBUG("PostAwaitable<void>: Function executed successfully");
        } catch (const std::exception& e) {
            SLLOOPER_ERROR_STREAM("PostAwaitable<void>: Exception: " << e.what());
            self->setException(std::current_exception());
        } catch (...) {
            SLLOOPER_ERROR("PostAwaitable<void>: Unknown exception");
            self->setException(std::current_exception());
        }
        SLLOOPER_DEBUG("PostAwaitable<void>: Resuming coroutine");
        handle.resume();
    });
}

// Template class instantiations
template class WorkAwaitable<int>;
template class WorkAwaitable<std::string>;
template class WorkAwaitable<double>;
template class WorkAwaitable<bool>;
template class WorkAwaitable<long>;
template class WorkAwaitable<float>;

template class PostAwaitable<int>;
template class PostAwaitable<std::string>;
template class PostAwaitable<double>;
template class PostAwaitable<bool>;
template class PostAwaitable<long>;
template class PostAwaitable<float>;

} // namespace swt