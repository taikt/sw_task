#include "TimerManager.h"
#include "Message.h"
#include <pthread.h>

struct TimerContext {
    std::weak_ptr<Handler> handler;
    int messageId;
};

TimerManager::TimerManager(std::shared_ptr<Handler> handler)
    : mHandler(handler) {}

TimerManager::~TimerManager() {}

timer_t TimerManager::startTimer(int messageId, int timeoutMs) {
    struct sigevent sev{};
    struct itimerspec its{};
    timer_t timerId;

    auto ctx = new TimerContext{mHandler, messageId};

    sev.sigev_notify = SIGEV_THREAD;
    sev.sigev_notify_function = timerThreadFunc;
    sev.sigev_value.sival_ptr = ctx;

    if (timer_create(CLOCK_REALTIME, &sev, &timerId) == -1)
        return (timer_t)0;

    its.it_value.tv_sec = timeoutMs / 1000;
    its.it_value.tv_nsec = (timeoutMs % 1000) * 1000000;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    timer_settime(timerId, 0, &its, nullptr);
    return timerId;
}

void TimerManager::stopTimer(timer_t timerId) {
    timer_delete(timerId);
}

void TimerManager::timerThreadFunc(union sigval sv) {
    // Detach thread to avoid memory leaks
    pthread_detach(pthread_self());

    auto ctx = static_cast<TimerContext*>(sv.sival_ptr);
    if (ctx && !ctx->handler.expired()) {
        auto handler = ctx->handler.lock();
        if (handler) {
            handler->sendMessage(handler->obtainMessage(ctx->messageId));
        }
    }
    delete ctx;
}