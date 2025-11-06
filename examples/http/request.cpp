#include "http/HttpService.h"
#include <iostream>
#include <thread>
#include <atomic>
#include <chrono>

int main() {
    using namespace sw_task::net;

    HttpService svc;
    std::atomic<bool> done{false};

    HttpRequest req;
    req.url = "https://httpbin.org/get";
    req.timeoutMs = 5000;
    req.onComplete = [&](const HttpResponse& r) {
        std::cout << "Completed: status=" << r.statusCode
                  << " body_size=" << r.body.size()
                  << " elapsed=" << r.elapsedMs << "ms\n";
        if (!r.body.empty()) {
            std::cout << "Body (first 200 chars):\n"
                      << r.body.substr(0, 200) << "\n";
        }
        if (r.error) {
            std::cout << "Error: " << r.error->message << "\n";
        }
        done = true;
    };

    auto id = svc.submit(std::move(req));
    std::cout << "Submitted request id=" << id << "\n";

    // Chờ tối đa timeout + margin
    auto start = std::chrono::steady_clock::now();
    while (!done) {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (std::chrono::steady_clock::now() - start > std::chrono::seconds(6)) {
            std::cout << "Timeout waiting for completion\n";
            break;
        }
    }

    return 0;
}