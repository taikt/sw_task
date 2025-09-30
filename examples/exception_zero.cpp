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


Promise<int> fetchData(std::shared_ptr<SLLooper> looper) {
    auto promise = looper->createPromise<int>();
    looper->post([promise]() mutable {
        std::cout << "Start fetching..." << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        promise.set_value(10);
    });
    return promise;
}

int main() {
    auto looper = std::make_shared<SLLooper>();

    fetchData(looper)
    .then(looper, [](int value) {
        std::cout << "Step 1: value = " << value << std::endl;
        int zero = 0;
        // khong bat duoc exception chia cho 0
        int result = value / zero; // Lỗi chia cho 0 (undefined behavior)
        std::cout << "Result: " << result << std::endl;
        return result;
    })
    .catchError(looper, [](std::exception_ptr ex) {
        try {
            if (ex) std::rethrow_exception(ex);
        } catch (const std::exception& e) {
            std::cout << "Đã bắt exception: " << e.what() << std::endl;
        }
        return -1;
    });

    std::this_thread::sleep_for(std::chrono::seconds(1));
    return 0;
}