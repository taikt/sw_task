#include "SLLooper.h"
#include "Handler.h"
#include "Message.h"
#include <iostream>
#include <memory>
#include <vector>
#include <chrono>
#include <thread>

using namespace swt;

enum Msg : int32_t {
    MSG_LOAD = 1,
    MSG_FETCH = 2,
    MSG_TRANSFORM = 3,
    MSG_COMPUTE = 4,
    MSG_SAVE = 5
};

struct PipelineState {
    int factor = 0;
    std::vector<int> raw;
    std::vector<int> transformed;
    int metric = 0;
    bool saved = false;
};

class PipelineHandler : public Handler {
public:
    PipelineHandler(std::shared_ptr<SLLooper>& looper) : Handler(looper) {}

    void start() {
        sendMessage(obtainMessage(MSG_LOAD));
    }

    void handleMessage(const std::shared_ptr<Message>& msg) override {
        switch (msg->what) {
        case MSG_LOAD: {
            std::cout << "[MSG_LOAD]\n";
            state.factor = loadConfig();
            sendMessage(obtainMessage(MSG_FETCH));
            break;
        }
        case MSG_FETCH: {
            std::cout << "[MSG_FETCH]\n";
            state.raw = fetchData();
            sendMessage(obtainMessage(MSG_TRANSFORM));
            break;
        }
        case MSG_TRANSFORM: {
            std::cout << "[MSG_TRANSFORM]\n";
            state.transformed = transformData(state.raw, state.factor);
            sendMessage(obtainMessage(MSG_COMPUTE));
            break;
        }
        case MSG_COMPUTE: {
            std::cout << "[MSG_COMPUTE]\n";
            state.metric = computeMetric(state.transformed);
            sendMessage(obtainMessage(MSG_SAVE));
            break;
        }
        case MSG_SAVE: {
            std::cout << "[MSG_SAVE]\n";
            state.saved = saveResult(state.metric);
            std::cout << "[pipeline] saved=" << std::boolalpha << state.saved << "\n";
            if (auto sp = getLooper()) sp->exit();
            break;
        }
        default:
            std::cout << "[Handler] unknown what=" << msg->what << "\n";
            break;
        }
    }

private:
    PipelineState state;

    // Các hàm logic (cùng thread)
    int loadConfig() {
        std::cout << "[loadConfig]\n";
        return 5;
    }
    std::vector<int> fetchData() {
        std::cout << "[fetchData]\n";
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
        int sum = 0; for (int v: data) sum += v; return sum;
    }
    bool saveResult(int metric) {
        std::cout << "[saveResult] metric=" << metric << "\n";
        return metric > 0;
    }
};

int main() {
    auto looper = std::make_shared<SLLooper>();
    auto handler = std::make_shared<PipelineHandler>(looper);
    handler->start();

    std::this_thread::sleep_for(std::chrono::milliseconds(220));
    std::cout << "[main] exit\n";
    return 0;
}