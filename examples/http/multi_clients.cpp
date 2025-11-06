#include "http/HttpService.h"
#include <thread>
#include <vector>
#include <iostream>

using namespace sw_task::net;

int main() {
    HttpService svc;
    svc.setMaxConcurrent(8); // Tăng số lượng xử lý đồng thời

    std::vector<HttpService::RequestId> ids;
    std::vector<std::thread> clients;

    for (int i = 0; i < 10; ++i) {
        clients.emplace_back([i, &svc] {
            HttpRequest req;
            req.url = "https://httpbin.org/get?client=" + std::to_string(i);
            req.onComplete = [i](const HttpResponse& resp) {
                std::cout << "Client " << i << " status: " << resp.statusCode
                          << ", body size: " << resp.body.size() << "\n";
            };
            svc.submit(std::move(req));
        });
    }

    for (auto& t : clients) t.join();

    // Đợi xử lý xong (có thể dùng sleep hoặc kiểm tra svc.pending())
    std::this_thread::sleep_for(std::chrono::seconds(3));
    svc.shutdown();
}