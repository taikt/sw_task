/*
Bắt được exception trong `.catchError` nếu:

- **Người dùng chủ động throw** (ví dụ: `throw std::runtime_error("...")`)
- **C++ exception tự động tạo ra** bởi thư viện chuẩn hoặc các hàm sử dụng `throw` (ví dụ: `std::vector::at()` ném `std::out_of_range`, `std::stoi("abc")` ném `std::invalid_argument`, v.v.)

**Không bắt được** các lỗi hệ thống như:  
- Truy cập bộ nhớ sai (null pointer, buffer overflow, ...)
- Chia cho 0 với số nguyên
- Crash do lỗi native code

Những lỗi này sẽ làm chương trình dừng ngay lập tức, không truyền qua promise chain.

*/

#include "SLLooper.h"
#include "Promise.h"
#include <iostream>
#include <memory>
#include <thread>
#include <chrono>
#include <stdexcept>
using namespace swt;

// Hàm trả về Promise<int>, có thể ném exception
Promise<int> fetchWithError(std::shared_ptr<SLLooper> looper, bool shouldThrow) {
    auto promise = looper->createPromise<int>();
    looper->post([promise, shouldThrow]() mutable {
        std::cout << "Start fetching..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        if (shouldThrow) {
            promise.set_exception(std::make_exception_ptr(std::runtime_error("Lỗi khi fetch dữ liệu!")));
        } else {
            promise.set_value(42);
        }
    });
    return promise;
}

int main() {
    auto looper = std::make_shared<SLLooper>();

    fetchWithError(looper, /*shouldThrow=*/true)
    .then(looper, [](int value) {
        std::cout << "Received value: " << value << std::endl;
        return value * 2;
    })
    .catchError(looper, [](std::exception_ptr ex) {
        try {
            if (ex) std::rethrow_exception(ex);
        } catch (const std::exception& e) {
            std::cout << "Đã bắt exception: " << e.what() << std::endl;
        }
        return -1;
    })
    .then(looper, [](int value) {
        std::cout << "Giá trị cuối cùng: " << value << std::endl;
        return 0;
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}