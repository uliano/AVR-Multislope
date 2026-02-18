/**
 * @file timer.hpp
 * @author uliano
 * @brief Type-safe timer system with support for multiple time units and callback types
 * @date Created on March 5, 2025, 9:34 AM
 * @revised 01/9/2026 - Renamed to .hpp
 *
 * This file implements a flexible timer system that supports:
 * - Multiple time units (ticks, milliseconds, seconds) enforced at compile-time
 * - Both free functions and member method callbacks
 * - One-shot and periodic timers
 * - Automatic timer list management via linked list
 * - Safe method pointer storage using type-erased unions
 */

#pragma once
#include <stdint.h>
#include "utils.hpp"
#include "ticker.hpp"
#include <string.h> // Required for memcpy

#include "globals.hpp"

/**
 * @brief Tag types for compile-time time unit enforcement
 *
 * These empty structs serve as type tags to create distinct Timer specializations
 * for different time units, preventing accidental mixing of incompatible timers.
 */
struct Ticks {};   ///< Tag for tick-based timers (raw hardware ticks)
struct Millis {};  ///< Tag for millisecond-based timers
struct Secs {};    ///< Tag for second-based timers

/// Type alias for free function callbacks
using CallbackFunction = void (*)();

/**
 * @class TimerBase
 * @brief Abstract base class providing common timer functionality
 *
 * This class implements the core timer logic including:
 * - Callback storage (both function pointers and method pointers)
 * - Expiration checking with overflow-safe time comparison
 * - Support for one-shot and periodic timers
 * - Linked list integration for timer management
 *
 * The class uses a type-erasure technique to store method pointers safely
 * across different class types while maintaining type safety at invocation.
 */
class TimerBase {
public:
    TimerBase* next;  ///< Pointer to next timer in linked list

    /**
     * @brief Union to store either a free function or a method wrapper
     *
     * - function: Direct function pointer for free function callbacks
     * - method_wrapper: Generic wrapper function that invokes type-specific method pointers
     */
    union {
        CallbackFunction function;
        void (*method_wrapper)(void*, void*);
    } m_callback;

    void* m_callback_object;  ///< Object instance pointer for method callbacks

    /**
     * @brief Union to safely store method pointers of varying sizes
     *
     * Member function pointers can be larger than regular pointers (especially with
     * virtual inheritance). This union stores them as raw bytes for later retrieval.
     */
    union {
        void* raw;
        void (TimerBase::*method)();
    } m_method_ptr;

    uint32_t m_period;      ///< Timer period in time units specific to template specialization
    uint32_t m_expiration;  ///< Absolute time when timer should expire
    bool m_running = false;  ///< True if timer is currently active
    bool m_expired = false;  ///< True if one-shot timer has expired
    bool m_periodic = false; ///< True for auto-restarting timers, false for one-shot

    /**
     * @brief Template function to safely invoke a member method callback
     * @tparam T The class type of the object instance
     * @param obj_ptr Pointer to the object instance (type-erased as void*)
     * @param method_ptr Pointer to the stored method pointer (type-erased as void*)
     *
     * This static template function serves as a type-specific wrapper that:
     * 1. Casts the object pointer back to its original type
     * 2. Reconstructs the method pointer from raw bytes using memcpy
     * 3. Invokes the method on the object instance
     *
     * Using memcpy ensures safe handling of method pointers which may have
     * non-standard sizes due to virtual inheritance or multiple inheritance.
     */
    template <typename T>
    static void invoke_method(void* obj_ptr, void* method_ptr) {
        T* instance = static_cast<T*>(obj_ptr);
        void (T::*method)();
        memcpy(&method, method_ptr, sizeof(method));
        (instance->*method)();
    }

    /**
     * @brief Check if timer has expired and execute callback if needed
     * @param now Current time value in the appropriate time units
     *
     * This method:
     * 1. Uses signed arithmetic for overflow-safe time comparison
     * 2. Invokes the appropriate callback type (method or function)
     * 3. For periodic timers: reschedules the next expiration
     * 4. For one-shot timers: marks as expired and stops
     *
     * The time comparison (int32_t)(now - m_expiration) >= 0 handles
     * uint32_t overflow correctly by treating time as a circular space.
     *
     * For periodic timers, if the system is heavily loaded and the next
     * expiration has already passed, the timer is rescheduled from 'now'
     * rather than accumulating missed periods.
     */
    void check_expiration(uint32_t now) {
        if (m_running) {
            if ((int32_t)(now - m_expiration) >= 0) {
                if (m_method_ptr.raw) {
                    m_callback.method_wrapper(m_callback_object, &m_method_ptr.raw);
                } else if (m_callback.function) {
                    m_callback.function();
                }
                if (m_periodic) {
                    m_expiration += m_period;
                    if ((int32_t)(now - m_expiration) >= 0) {
                        m_expiration = now + m_period;
                    }
                } else {
                    m_running = false;
                    m_expired = true;
                }
            }
        }
    }

    /// Pure virtual function to enforce abstract base class behavior
    virtual void enforceAbstract() = 0;

public:
    /**
     * @brief Constructor for free function callbacks
     * @param period Timer period in appropriate time units
     * @param periodic True for repeating timer, false for one-shot
     * @param callback Function pointer to invoke on expiration
     */
    TimerBase(uint32_t period, bool periodic, CallbackFunction callback)
        : next(nullptr), m_callback{.function = callback}, m_callback_object(nullptr), m_method_ptr{.raw = nullptr},
          m_period(period), m_expiration(0), m_running(false), m_expired(false), m_periodic(periodic) {}

    /**
     * @brief Constructor for member method callbacks
     * @tparam T Class type of the object
     * @param period Timer period in appropriate time units
     * @param periodic True for repeating timer, false for one-shot
     * @param method Pointer to member method to invoke
     * @param obj Pointer to object instance
     *
     * This constructor performs type-erasure to store the method pointer:
     * 1. Stores a type-specific wrapper function (invoke_method<T>)
     * 2. Stores the object pointer as void*
     * 3. Safely copies the method pointer bytes into m_method_ptr union
     */
    template <typename T>
    TimerBase(uint32_t period, bool periodic, void (T::*method)(), T* obj)
        : next(nullptr), m_callback{.method_wrapper = invoke_method<T>},
          m_callback_object(static_cast<void*>(obj)), m_method_ptr{.raw = nullptr},
          m_period(period), m_expiration(0), m_running(false), m_expired(false), m_periodic(periodic) {
        memcpy(&m_method_ptr.raw, &method, sizeof(method));
    }

    virtual ~TimerBase() = default;

    /// Stop the timer from running
    void stop() { m_running = false; }

    /// Change the timer period (takes effect on next start or period for periodic timers)
    void set_period(uint32_t period) { m_period = period; }

    /// Convert between one-shot and periodic mode
    void set_periodic(bool periodic) { m_periodic = periodic; }

    /// Check if timer is currently running
    inline bool running() { return m_running; }

    /// Check if one-shot timer has expired (only meaningful for non-periodic timers)
    inline bool expired() { return m_expired; }
};

/**
 * @class Timer
 * @brief Template class providing time-unit-specific timer implementations
 * @tparam T Time unit tag type (Ticks, Millis, or Secs)
 *
 * This template specializes the timer system for different time units:
 * - Timer<Ticks>: Uses raw hardware tick counts from Ticker
 * - Timer<Millis>: Uses millisecond timestamps
 * - Timer<Secs>: Uses second timestamps
 *
 * Each specialization:
 * - Maintains its own linked list of timers (static head pointer)
 * - Provides unit-specific start() implementation
 * - Provides unit-specific checkAllTimers() for polling all active timers
 *
 * The static_assert prevents instantiation with invalid time unit types.
 *
 * @note All timers of the same type share a single linked list for efficient
 *       batch checking via checkAllTimers()
 */
template <typename T>
class Timer : public TimerBase {
    static_assert(
        is_same<T, Ticks>::value ||
        is_same<T, Millis>::value ||
        is_same<T, Secs>::value,
        "Timer can only be instantiated with Ticks, Millis, or Secs!");

private:
    static Timer<T>* head;  ///< Head of linked list for this timer type
    void enforceAbstract() override {}  ///< Empty implementation to make class concrete

public:
    /**
     * @brief Constructor for member method callbacks
     * @tparam Obj Class type of the callback object
     * @param period Timer period in time units specified by template parameter T
     * @param periodic True for auto-restarting timer, false for one-shot
     * @param method Pointer to member method (signature: void method())
     * @param object Pointer to object instance on which to invoke the method
     *
     * Automatically adds this timer to the front of the linked list for this time unit.
     */
    template <typename Obj>
    Timer(uint32_t period, bool periodic, void (Obj::*method)(), Obj* object)
        : TimerBase(period, periodic, method, object) {
        next = head;
        head = this;
    }

    /**
     * @brief Constructor for free function callbacks
     * @param period Timer period in time units specified by template parameter T
     * @param periodic True for auto-restarting timer, false for one-shot
     * @param callback Function pointer to invoke on expiration (nullptr = no callback)
     *
     * Automatically adds this timer to the front of the linked list for this time unit.
     */
    Timer(uint32_t period, bool periodic, CallbackFunction callback = nullptr)
        : TimerBase(period, periodic, callback) {
        next = head;
        head = this;
    }

    /**
     * @brief Destructor automatically removes timer from linked list
     *
     * Searches the linked list to find and remove this timer, ensuring
     * that checkAllTimers() won't try to access freed memory.
     */
    ~Timer() override {
        if (head == this) {
            head = static_cast<Timer<T>*>(next);
        } else {
            Timer<T>* prev = head;
            while (prev && prev->next != this) {
                prev = static_cast<Timer<T>*>(prev->next);
            }
            if (prev) prev->next = next;
        }
    }

    /**
     * @brief Start or restart the timer
     *
     * Queries the current time from Ticker based on the template parameter:
     * - Timer<Ticks>: Uses Ticker::ticks()
     * - Timer<Millis>: Uses Ticker::millis()
     * - Timer<Secs>: Uses Ticker::secs()
     *
     * Calculates expiration time as current_time + period and marks timer as running.
     * The compile-time if constexpr ensures only the appropriate time source is used.
     */
    void start() {
        m_running = true;
        uint32_t start;
        if constexpr (is_same<T, Millis>::value) {
            start = Ticker::ptr->millis();
        } else if constexpr (is_same<T, Secs>::value) {
            start = Ticker::ptr->secs();
        } else if constexpr (is_same<T, Ticks>::value) {
            start = Ticker::ptr->ticks();
        }
        m_expiration = start + m_period;
    }

    /**
     * @brief Check all timers of this type for expiration
     *
     * This static method should be called periodically (e.g., from main loop)
     * to process all timers of this time unit type.
     *
     * Implementation details:
     * 1. Retrieves current time from the appropriate Ticker method
     * 2. Optimization: Returns immediately if time hasn't changed since last check
     * 3. Iterates through linked list, checking each timer's expiration
     * 4. Executes callbacks for expired timers and handles rescheduling
     *
     * The static last_check variable prevents redundant processing when called
     * multiple times within the same time unit (e.g., multiple times per millisecond).
     *
     * @note Must be called regularly for timers to function. Typically invoked from:
     *       - Main loop for Millis/Secs timers
     *       - Interrupt handler for Ticks timers (if sub-millisecond precision needed)
     */
    static void checkAllTimers() {
        static uint32_t last_check = 0;
        uint32_t now;
        if constexpr (is_same<T, Millis>::value) {
            now = Ticker::ptr->millis();
        } else if constexpr (is_same<T, Secs>::value) {
            now = Ticker::ptr->secs();
        } else if constexpr (is_same<T, Ticks>::value) {
            now = Ticker::ptr->ticks();
        }
        if (now == last_check) return;
        last_check = now;
        Timer<T>* timer = head;
        while (timer) {
            timer->check_expiration(now);
            timer = static_cast<Timer<T>*>(timer->next);
        }
    }
};

/**
 * @brief Initialize static head pointer for each Timer template specialization
 *
 * Each instantiation of Timer<T> gets its own static linked list.
 * For example:
 * - Timer<Millis>::head points to the list of all millisecond timers
 * - Timer<Secs>::head points to the list of all second timers
 * - Timer<Ticks>::head points to the list of all tick timers
 */
template <typename T>
Timer<T>* Timer<T>::head = nullptr;

/**
 * @example Usage examples:
 *
 * // One-shot timer with free function (500ms delay)
 * void onTimeout() { Serial.println("Timeout!"); }
 * Timer<Millis> timer1(500, false, onTimeout);
 * timer1.start();
 *
 * // Periodic timer with member method (1 second interval)
 * class Blinker {
 *     void toggle() { digitalWrite(LED, !digitalRead(LED)); }
 *     Timer<Secs> timer{1, true, &Blinker::toggle, this};
 * public:
 *     void begin() { timer.start(); }
 * };
 *
 * // In main loop:
 * void loop() {
 *     Timer<Millis>::checkAllTimers();  // Process millisecond timers
 *     Timer<Secs>::checkAllTimers();    // Process second timers
 * }
 */
