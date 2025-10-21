#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <memory>
#include <thread>
#include <stdexcept>
#include <vector>

using namespace swt;

// 1. Abstract interface: chiến lược lấy dữ liệu
class IDataFetcher {
public:
    virtual ~IDataFetcher() = default;
    // Có thể ném lỗi; không dùng co_await tại đây (chạy nền)
    virtual int fetch() = 0;
};

// 2. Triển khai cụ thể: giả lập tác vụ chậm
class SlowDataFetcher : public IDataFetcher {
public:
    explicit SlowDataFetcher(int base) : base_(base) {}
    int fetch() override {
        std::cout << "[SlowDataFetcher] Start heavy work...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
        // Giả lập tính toán
        return base_ * 2 + 5;
    }
private:
    int base_;
};

// 3. Triển khai khác: có thể ném lỗi
class FragileFetcher : public IDataFetcher {
public:
    int fetch() override {
        std::cout << "[FragileFetcher] Attempting fetch...\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        // Giả lập lỗi runtime
        throw std::runtime_error("FragileFetcher: simulated failure");
    }
};

// 4. Service bất đồng bộ: quản lý chiến lược và expose coroutine API
class AsyncFetchService : public std::enable_shared_from_this<AsyncFetchService> {
public:
    AsyncFetchService(std::shared_ptr<SLLooper> looper,
                      std::shared_ptr<IDataFetcher> strategy)
        : looper_(std::move(looper)), strategy_(std::move(strategy)) {}

    void setStrategy(std::shared_ptr<IDataFetcher> strategy) {
        strategy_ = std::move(strategy);
    }

    // Coroutine trả về dữ liệu
    Task<int> fetchOnce() {
        std::cout << "[AsyncFetchService] fetchOnce() begin\n";
        int value = 0;
        try {
            // co_await chạy hàm thành phần (lambda giữ this & strategy)
            value = co_await looper_->awaitPost([self = shared_from_this()] {
                return self->strategy_->fetch();
            });
            std::cout << "[AsyncFetchService] fetchOnce() got value=" << value << "\n";
        } catch (const std::exception& ex) {
            std::cout << "[AsyncFetchService] fetchOnce() caught error: " << ex.what() << "\n";
            value = -1; // fallback đơn giản
        }
        co_return value;
    }

    // Coroutine thực hiện nhiều lần và trả về tổng
    Task<int> fetchAggregate(int times) {
        std::cout << "[AsyncFetchService] fetchAggregate(" << times << ") begin\n";
        int sum = 0;
        for (int i = 0; i < times; ++i) {
            int v = co_await fetchOnce(); // Gọi coroutine khác (composition)
            if (v >= 0) {
                sum += v;
            } else {
                std::cout << "[AsyncFetchService] Skipping failed value at iteration " << i << "\n";
            }
        }
        std::cout << "[AsyncFetchService] fetchAggregate result=" << sum << "\n";
        co_return sum;
    }

    // Coroutine minh họa thay đổi chiến lược động
    Task<void> demoSwitchStrategy() {
        std::cout << "[AsyncFetchService] demoSwitchStrategy begin\n";
        int first = co_await fetchOnce();
        std::cout << "[AsyncFetchService] First strategy result=" << first << "\n";

        std::cout << "[AsyncFetchService] Switching to FragileFetcher...\n";
        setStrategy(std::make_shared<FragileFetcher>());

        int second = co_await fetchOnce();
        std::cout << "[AsyncFetchService] Second strategy result=" << second << "\n";
        std::cout << "[AsyncFetchService] demoSwitchStrategy done\n";
    }

private:
    std::shared_ptr<SLLooper> looper_;
    std::shared_ptr<IDataFetcher> strategy_;
};

int main() {
    auto looper = std::make_shared<SLLooper>();

    // Khởi tạo service với chiến lược ban đầu
    auto service = std::make_shared<AsyncFetchService>(
        looper,
        std::make_shared<SlowDataFetcher>(10)
    );

    // Tạo các coroutine
    auto t1 = service->fetchOnce();
    auto t2 = service->fetchAggregate(3);
    auto t3 = service->demoSwitchStrategy();

    // Start (Task là lazy)
    t1.start();
    t2.start();
    t3.start();

    std::cout << "[main] Tasks started, waiting...\n";
    std::this_thread::sleep_for(std::chrono::seconds(3));

    std::cout << "[main] Exit.\n";
    return 0;
}