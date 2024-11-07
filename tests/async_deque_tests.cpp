#include <gtest/gtest.h>
#include <async_deque/async_deque.hpp>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>

using namespace async_deque;
using namespace std::chrono_literals;

// Helper class to track operations
class TrackedItem {
public:
    explicit TrackedItem(int value) : value_(value) {
        instances_++;
    }
    
    TrackedItem(const TrackedItem& other) : value_(other.value_) {
        instances_++;
    }
    
    TrackedItem(TrackedItem&& other) noexcept : value_(other.value_) {
        instances_++;
        other.value_ = 0;
    }
    
    ~TrackedItem() {
        instances_--;
    }
    
    TrackedItem& operator=(const TrackedItem& other) {
        if (this != &other) {
            value_ = other.value_;
        }
        return *this;
    }
    
    TrackedItem& operator=(TrackedItem&& other) noexcept {
        if (this != &other) {
            value_ = other.value_;
            other.value_ = 0;
        }
        return *this;
    }
    
    int value() const { return value_; }
    static int instances() { return instances_; }
    
private:
    int value_;
    static inline std::atomic<int> instances_{0};
};

// Base test fixture
class AsyncDequeTest : public ::testing::Test {
protected:
    void SetUp() override {
        ASSERT_EQ(TrackedItem::instances(), 0) 
            << "Leaked TrackedItem instances from previous test";
    }
    
    void TearDown() override {
        ASSERT_EQ(TrackedItem::instances(), 0) 
            << "Leaked TrackedItem instances";
    }
};

// Basic functionality tests
TEST_F(AsyncDequeTest, DefaultConstructor) {
    AsyncDeque<int> deque;
    EXPECT_TRUE(deque.empty());
    EXPECT_EQ(deque.size(), 0);
    EXPECT_FALSE(deque.is_closed());
}

TEST_F(AsyncDequeTest, CapacityConstructor) {
    const size_t capacity = 5;
    AsyncDeque<int> deque(capacity);
    EXPECT_TRUE(deque.empty());
    EXPECT_EQ(deque.capacity(), capacity);
}

TEST_F(AsyncDequeTest, PushPopFrontBasic) {
    AsyncDeque<int> deque(2);
    EXPECT_TRUE(deque.push_front(1));
    EXPECT_TRUE(deque.push_front(2));
    EXPECT_EQ(deque.size(), 2);
    
    auto val = deque.pop_front();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 2);
    
    val = deque.pop_front();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1);
    
    EXPECT_TRUE(deque.empty());
}

TEST_F(AsyncDequeTest, PushPopBackBasic) {
    AsyncDeque<int> deque(2);
    EXPECT_TRUE(deque.push_back(1));
    EXPECT_TRUE(deque.push_back(2));
    EXPECT_EQ(deque.size(), 2);
    
    auto val = deque.pop_back();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 2);
    
    val = deque.pop_back();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1);
    
    EXPECT_TRUE(deque.empty());
}

// Move semantics tests
TEST_F(AsyncDequeTest, MoveConstructor) {
    AsyncDeque<TrackedItem> source(5);
    EXPECT_TRUE(source.push_back(TrackedItem(1)));
    EXPECT_TRUE(source.push_back(TrackedItem(2)));
    
    AsyncDeque<TrackedItem> dest(std::move(source));
    EXPECT_EQ(dest.size(), 2);
    
    auto val = dest.pop_front();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->value(), 1);
}

TEST_F(AsyncDequeTest, MoveAssignment) {
    AsyncDeque<TrackedItem> source(5);
    AsyncDeque<TrackedItem> dest(5);
    
    EXPECT_TRUE(source.push_back(TrackedItem(1)));
    EXPECT_TRUE(source.push_back(TrackedItem(2)));
    
    dest = std::move(source);
    EXPECT_EQ(dest.size(), 2);
    
    auto val = dest.pop_front();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(val->value(), 1);
}

TEST_F(AsyncDequeTest, MoveAssignmentDifferentCapacities) {
    AsyncDeque<TrackedItem> source(5);
    AsyncDeque<TrackedItem> dest(10);
    
    EXPECT_TRUE(source.push_back(TrackedItem(1)));
    EXPECT_TRUE(source.push_back(TrackedItem(2)));
    size_t original_size = dest.size();
    
    // Moving from source to dest should now be a no-op because of different capacities
    dest = std::move(source);
    
    // Destination should remain unchanged
    EXPECT_EQ(dest.size(), original_size);
    EXPECT_EQ(dest.capacity(), 10);
    
    // Source should still be usable
    EXPECT_TRUE(source.push_back(TrackedItem(3)));
}

// Capacity and blocking tests
TEST_F(AsyncDequeTest, CapacityBlocking) {
    AsyncDeque<int> deque(2);
    
    std::atomic<bool> push_completed{false};
    std::thread pusher([&]() {
        EXPECT_TRUE(deque.push_back(1));
        EXPECT_TRUE(deque.push_back(2));
        EXPECT_TRUE(deque.push_back(3));  // Should block
        push_completed = true;
    });
    
    std::this_thread::sleep_for(100ms);
    EXPECT_FALSE(push_completed);  // Should still be blocked
    
    auto val = deque.pop_front();  // Unblock the pusher
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1);
    
    pusher.join();
    EXPECT_TRUE(push_completed);
}

// Timeout tests
TEST_F(AsyncDequeTest, TryPushTimeout) {
    AsyncDeque<int> deque(1);
    EXPECT_TRUE(deque.push_back(1));
    
    auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(deque.try_push_back(2, 100ms));
    auto duration = std::chrono::steady_clock::now() - start;
    
    EXPECT_GE(duration, 100ms);
    EXPECT_LT(duration, 150ms);  // Allow some overhead
}

TEST_F(AsyncDequeTest, TryPopTimeout) {
    AsyncDeque<int> deque(1);
    
    auto start = std::chrono::steady_clock::now();
    EXPECT_FALSE(deque.try_pop_front(100ms).has_value());
    auto duration = std::chrono::steady_clock::now() - start;
    
    EXPECT_GE(duration, 100ms);
    EXPECT_LT(duration, 150ms);
}

// Close behavior tests
TEST_F(AsyncDequeTest, CloseEmptyDeque) {
    AsyncDeque<int> deque;
    deque.close();
    
    EXPECT_TRUE(deque.is_closed());
    EXPECT_FALSE(deque.push_back(1));
    EXPECT_FALSE(deque.pop_front().has_value());
}

TEST_F(AsyncDequeTest, CloseNonEmptyDeque) {
    AsyncDeque<int> deque;
    EXPECT_TRUE(deque.push_back(1));
    EXPECT_TRUE(deque.push_back(2));
    
    deque.close();
    
    EXPECT_TRUE(deque.is_closed());
    EXPECT_FALSE(deque.push_back(3));
    
    auto val = deque.pop_front();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 1);
    
    val = deque.pop_front();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(*val, 2);
    
    EXPECT_FALSE(deque.pop_front().has_value());
}

// Concurrent operation tests
TEST_F(AsyncDequeTest, ConcurrentPushPop) {
    AsyncDeque<int> deque(100);
    std::atomic<int> sum{0};
    constexpr int num_producers = 4;
    constexpr int num_consumers = 4;
    constexpr int items_per_producer = 1000;
    
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;
    
    // Start producers
    for (int i = 0; i < num_producers; ++i) {
        producers.emplace_back([&deque, i]() {
            for (int j = 0; j < items_per_producer; ++j) {
                EXPECT_TRUE(deque.push_back(i * items_per_producer + j));
            }
        });
    }
    
    // Start consumers
    for (int i = 0; i < num_consumers; ++i) {
        consumers.emplace_back([&deque, &sum]() {
            while (true) {
                auto val = deque.pop_front();
                if (!val.has_value()) break;
                sum += *val;
            }
        });
    }
    
    // Wait for producers to finish
    for (auto& producer : producers) {
        producer.join();
    }
    
    // Close the deque and wait for consumers
    deque.close();
    for (auto& consumer : consumers) {
        consumer.join();
    }
    
    // Calculate expected sum
    int expected_sum = 0;
    for (int i = 0; i < num_producers * items_per_producer; ++i) {
        expected_sum += i;
    }
    
    EXPECT_EQ(sum, expected_sum);
}

// Extension mechanism tests
class TestExtension : public AsyncDeque<int> {
public:
    explicit TestExtension(size_t capacity) : AsyncDeque<int>(capacity) {}
    
    int push_count() const { return push_count_; }
    int pop_count() const { return pop_count_; }
    bool close_called() const { return close_called_; }
    
protected:
    void on_push_back(const int& item) override {
        push_count_++;
        last_pushed_ = item;
    }
    
    void on_push_front(const int& item) override {
        push_count_++;
        last_pushed_ = item;
    }
    
    void on_pop_back(const int& item) override {
        pop_count_++;
        last_popped_ = item;
    }
    
    void on_pop_front(const int& item) override {
        pop_count_++;
        last_popped_ = item;
    }
    
    void on_close() override {
        close_called_ = true;
    }
    
private:
    std::atomic<int> push_count_{0};
    std::atomic<int> pop_count_{0};
    std::atomic<int> last_pushed_{0};
    std::atomic<int> last_popped_{0};
    std::atomic<bool> close_called_{false};
};

TEST_F(AsyncDequeTest, ExtensionHooks) {
    TestExtension deque(5);
    
    EXPECT_TRUE(deque.push_back(1));
    EXPECT_TRUE(deque.push_front(2));
    EXPECT_EQ(deque.push_count(), 2);
    
    auto val = deque.pop_back();
    ASSERT_TRUE(val.has_value());
    val = deque.pop_front();
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(deque.pop_count(), 2);
    
    deque.close();
    EXPECT_TRUE(deque.close_called());
}

