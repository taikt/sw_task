#include <chrono>
#include "Message.h"
#include "Promise.h"
#include "EventQueue.h"
#include <mutex>
#include <thread>
#include <iostream>
using namespace std;
#include <string>

namespace swt {

EventQueue::EventQueue() :
        mCurrentMessage(NULL),
        mStarted(false), mQuit(false)
{
}

EventQueue::~EventQueue() {
    try {
        // std::cout << "EventQueue destructor called" << std::endl;
        // ...existing cleanup code...
        // std::cout << "EventQueue destructor finished" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Exception in EventQueue destructor: " << e.what() << std::endl;
    } catch (...) {
        std::cerr << "Unknown exception in EventQueue destructor" << std::endl;
    }
}

bool EventQueue::enqueueMessage(const std::shared_ptr<Message>& message, int64_t whenUs)
{
    // cout<<"message queue: receive msg\n";
    
    {
        std::lock_guard<std::mutex> lock(iMutex);
        
        // Insert at correct position instead of sort
        auto insertPos = std::upper_bound(mQueue.begin(), mQueue.end(), whenUs,
            [](int64_t time, const QueueItem& item) {
                return time < item.whenUs;
            });
        mQueue.emplace(insertPos, message, whenUs);
    }

    // ALWAYS notify, regardless of mStarted
    mQueueChanged.notify_one();
    return true;
}

// Kiểm tra queue đã quit chưa
bool EventQueue::isQuit(){
    return mQuit;
}

std::optional<EventQueue::QueueItem> EventQueue::pollNext()
{
    // std::cout << "EventQueue: pollNext() called" << std::endl;
    
    int poll_attempt = 0;
    while (!mQuit) {
        poll_attempt++;
        mStarted = true;
        
        // std::cout << "EventQueue: poll attempt " << poll_attempt << std::endl;
        
        {
            std::unique_lock<std::mutex> lk(iMutex);
            // std::cout << "EventQueue: checking queue, size: " << mQueue.size() << std::endl;
            
            // CHECK QUEUE FIRST before going to wait
            if (!mQueue.empty()) {
                auto currentTime = uptimeMicros();
                // std::cout << "EventQueue: found item, time: " << mQueue.front().whenUs 
                //          << ", current time: " << currentTime << std::endl;
                         
                if (mQueue.front().whenUs <= currentTime) {
                    auto item = std::move(mQueue.front());
                    mQueue.pop_front();
                    // std::cout << "EventQueue: returning immediate item, type: " << (int)item.type << std::endl;
                    return item;
                }
            }
            
            if (mQueue.empty()) {
                // std::cout << "EventQueue: queue empty, entering wait..." << std::endl;
                
                // SHORTER timeout to avoid infinite wait
                auto result = mQueueChanged.wait_for(lk, std::chrono::milliseconds(500), [this]{
                    // std::cout << "EventQueue: wait predicate - queue size: " << mQueue.size() << ", quit: " << mQuit << std::endl;
                    return (!mQueue.empty() || mQuit);
                });
                
                if (!result) {
                    // std::cout << "EventQueue: wait timed out after 500ms! Attempt: " << poll_attempt << std::endl;
                    
                    // SAFETY: Return null after multiple timeouts
                    if (poll_attempt >= 20) {  // 20 * 500ms = 10 seconds max
                        // std::cout << "EventQueue: too many timeouts, returning null" << std::endl;
                        return std::nullopt;
                    }
                    continue;
                }
                
                // std::cout << "EventQueue: woke up from wait" << std::endl;
            }
            
            if (mQuit) {
                // std::cout << "EventQueue: quit requested" << std::endl;
                break;
            }
            
            // RECHECK queue after wait
            if (!mQueue.empty()) {
                auto currentTime = uptimeMicros();
                // std::cout << "EventQueue: after wait, front item time: " << mQueue.front().whenUs 
                //          << ", current time: " << currentTime << std::endl;
                         
                if (mQueue.front().whenUs <= currentTime) {
                    auto item = std::move(mQueue.front());
                    mQueue.pop_front();
                    // std::cout << "EventQueue: returning post-wait item, type: " << (int)item.type << std::endl;
                    return item;
                } else {
                    // Future item - wait for it
                    auto waitTime = mQueue.front().whenUs - currentTime;
                    // std::cout << "EventQueue: waiting " << waitTime << " microseconds for future item" << std::endl;
                    
                    if (waitTime > 500000) waitTime = 500000; // Cap at 500ms
                    
                    mQueueChanged.wait_for(lk, std::chrono::microseconds(waitTime));
                    // std::cout << "EventQueue: finished waiting for future item" << std::endl;
                }
            }
        }
    }
    
    // std::cout << "EventQueue: returning nullopt" << std::endl;
    return std::nullopt;
}

std::shared_ptr<Message> EventQueue::poll()
{
    // Legacy method - convert to new format
    auto item = pollNext();
    if (item && item->type == QueueItemType::MESSAGE) {
        return item->message;
    }
    return nullptr;
}

bool EventQueue::hasMessage(const std::shared_ptr<Handler>& h, int32_t what, void* obj)
{
    if (h == NULL) {
        return false;
    }
    std::lock_guard<std::mutex> lock(iMutex);
    
    for (const auto& item : mQueue) {
        if (item.type == QueueItemType::MESSAGE && 
            item.message->mHandler == h && 
            item.message->what == what &&
            (obj == NULL || item.message->obj == obj)) {
            return true;
        }
    }
    return false;
}

void EventQueue::quit()
{
    {
        std::lock_guard<std::mutex> lock(iMutex);
        mQuit = true;
    }
    mQueueChanged.notify_all();
}

int64_t EventQueue::uptimeMicros()
{
    auto now = std::chrono::steady_clock::now();
    auto dur = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch());
    return dur.count();
}



// Explicit instantiations
template swt::Promise<int> swt::EventQueue::enqueuePromise<int>();
template swt::Promise<void> swt::EventQueue::enqueuePromise<void>();
template swt::Promise<std::string> swt::EventQueue::enqueuePromise<std::string>();

} // namespace swt