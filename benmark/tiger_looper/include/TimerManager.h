#include <signal.h>
#include <time.h>
#include <functional>
#include <memory>
#include "Handler.h"

class TimerManager {
public:
    using TimerCallback = std::function<void()>;

    TimerManager(std::shared_ptr<Handler> handler);
    ~TimerManager();

    // Đăng ký timer, trả về timer id
    timer_t startTimer(int messageId, int timeoutMs);

    // Hủy timer
    void stopTimer(timer_t timerId);

private:
    static void timerThreadFunc(union sigval sv);

    std::shared_ptr<Handler> mHandler;
};