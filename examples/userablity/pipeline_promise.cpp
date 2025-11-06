#include "SLLooper.h"
#include "Promise.h"
#include <iostream>
#include <memory>
#include <vector>
#include <thread>
#include <chrono>

using namespace swt;

// Các bước synchronous mô phỏng (được thực thi bên trong looper->post)
int loadConfig() {
    std::cout << "[loadConfig]\n";
    return 5;
}

std::vector<int> fetchData(int factor) {
    std::cout << "[fetchData] factor=" << factor << "\n";
    return {1,2,3,4,5};
}

std::vector<int> transformData(const std::vector<int>& in, int factor) {
    std::cout << "[transformData]\n";
    std::vector<int> out; out.reserve(in.size());
    for (int v: in) out.push_back(v * factor);
    return out;
}

int computeMetric(const std::vector<int>& data) {
    std::cout << "[computeMetric]\n";
    int sum = 0;
    for (int v: data) sum += v;
    return sum;
}

bool saveResult(int metric) {
    std::cout << "[saveResult] metric=" << metric << "\n";
    return metric > 0;
}

// Helper: post một hàm synchronous lên looper và trả về Promise<T>
template<typename F>
auto asyncCall(std::shared_ptr<SLLooper> looper, F&& f)
    -> Promise<decltype(f())>
{
    auto p = looper->createPromise<decltype(f())>();
    looper->post([p, fn = std::forward<F>(f)]() mutable {
        try {
            p.set_value(fn());
        } catch (...) {
            p.set_exception(std::current_exception());
        }
    });
    return p;
}

int main() {
    auto looper = std::make_shared<SLLooper>();

    asyncCall(looper, loadConfig)
    .then(looper, [looper](int factor) {
        return fetchData(factor); 
    })
    .then(looper, [looper](std::vector<int> raw) {
        int factor = 5; 
        return transformData(raw, factor);
    })
    .then(looper, [looper](std::vector<int> transformed) {
        return computeMetric(transformed);
    })
    .then(looper, [looper](int metric) {
        return saveResult(metric); // bool
    })
    .then(looper, [looper](bool saved) {
        std::cout << "[pipeline] saved=" << std::boolalpha << saved << "\n";
        looper->exit();
        return saved; 
    })
    .catchError(looper, [looper](std::exception_ptr ex) {
        try {
            if (ex) std::rethrow_exception(ex);
        } catch (const std::exception& e) {
            std::cout << "[pipeline] error: " << e.what() << "\n";
        }
        looper->exit();
        return false;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    std::cout << "[main] exit\n";
    return 0;
}