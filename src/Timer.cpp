#include "Timer.h"
#include "SLLooper.h"
#include <iostream>

namespace swt {

Timer::Timer(TimerId id, std::weak_ptr<SLLooper> looper)
    : mId(id), mLooper(looper), mCancelled(false), mMoved(false) {
    // std::cout << "[Timer] Created timer " << mId << std::endl;
}

Timer::Timer(Timer&& other) noexcept {
    moveFrom(std::move(other));
    // std::cout << "[Timer] Move constructed timer " << mId << " (from " 
    //           << other.mId << ")" << std::endl;
}

Timer& Timer::operator=(Timer&& other) noexcept {
    if (this != &other) {
        // Cancel current timer trước khi move
        if (!mMoved.load() && !mCancelled.load()) {
            cancel();
        }
        
        moveFrom(std::move(other));
        // std::cout << "[Timer] Move assigned timer " << mId << " (from " 
        //           << other.mId << ")" << std::endl;
    }
    return *this;
}

void Timer::moveFrom(Timer&& other) noexcept {
    mId = other.mId;
    mLooper = other.mLooper;
    mCancelled.store(other.mCancelled.load());
    mMoved.store(false);  // This object is not moved
    

    if (auto looper = mLooper.lock()) {
        looper->updateTimerCancelledPtr(mId, &mCancelled);
        // std::cout << "[Timer] Updated TimerManager pointer for moved timer " << mId << std::endl;
    }
    
    // Mark source as moved
    other.mMoved.store(true);
    other.mCancelled.store(true);  // Prevent double cancellation
}

Timer::~Timer() {
    // Chỉ cancel nếu object này chưa bị moved
    if (!mMoved.load() && !mCancelled.load()) {
        // std::cout << "[Timer] Destructor cancelling timer " << mId << std::endl;
        cancel();
    } else if (mMoved.load()) {
        // std::cout << "[Timer] Destructor skipping moved timer " << mId << std::endl;
    }
}

void Timer::cancel() {
    if (mMoved.load()) {
        std::cout << "[Timer] Cannot cancel moved timer " << mId << std::endl;
        return;
    }
    
    if (mCancelled.exchange(true)) {
        return; // Already cancelled
    }
    
    if (auto looper = mLooper.lock()) {
        std::cout << "[Timer] Cancelling timer " << mId << std::endl;
        looper->cancelTimerInternal(mId);  
    }
}

bool Timer::isActive() const {
    if (mMoved.load() || mCancelled.load()) {
        return false;
    }
    
    if (auto looper = mLooper.lock()) {
        return looper->hasTimerInternal(mId);  
    }
    return false;
}

void Timer::restart(uint64_t delay_ms) {
    if (mMoved.load()) {
        // std::cout << "[Timer] Cannot restart moved timer " << mId << std::endl;
        return;
    }
    
    if (auto looper = mLooper.lock()) {
        // std::cout << "[Timer] Restarting timer " << mId << " with delay "
                //   << delay_ms << "ms" << std::endl;
        
        if (looper->restartTimerInternal(mId, delay_ms)) {  
            mCancelled.store(false);
        }
    }
}

} // namespace swt