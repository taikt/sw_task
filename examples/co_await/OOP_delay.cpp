// awaitPost và awaitDelay giải quyết hai nhu cầu khác nhau:

// 1) thực thi công việc bất đồng bộ ngay khi có thể (awaitPost), và
// 2) tạm ngưng coroutine cho tới một mốc thời gian nhất định mà không “chiếm” luồng.
// Khác biệt cốt lõi
// awaitPost(f):

// Đẩy một hàm/lambda vào SLLooper để thực thi ngay khi looper rảnh.
// Coroutine tạm suspend, looper chạy f, lấy kết quả, rồi resume coroutine.
// Thời điểm resume phụ thuộc độ bận của hàng đợi, KHÔNG phụ thuộc một delay bạn chọn.
// Dùng cho thực thi logic/do tính toán/IO đã sẵn sàng.
// awaitDelay(ms):

// Tạo (hoặc sử dụng) một timer qua TimerManager (timerfd + epoll).
// Suspend coroutine cho tới khi timer hết hạn (không tiêu tốn CPU, không block looper).
// Khi timeout xảy ra, looper nhận sự kiện timer và resume coroutine.
// Dùng để “chờ thời gian”: pacing, debounce, retry với backoff, interval lặp định kỳ, timeout logic.
// Vì sao cần awaitDelay nếu đã có bất đồng bộ?
// “Bất đồng bộ” chỉ nói rằng bạn không chờ theo kiểu block thread. Nó KHÔNG cung cấp ngữ nghĩa thời gian. 
// Các lý do thực tế:

// - Pacing / Rate limiting: Giữa hai lần gọi API cần cách nhau X ms.
// - Retry backoff: Sau khi lỗi, chờ 500 ms rồi thử lại (không block looper bằng sleep).
// - Timeout: Đợi sự kiện khác xảy ra; nếu quá 1000 ms thì thoát (dùng co_await awaitDelay như một nhánh race).
// -Thực thi định kỳ: 
// Vòng lặp coroutine: xử lý → awaitDelay(interval) → lặp lại (ít drift hơn so với tự post + sleep).
// - Giải phóng luồng: Thay vì std::this_thread::sleep_for trong lambda của awaitPost 
// (sẽ block thread looper), awaitDelay tách thời gian chờ ra khỏi thực thi, cho phép looper 
// chạy việc khác trong khi coroutine đang suspend.
// - Tính đọc dễ dàng: Logic tuần tự: step1(); chờ 200 ms; step2(); tách rõ ràng giữa “làm việc” và “đợi”.
// - Fairness: Không ứ đọng hàng đợi bởi các lambda chứa sleep; ngủ trong lambda làm trễ mọi thứ sau nó.
// - Năng lượng/CPU: Timerfd + epoll, kernel đánh thức đúng lúc; sleep bận trong thread looper 
// sẽ giữ thread lâu và ngăn nhiệm vụ khác.
// - Kiểm soát drift: Bạn có thể lấy timestamp trước/ sau awaitDelay để bù trừ hoặc điều chỉnh 
// (ví dụ interval cố định).
// - Kết hợp dễ với cancellation: Timer có thể bị huỷ nếu coroutine bị destroy trước thời điểm hết hạn.


#include "SLLooper.h"
#include "Task.h"
#include <iostream>
#include <memory>

using namespace swt;

class Worker {
public:
    void prepare() {
        std::cout << "[Worker::prepare] Preparing work...\n";
    }
    void finish() {
        std::cout << "[Worker::finish] Work finished.\n";
    }
};

// Coroutine: gọi prepare, chờ một khoảng delay, rồi gọi finish
Task<void> demo(std::shared_ptr<SLLooper> looper, std::shared_ptr<Worker> w) {
    std::cout << "[demo] Begin\n";

    // Bước 1
    co_await looper->awaitPost([w] { w->prepare(); });

    // Chờ 500 ms (awaitDelay trả về void)
    std::cout << "[demo] Waiting 500 ms...\n";
    co_await looper->awaitDelay(500);  // 500 milliseconds

    // Bước 2
    co_await looper->awaitPost([w] { w->finish(); });

    std::cout << "[demo] Done\n";
}

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto worker = std::make_shared<Worker>();

    auto t = demo(looper, worker);
    t.start(); // Task là lazy, phải start

    // Chờ một chút cho coroutine chạy xong
    std::this_thread::sleep_for(std::chrono::milliseconds(700));
    return 0;
}