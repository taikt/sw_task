// g++ raw_example.cpp -std=c++20

#include <coroutine>
#include <thread>
#include <iostream>
#include <chrono>

// Awaitable chạy phép cộng giả lập bất đồng bộ
struct AddAsync {
    int a, b;
    int result{0};

    bool await_ready() const noexcept { return false; }

    void await_suspend(std::coroutine_handle<> h) {
        std::thread([this, h] {
            std::this_thread::sleep_for(std::chrono::milliseconds(5000)); // giả lập delay
            result = a + b;
            h.resume();
        }).detach();
    }

    int await_resume() const noexcept { return result; }
};

// Task<T> tối giản (không exception, không multi-await, không sync_wait tinh vi)
template<typename T>
struct Task {
    struct promise_type {
        T value;

        Task get_return_object() {
            return Task{
                std::coroutine_handle<promise_type>::from_promise(*this)
            };
        }
        // Chạy ngay coroutine cho đến điểm suspend đầu tiên
        std::suspend_never initial_suspend() noexcept { return {}; }
        // Giữ frame lại sau khi co_return để caller đọc value
        std::suspend_always final_suspend() noexcept { return {}; }
        void return_value(T v) noexcept { value = v; }
        void unhandled_exception() { std::terminate(); }
    };

    std::coroutine_handle<promise_type> h;

    explicit Task(std::coroutine_handle<promise_type> handle) : h(handle) {}
    Task(Task&& other) noexcept : h(other.h) { other.h = {}; }
    Task(const Task&) = delete;
    Task& operator=(const Task&) = delete;

    ~Task() {
        if (h) h.destroy();
    }

    bool done() const { return !h || h.done(); }

    T get() {
        while (!done()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        return h.promise().value;
    }
};

// Coroutine sử dụng awaitable
Task<int> compute() {
    int x = co_await AddAsync{2, 3}; // suspend, thread nền tính 2+3
    co_return x * 10;                // trả 50
}

int main() {
    Task<int> t = compute(); // tạo + chạy tới co_await
    std::cout << "Computing...\n";
    int r = t.get();         // chờ hoàn thành (polling đơn giản)
    std::cout << "Result = " << r << "\n";
}


