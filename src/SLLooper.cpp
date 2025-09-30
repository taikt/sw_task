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
 
 using namespace std;
 
 /**
  * @brief Constructor - initializes event loop and starts main thread
  * 
  * Creates EventQueue, initializes atomic flags, and starts the main
  * event loop thread. TimerManager is created lazily when first needed.
  * 
  * @note Must be created as shared_ptr for enable_shared_from_this functionality
  */
 SLLooper::SLLooper()
 {
     SLLOOPER_INFO("Constructor called");
     mEventQueue = std::make_shared<EventQueue>();
     mStarted = false;
     
     // Start main event loop thread
     t1 = std::thread(&SLLooper::loop, this);
     SLLOOPER_INFO("Constructor finished");
 }
 
 /**
  * @brief Initialize TimerManager lazily when first timer is requested
  * 
  * Creates TimerManager instance with weak reference to this SLLooper.
  * Uses lazy initialization to avoid unnecessary resource allocation
  * if no timers are used.
  * 
  * @throws std::exception if TimerManager creation fails
  */
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
 
 /**
  * @brief Destructor - performs orderly shutdown of event loop
  * 
  * Stops the event loop, cancels all timers, and joins the main thread.
  * Uses exception handling to ensure clean shutdown even if errors occur.
  */
 SLLooper::~SLLooper() {
     try {
         SLLOOPER_INFO("Destructor called");
         mStarted = false;
         
         // Reset TimerManager first to cancel all timers
         if (mTimerManager) {
             SLLOOPER_DEBUG("Resetting TimerManager...");
             mTimerManager.reset();
         }
         
         // Signal event queue to quit
         mEventQueue->quit();
         
         // Join main thread if possible
         if (t1.joinable()) {
             if (std::this_thread::get_id() != t1.get_id()) {
                 t1.join();
             } else {
                 // Avoid deadlock if destructor called from event loop thread
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
 
 /**
  * @brief Request event loop to exit gracefully
  * 
  * Signals the main event loop to stop processing and exit.
  * The loop will finish processing current messages before stopping.
  */
 void SLLooper::exit() {
     SLLOOPER_DEBUG("exit() called");
     mStarted = false;
     mEventQueue->quit();
     SLLOOPER_DEBUG("exit() finished");
 }
 
 /**
  * @brief Get access to the underlying event queue
  * @return std::shared_ptr<EventQueue> Shared pointer to the event queue
  * 
  * Provides direct access to the EventQueue for advanced operations.
  * Most users should prefer the high-level post() methods instead.
  * 
  * @note Thread-safe operation
  * @see post(), postDelayed()
  */
 std::shared_ptr<EventQueue> SLLooper::getEventQueue() {
     return mEventQueue;
 }
 
 /**
  * @brief Main event loop implementation
  * @return false when loop exits
  * 
  * Continuously processes messages and function tasks from the event queue.
  * Runs until mStarted becomes false or quit signal is received.
  * 
  * Loop behavior:
  * - Polls for next queue item with timeout
  * - Processes messages by calling handler->dispatchMessage()
  * - Executes function tasks directly
  * - Handles exceptions to prevent loop termination
  * - Reduces debug log spam during idle periods
  * 
  * @note Runs in dedicated thread created in constructor
  * @note Exception-safe - catches and logs all exceptions
  */
 bool SLLooper::loop()
 {
     try {
         mStarted.store(true);
         SLLOOPER_INFO("start looper");
 
         int loop_count = 0;
         while (mStarted.load()) {
             loop_count++;
             auto item = mEventQueue->pollNext();
             
             // Reduce log spam - only log every 50 cycles when no items
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
             
             // Small delay to prevent busy waiting
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
 
 /**
  * @brief Add one-shot timer with callback
  * @param callback Function to call when timer expires
  * @param delay_ms Delay in milliseconds before timer expiration
  * @return Timer RAII object for timer management
  * 
  * Creates a one-shot timer that fires once after the specified delay.
  * Uses lazy initialization for TimerManager to avoid unnecessary overhead.
  * 
  * Implementation flow:
  * 1. Validate callback parameter
  * 2. Initialize TimerManager if needed
  * 3. Create Timer object with initial state
  * 4. Call createTimerInternal to register with TimerManager
  * 5. Update Timer object with real timer ID
  * 
  * @note Timer automatically cancels when Timer object is destroyed
  * @note Thread-safe operation
  */
 Timer SLLooper::addTimer(std::function<void()> callback, uint64_t delay_ms) {
     SLLOOPER_DEBUG_STREAM("addTimer called with delay " << delay_ms << "ms");
     
     if (!callback) {
         SLLOOPER_ERROR("callback is null!");
         return Timer(0, weak_from_this());
     }
     
     // Initialize TimerManager first
     initializeTimerManager();
     
     // Create Timer object with temporary ID, then update with real ID
     Timer timer(0, weak_from_this());
     TimerId id = createTimerInternal(std::move(callback), delay_ms, false, &timer.mCancelled);
     
     if (id == 0) {
         SLLOOPER_ERROR("Failed to create timer!");
         return timer; // Return invalid timer
     }
     
     // Update timer with real ID
     timer.mId = id;
     
     SLLOOPER_DEBUG_STREAM("Created timer with ID " << id << ", active: " 
                          << timer.isActive());
     
     return timer;
 }
 
 /**
  * @brief Add periodic timer with callback
  * @param callback Function to call on each timer expiration
  * @param interval_ms Interval in milliseconds between expirations
  * @return Timer RAII object for timer management
  * 
  * Creates a periodic timer that fires repeatedly at the specified interval.
  * Uses the same implementation pattern as addTimer but with periodic=true.
  * 
  * @note Timer continues until cancelled or Timer object is destroyed
  * @note Thread-safe operation
  */
 Timer SLLooper::addPeriodicTimer(std::function<void()> callback, uint64_t interval_ms) {
     SLLOOPER_DEBUG_STREAM("addPeriodicTimer called with interval " << interval_ms << "ms");
     
     if (!callback) {
         SLLOOPER_ERROR("callback is null!");
         return Timer(0, weak_from_this());
     }
     
     // Initialize TimerManager first
     initializeTimerManager();
     
     // Same pattern as addTimer but with periodic=true
     Timer timer(0, weak_from_this());
     TimerId id = createTimerInternal(std::move(callback), interval_ms, true, &timer.mCancelled);
     
     if (id == 0) {
         SLLOOPER_ERROR("Failed to create periodic timer!");
         return timer;
     }
     
     timer.mId = id;
     
     SLLOOPER_DEBUG_STREAM("Created periodic timer with ID " << id << ", active: " 
                          << timer.isActive());
     
     return timer;
 }
 
 /**
  * @brief Update timer cancellation pointer for moved Timer objects
  * @param id Timer identifier
  * @param newPtr New pointer to atomic cancellation flag
  * 
  * Called when Timer objects are moved to update internal pointer
  * references for proper cancellation handling. This ensures that
  * timer cancellation works correctly after Timer move operations.
  * 
  * @note For internal use by Timer move constructor/assignment
  */
 void SLLooper::updateTimerCancelledPtr(TimerId id, std::atomic<bool>* newPtr) {
     SLLOOPER_DEBUG_STREAM("Updating cancelled pointer for timer " << id);
     if (mTimerManager) {
         mTimerManager->updateCancelledPtr(id, newPtr);
     } else {
         SLLOOPER_ERROR("TimerManager is null in updateTimerCancelledPtr!");
     }
 }
 
 // ========== INTERNAL TIMER API ==========
 
 /**
  * @brief Internal timer creation method
  * @param callback Function to call when timer expires
  * @param delay_ms Delay in milliseconds
  * @param periodic true for repeating timer, false for one-shot
  * @param cancelled Pointer to atomic cancellation flag
  * @return TimerId Unique timer identifier (0 if failed)
  * 
  * Low-level timer creation used internally by Timer objects.
  * Delegates to TimerManager after ensuring it's initialized.
  * 
  * @note For internal use by addTimer() and addPeriodicTimer()
  */
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
 
 /**
  * @brief Internal timer cancellation method
  * @param id Timer identifier to cancel
  * @return true if timer was found and cancelled
  * 
  * @note For internal use by Timer::cancel()
  */
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
 
 /**
  * @brief Check if timer exists in internal management
  * @param id Timer identifier to check
  * @return true if timer exists and is active
  * 
  * @note For internal use by Timer::isActive()
  */
 bool SLLooper::hasTimerInternal(TimerId id) {
     if (!mTimerManager) return false;
     return mTimerManager->hasTimer(id);
 }
 
 /**
  * @brief Restart existing timer with new delay
  * @param id Timer identifier to restart
  * @param delay_ms New delay in milliseconds
  * @return true if timer was found and restarted
  * 
  * @note For internal use by Timer::restart()
  */
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
 
 /**
  * @brief Get count of active timers
  * @return Number of currently active timers
  * 
  * Useful for debugging and monitoring timer usage.
  * Returns 0 if TimerManager is not initialized.
  */
 size_t SLLooper::getActiveTimerCount() {
     if (!mTimerManager) {
         // Don't log this one as it's called frequently
         return 0;
     }
     size_t count = mTimerManager->getActiveTimerCount();
     return count;
 }