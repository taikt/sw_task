#pragma once
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <optional>
#include <cstdint>
#include <chrono>
#include <curl/curl.h> // Quan trọng để tránh xung đột typedef

namespace sw_task::net {

enum class HttpMethod { Get, Post, Put, Delete, Patch, Head, Options };

enum class HttpErrorCode {
    Network,
    Timeout,
    Canceled,
    Resolve,
    SSL,
    Protocol,
    Internal
};

struct HttpError {
    HttpErrorCode code;
    std::string message;
};

struct HttpResponse {
    long statusCode{0};
    std::string body;
    std::string effectiveUrl;
    std::unordered_map<std::string,std::string> headers;
    std::optional<HttpError> error;
    double elapsedMs{0.0};

    bool ok() const {
        return !error.has_value() && statusCode >= 200 && statusCode < 300;
    }
};

struct HttpRequest {
    using CompleteFn = std::function<void(const HttpResponse&)>;
    using ProgressFn = std::function<void(std::size_t, std::size_t)>;

    HttpMethod method{HttpMethod::Get};
    std::string url;
    std::vector<std::pair<std::string,std::string>> headers;
    std::string body;
    uint32_t timeoutMs{0};
    CompleteFn onComplete;
    ProgressFn onProgress;

    void addHeader(std::string name, std::string value) {
        headers.emplace_back(std::move(name), std::move(value));
    }
};

class HttpService {
public:
    using RequestId = std::uint64_t;

    HttpService();
    ~HttpService();

    RequestId submit(HttpRequest req);
    bool cancel(RequestId id);

    void setMaxConcurrent(std::size_t n);
    std::size_t pending() const;
    std::size_t activeCount() const;
    std::size_t queuedCount() const;

    void shutdown();

private:
    struct RequestCtx {
        RequestId id;
        HttpRequest req;
        CURL* easy{nullptr};
        bool canceled{false};
        std::string responseBody;
        std::unordered_map<std::string,std::string> respHeaders;
        std::chrono::steady_clock::time_point startTime;
        curl_slist* headerList{nullptr}; // để cleanup
    };

    CURLM* multi_{nullptr};
    std::atomic<bool> running_{true};
    std::thread worker_;

    mutable std::mutex mtx_;
    std::condition_variable cv_;

    std::vector<RequestCtx> newQueue_;
    std::unordered_map<RequestId, RequestCtx> active_;
    std::unordered_map<CURL*, RequestId> easyToId_;

    std::size_t maxConcurrent_{16};
    RequestId nextId_{1};

    void runLoop();
    void addNewRequestsLocked();
    void performOnce(int waitTimeoutMs);
    void collectCompletions();
    void finishRequest(RequestCtx& ctx, CURLcode result);
    void cleanupEasy(RequestCtx& ctx);
    void setupEasy(RequestCtx& ctx);

    static size_t writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata);
    static size_t headerCallback(char* buffer, size_t size, size_t nitems, void* userdata);
};

} // namespace sw_task::net