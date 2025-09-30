/**
 * @file Timer.h
 * @brief RAII timer wrapper with move semantics for safe timer management
 * @author Tran Anh Tai
 * @date 9/2025
 * @version 1.0.0
 */

 #pragma once

 #include <memory>
 #include <functional>
 #include <atomic>
 
 namespace swt {
 class SLLooper;
 
 /**
  * @typedef TimerId
  * @brief Unique identifier type for timer instances
  * 
  * 64-bit unsigned integer providing unique identification for each timer
  * created by TimerManager. Used for internal timer tracking and management.
  */
 using TimerId = uint64_t;
 
 /**
  * @class Timer
  * @brief RAII timer wrapper with boost-style API and move semantics
  * 
  * Timer provides a safe, RAII-based interface for managing kernel timers.
  * Key characteristics:
  * - **RAII lifecycle**: Automatic cleanup when Timer object is destroyed
  * - **Move-only semantics**: Prevents accidental copying and ensures unique ownership
  * - **Thread-safe operations**: Safe cancellation and status checking across threads
  * - **Boost-style API**: Familiar interface for timer operations
  * 
  * The Timer class acts as a handle to an underlying kernel timer managed by
  * TimerManager. It ensures proper cleanup and provides safe access to timer
  * operations even when the timer is moved between objects.
  * 
  * @code{.cpp}
  * auto looper = std::make_shared<SLLooper>();
  * 
  * // Create one-shot timer
  * Timer timer = looper->addTimer([]() { 
  *     std::cout << "Timer fired!" << std::endl; 
  * }, 1000);
  * 
  * // Check if active
  * if (timer.isActive()) {
  *     std::cout << "Timer is running" << std::endl;
  * }
  * 
  * // Cancel if needed
  * timer.cancel();
  * 
  * // Move timer to another variable
  * Timer movedTimer = std::move(timer);
  * // Original timer is now in moved-from state
  * @endcode
  * 
  * @note Timer objects are move-only and cannot be copied
  * @warning Accessing moved-from Timer objects results in no-op operations
  * 
  * @see \ref swt::SLLooper::addTimer "SLLooper::addTimer", \ref swt::Timer "Timer", \ref TimerManager "TimerManager"
  */
 class Timer {
 private:
     TimerId mId;                                      /**< Unique timer identifier */
     std::weak_ptr<SLLooper> mLooper;                  /**< Weak reference to parent SLLooper */
     std::atomic<bool> mCancelled{false};              /**< Atomic cancellation flag */
     mutable std::atomic<bool> mMoved{false};          /**< Track if moved from (thread-safe) */
 
 public:
     /**
      * @brief Constructor - creates timer handle
      * @param id Unique timer identifier from TimerManager
      * @param looper Weak reference to parent SLLooper instance
      * 
      * Creates a Timer handle that manages the lifecycle of a kernel timer.
      * The weak_ptr prevents circular dependencies with the parent SLLooper.
      * 
      * @note Typically called internally by \ref swt::SLLooper::addTimer "SLLooper::addTimer"
      */
     Timer(TimerId id, std::weak_ptr<SLLooper> looper);
 
     /**
      * @brief Copy constructor - deleted (move-only semantics)
      * 
      * Timer objects cannot be copied to ensure unique ownership of
      * the underlying kernel timer resource.
      */
     Timer(const Timer&) = delete;
 
     /**
      * @brief Copy assignment - deleted (move-only semantics)
      * 
      * Timer objects cannot be copy-assigned to ensure unique ownership.
      */
     Timer& operator=(const Timer&) = delete;
 
     /**
      * @brief Move constructor - transfers timer ownership
      * @param other Timer object to move from
      * 
      * Safely transfers ownership of the underlying timer:
      * 1. Copies timer ID and looper reference
      * 2. Transfers cancellation state  
      * 3. Marks source object as moved-from
      * 4. Updates TimerManager's internal pointer to new location
      * 
      * @note Source timer becomes invalid after move
      * @note Thread-safe operation with atomic flags
      */
     Timer(Timer&& other) noexcept;
 
     /**
      * @brief Move assignment - transfers timer ownership
      * @param other Timer object to move from
      * @return Reference to this timer
      * 
      * Cancels current timer (if any) and transfers ownership from other timer.
      * Handles self-assignment safely.
      * 
      * @note Source timer becomes invalid after move
      * @note Thread-safe operation
      */
     Timer& operator=(Timer&& other) noexcept;
 
     /**
      * @brief Destructor - automatic timer cleanup
      * 
      * Automatically cancels the timer if:
      * - Timer hasn't been moved from
      * - Timer hasn't been explicitly cancelled
      * - SLLooper is still alive
      * 
      * Provides RAII guarantees for proper resource cleanup.
      */
     ~Timer();
 
     // ========== Public API (boost-style) ==========
 
     /**
      * @brief Cancel the timer
      * 
      * Cancels the underlying kernel timer and prevents future callback execution.
      * Safe to call multiple times - subsequent calls are no-ops.
      * 
      * @note Thread-safe operation
      * @note No-op if timer has been moved from
      * @note Safe to call even if timer has already expired
      * 
      * @code{.cpp}
      * Timer timer = looper->addTimer(callback, 1000);
      * timer.cancel();  // Timer won't fire
      * timer.cancel();  // Safe - no-op
      * @endcode
      * 
      * @see \ref swt::Timer::cancel "Timer::cancel"
      */
     void cancel();
 
     /**
      * @brief Check if timer is currently active
      * @return true if timer is active and not cancelled, false otherwise
      * 
      * Returns timer status by checking both local cancellation flag
      * and TimerManager's internal state. A timer is considered active if:
      * - It hasn't been cancelled locally
      * - It exists in TimerManager's active timer map
      * - Timer object hasn't been moved from
      * 
      * @note Thread-safe operation
      * @note Returns false for moved-from timer objects
      * 
      * @code{.cpp}
      * Timer timer = looper->addTimer(callback, 1000);
      * if (timer.isActive()) {
      *     std::cout << "Timer is running" << std::endl;
      * }
      * @endcode
      * 
      * @see \ref swt::Timer::isActive "Timer::isActive"
      */
     bool isActive() const;
 
     /**
      * @brief Get unique timer identifier
      * @return TimerId for this timer instance
      * 
      * Returns the unique identifier assigned by TimerManager.
      * Useful for debugging and logging purposes.
      * 
      * @note Safe to call on moved-from objects (returns original ID)
      * @see \ref swt::Timer::getId "Timer::getId"
      */
     TimerId getId() const { return mId; }
 
     /**
      * @brief Restart timer with new delay (one-shot timers only)
      * @param delay_ms New delay in milliseconds before timer expiration
      * 
      * Restarts the timer with a new delay value while maintaining
      * all other timer properties (callback, periodic flag, etc.).
      * Resets the cancellation flag to allow the timer to fire again.
      * 
      * @note Only works for one-shot timers
      * @note No-op if timer has been moved from
      * @note Thread-safe operation
      * 
      * @code{.cpp}
      * Timer timer = looper->addTimer(callback, 1000);
      * timer.cancel();           // Cancel current timer
      * timer.restart(2000);      // Restart with 2 second delay
      * @endcode
      * 
      * @see \ref swt::Timer::restart "Timer::restart"
      */
     void restart(uint64_t delay_ms);
 
     /**
      * @brief Friend class declaration for internal access
      * 
      * Allows SLLooper to access private members during timer creation
      * and management operations.
      */
     friend class SLLooper;
 
 private:
     /**
      * @brief Helper function for safe move operations
      * @param other Timer object to move from
      * 
      * Internal helper that performs the actual move operation:
      * 1. Transfers all member variables
      * 2. Updates TimerManager's internal pointers
      * 3. Marks source object as moved-from
      * 
      * @note Called by move constructor and move assignment operator
      * @note Assumes proper synchronization by caller
      */
     void moveFrom(Timer&& other) noexcept;
 };
 
 } // namespace swt