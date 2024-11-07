#pragma once
#include <deque>
#include <mutex>
#include <optional>
#include <condition_variable>
#include <chrono>
#include <type_traits>

/**
 * @file async_deque.hpp
 * @brief Thread-safe asynchronous double-ended queue implementation
 * @author Harri Pehkonen
 * @date 2024
 * 
 * @details This header provides a thread-safe, capacity-bounded double-ended queue
 * with support for blocking and non-blocking operations, timeouts, and extension mechanisms.
 * 
 * Example usage:
 * @code{.cpp}
 * AsyncDeque<int> queue(100);  // Create a queue with capacity 100
 * 
 * // Producer thread
 * queue.push_back(42);
 * if (queue.try_push_back(43, 100ms)) {
 *     // Successfully pushed within timeout
 * }
 * 
 * // Consumer thread
 * if (auto value = queue.pop_front()) {
 *     std::cout << "Got value: " << *value << std::endl;
 * }
 * @endcode
 */

namespace async_deque {

/**
 * @brief Forward declaration for the AsyncDeque template with extensions
 * @tparam T The type of elements to store
 * @tparam Extensions Parameter pack for extension types
 */
template<typename T, typename... Extensions>
class AsyncDeque;

/**
 * @brief A thread-safe asynchronous double-ended queue
 * 
 * @tparam T The type of elements to store in the queue
 * 
 * This class implements a thread-safe double-ended queue with the following features:
 * - Bounded capacity
 * - Blocking and non-blocking operations
 * - Timeout support for operations
 * - RAII-compliant resource management
 * - Move semantics support
 * - Extension mechanism through virtual hooks
 * 
 * @note All public methods are thread-safe
 * @warning Copy operations are explicitly deleted
 * 
 * @invariant capacity() remains constant throughout the object's lifetime
 * @invariant size() <= capacity() at all times
 */
template<typename T>
class AsyncDeque<T> {
protected:
    mutable std::mutex mutex_;              ///< Mutex for thread-safety
    std::condition_variable cv_;            ///< Condition variable for blocking operations
    std::deque<T> deque_;                  ///< Underlying container
    bool closed_ = false;                   ///< Queue state flag
    const size_t capacity_;                 ///< Maximum queue capacity

    /**
     * @name Extension Hooks
     * Virtual methods that can be overridden by derived classes to extend functionality
     * @{
     */

    /**
     * @brief Called after an item is pushed to the back
     * @param item Reference to the item that was pushed
     * @note Thread-safe: called while holding the mutex
     */
    virtual void on_push_back(const T& item) {}

    /**
     * @brief Called after an item is pushed to the front
     * @param item Reference to the item that was pushed
     * @note Thread-safe: called while holding the mutex
     */
    virtual void on_push_front(const T& item) {}

    /**
     * @brief Called after an item is popped from the back
     * @param item Reference to the item that was popped
     * @note Thread-safe: called while holding the mutex
     */
    virtual void on_pop_back(const T& item) {}

    /**
     * @brief Called after an item is popped from the front
     * @param item Reference to the item that was popped
     * @note Thread-safe: called while holding the mutex
     */
    virtual void on_pop_front(const T& item) {}

    /**
     * @brief Called when the queue is closed
     * @note Thread-safe: called while holding the mutex
     */
    virtual void on_close() {}

    /** @} */  // End of Extension Hooks

public:
    /**
     * @brief Constructs an AsyncDeque with the specified capacity
     * 
     * @param capacity Maximum number of items the queue can hold
     * @throws std::bad_alloc if memory allocation fails
     * 
     * @post empty() == true
     * @post is_closed() == false
     * @post this->capacity() == capacity
     */
    explicit AsyncDeque(size_t capacity = std::numeric_limits<size_t>::max())
        : capacity_(capacity) {}

    /**
     * @brief Destructor
     * 
     * Ensures the queue is closed before destruction.
     * @note This will wake up any threads waiting on the queue
     */
    virtual ~AsyncDeque() {
        close();
    }

    /**
     * @name Move Operations
     * Support for move semantics with strong exception guarantee
     * @{
     */

    /**
     * @brief Move constructor
     * 
     * @param other Queue to move from
     * 
     * @note noexcept guarantee
     * @post other is empty but valid
     */
    AsyncDeque(AsyncDeque&& other) noexcept
        : capacity_(other.capacity_) {
        std::lock_guard<std::mutex> lock(other.mutex_);
        deque_ = std::move(other.deque_);
        closed_ = other.closed_;
    }

    /**
     * @brief Move assignment operator
     * 
     * @param other Queue to move from
     * @return AsyncDeque& Reference to *this
     * 
     * @note If capacities don't match, the operation is a no-op
     * @note noexcept guarantee
     */
    AsyncDeque& operator=(AsyncDeque&& other) noexcept {
        if (this != &other && capacity_ == other.capacity_) {
            std::scoped_lock lock(mutex_, other.mutex_);
            deque_ = std::move(other.deque_);
            closed_ = other.closed_;
        }
        return *this;
    }

    /** @} */  // End of Move Operations

    /**
     * @name Deleted Copy Operations
     * Copy operations are deleted to ensure thread-safety
     * @{
     */
    AsyncDeque(const AsyncDeque&) = delete;
    AsyncDeque& operator=(const AsyncDeque&) = delete;
    /** @} */

    /**
     * @name Push Operations
     * Methods for adding items to the queue
     * @{
     */

    /**
     * @brief Pushes an item to the back of the queue
     * 
     * @tparam U Type of the item (must be convertible to T)
     * @param item Item to push
     * @return true if the item was pushed successfully
     * @return false if the queue is closed
     * 
     * @note Blocks if the queue is at capacity
     * @throws Any exception thrown by T's move/copy constructor
     */
    template<typename U>
    bool push_back(U&& item);  // Implementation as before...

    /**
     * @brief Attempts to push an item to the back with a timeout
     * 
     * @tparam Rep Type representing the number of ticks
     * @tparam Period Type representing the tick period
     * @param item Item to push
     * @param timeout Maximum time to wait
     * @return true if the item was pushed
     * @return false if timed out or queue is closed
     * 
     * @throws Any exception thrown by T's copy constructor
     */
    template<typename Rep, typename Period>
    bool try_push_back(const T& item, 
                      const std::chrono::duration<Rep, Period>& timeout);

    /** @} */  // End of Push Operations

    // ... Similar comprehensive documentation for other methods ...

    /**
     * @brief Queries if a specific extension type is present
     * 
     * @tparam E The extension type to check for
     * @return true if the extension is present
     * @return false if the extension is not present
     * 
     * @note Thread-safe
     * @see AsyncDeque template with Extensions
     */
    template<typename E>
    bool has_extension() const {
        return false;  // Base case - no extensions
    }
};

/**
 * @class async_deque::AsyncDeque
 * @example producer_consumer.cpp
 * 
 * This example shows how to use AsyncDeque in a producer-consumer scenario:
 * 
 * @code
 * AsyncDeque<int> queue(100);
 * 
 * // Producer thread
 * std::thread producer([&queue]() {
 *     for (int i = 0; i < 1000; ++i) {
 *         if (!queue.push_back(i)) {
 *             break;  // Queue was closed
 *         }
 *     }
 * });
 * 
 * // Consumer thread
 * std::thread consumer([&queue]() {
 *     while (true) {
 *         auto value = queue.try_pop_front(100ms);
 *         if (!value) {
 *             break;  // Timeout or queue closed
 *         }
 *         process(*value);
 *     }
 * });
 * @endcode
 */

} // namespace async_deque
