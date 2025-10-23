#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>
#include <stdexcept>

using namespace swt;

// Các hàm giả lập logic nhẹ (có thể tùy chọn ném exception)
int loadConfig() {
    std::cout << "[loadConfig]\n";
    // Ví dụ: throw std::runtime_error("config missing");
    return 5;
}

std::vector<int> fetchData() {
    std::cout << "[fetchData]\n";
    return {1,2,3,4,5};
}

std::vector<int> transformData(const std::vector<int>& in, int factor) {
    std::cout << "[transformData]\n";
    std::vector<int> out;
    out.reserve(in.size());
    for (int v: in) out.push_back(v * factor);
    if (out.empty()) throw std::runtime_error("transformation error");
    return out;
}

int computeMetric(const std::vector<int>& data) {
    std::cout << "[computeMetric]\n";
    int sum = 0;
    for (int v: data) sum += v; 
    if (sum == 0) throw std::runtime_error("empty metric");
    
    return sum;
}

bool saveResult(int metric) {
    std::cout << "[saveResult] metric=" << metric << "\n";
    if (metric <= 0) throw std::runtime_error("metric invalid");
    return true;
}

Task<void> pipeline(std::shared_ptr<SLLooper> looper) {
    std::cout << "[pipeline] begin\n";
    try {
        int factor       = co_await looper->awaitPost(loadConfig);
        auto raw         = co_await looper->awaitPost(fetchData);
        auto transformed = co_await looper->awaitPost([&]{ return transformData(raw, factor); });
        int metric       = co_await looper->awaitPost([&]{ return computeMetric(transformed); });
        bool saved       = co_await looper->awaitPost([&]{ return saveResult(metric); });

        std::cout << "[pipeline] saved=" << std::boolalpha << saved << "\n";
    } catch (const std::exception& ex) {
        std::cout << "[pipeline] exception: " << ex.what() << "\n";
    } catch (...) {
        std::cout << "[pipeline] unknown exception\n";
    }
    std::cout << "[pipeline] end\n";
    looper->exit();
}

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto task = pipeline(looper);
    task.start();

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    std::cout << "[main] exit\n";
    return 0;
}