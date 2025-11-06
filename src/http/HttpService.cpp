#include "http/HttpService.h"
#include <cassert>
#include <iostream> // (Có thể bỏ nếu không cần log)

namespace sw_task::net {

HttpService::HttpService() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    multi_ = curl_multi_init();
    worker_ = std::thread([this] { runLoop(); });
}

HttpService::~HttpService() {
    shutdown();
    if (worker_.joinable()) worker_.join();

    for (auto& [id, ctx] : active_) {
        if (ctx.easy) {
            curl_multi_remove_handle(multi_, ctx.easy);
            curl_easy_cleanup(ctx.easy);
        }
        if (ctx.headerList) {
            curl_slist_free_all(ctx.headerList);
        }
    }
    curl_multi_cleanup(multi_);
    curl_global_cleanup();
}

void HttpService::shutdown() {
    bool expected = true;
    if (running_.compare_exchange_strong(expected, false)) {
        cv_.notify_all();
    }
}

HttpService::RequestId HttpService::submit(HttpRequest req) {
    std::unique_lock lk(mtx_);
    RequestCtx ctx;
    ctx.id = nextId_++;
    ctx.req = std::move(req);
    ctx.startTime = std::chrono::steady_clock::now();
    newQueue_.push_back(std::move(ctx));
    lk.unlock();
    cv_.notify_one();
    return ctx.id;
}

bool HttpService::cancel(RequestId id) {
    std::lock_guard lk(mtx_);
    for (auto& ctx : newQueue_) {
        if (ctx.id == id) {
            ctx.canceled = true;
            return true;
        }
    }
    auto it = active_.find(id);
    if (it != active_.end()) {
        it->second.canceled = true;
        return true;
    }
    return false;
}

void HttpService::setMaxConcurrent(std::size_t n) {
    maxConcurrent_ = n;
}

std::size_t HttpService::pending() const {
    std::lock_guard lk(mtx_);
    return newQueue_.size() + active_.size();
}

std::size_t HttpService::activeCount() const {
    std::lock_guard lk(mtx_);
    return active_.size();
}

std::size_t HttpService::queuedCount() const {
    std::lock_guard lk(mtx_);
    return newQueue_.size();
}

void HttpService::runLoop() {
    while (running_) {
        {
            std::unique_lock lk(mtx_);
            cv_.wait_for(lk, std::chrono::milliseconds(50), [this] {
                return !newQueue_.empty() || !running_;
            });
            addNewRequestsLocked();
        }
        if (!running_) break;

        long curlTimeoutMs = -1;
        curl_multi_timeout(multi_, &curlTimeoutMs);
        int waitMs;
        if (curlTimeoutMs < 0) waitMs = 100;
        else if (curlTimeoutMs == 0) waitMs = 0;
        else waitMs = static_cast<int>(curlTimeoutMs);

        performOnce(waitMs);
    }
    // Drain completions cuối vòng
    performOnce(0);
}

void HttpService::addNewRequestsLocked() {
    for (auto it = newQueue_.begin(); it != newQueue_.end();) {
        if (active_.size() >= maxConcurrent_) break;
        if (it->canceled) {
            if (it->headerList) curl_slist_free_all(it->headerList);
            it = newQueue_.erase(it);
            continue;
        }
        RequestCtx moved = std::move(*it);
        it = newQueue_.erase(it);
        auto emplaced = active_.emplace(moved.id, std::move(moved));
        RequestCtx& stableCtx = emplaced.first->second;
        setupEasy(stableCtx);
        curl_multi_add_handle(multi_, stableCtx.easy);
        easyToId_[stableCtx.easy] = stableCtx.id;
    }
}

void HttpService::setupEasy(RequestCtx& ctx) {
    CURL* easy = curl_easy_init();
    ctx.easy = easy;

    curl_easy_setopt(easy, CURLOPT_URL, ctx.req.url.c_str());
    curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, &HttpService::writeCallback);
    curl_easy_setopt(easy, CURLOPT_WRITEDATA, &ctx);
    curl_easy_setopt(easy, CURLOPT_HEADERFUNCTION, &HttpService::headerCallback);
    curl_easy_setopt(easy, CURLOPT_HEADERDATA, &ctx);
    curl_easy_setopt(easy, CURLOPT_NOPROGRESS, 1L);

    // Cho phép tự giải nén gzip/deflate
    curl_easy_setopt(easy, CURLOPT_ACCEPT_ENCODING, "");

#ifdef HTTP_SERVICE_VERBOSE
    curl_easy_setopt(easy, CURLOPT_VERBOSE, 1L);
#endif
    curl_easy_setopt(easy, CURLOPT_USERAGENT, "sw_task/1.0");

    if (!ctx.req.body.empty()) {
        curl_easy_setopt(easy, CURLOPT_POSTFIELDS, ctx.req.body.c_str());
        curl_easy_setopt(easy, CURLOPT_POSTFIELDSIZE, ctx.req.body.size());
    }
    if (ctx.req.timeoutMs > 0) {
        curl_easy_setopt(easy, CURLOPT_TIMEOUT_MS, ctx.req.timeoutMs);
    }

    if (ctx.req.method != HttpMethod::Get) {
        switch (ctx.req.method) {
            case HttpMethod::Post:    curl_easy_setopt(easy, CURLOPT_POST, 1L); break;
            case HttpMethod::Put:     curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "PUT"); break;
            case HttpMethod::Delete:  curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "DELETE"); break;
            case HttpMethod::Patch:   curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "PATCH"); break;
            case HttpMethod::Head:    curl_easy_setopt(easy, CURLOPT_NOBODY, 1L); break;
            case HttpMethod::Options: curl_easy_setopt(easy, CURLOPT_CUSTOMREQUEST, "OPTIONS"); break;
            default: break;
        }
    }

    for (auto& h : ctx.req.headers) {
        std::string line = h.first + ": " + h.second;
        ctx.headerList = curl_slist_append(ctx.headerList, line.c_str());
    }
    if (ctx.headerList) {
        curl_easy_setopt(easy, CURLOPT_HTTPHEADER, ctx.headerList);
    }
}

size_t HttpService::writeCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* ctx = static_cast<RequestCtx*>(userdata);
    assert(ctx->easy != nullptr && "Dangling RequestCtx pointer");
    std::size_t total = size * nmemb;
    if (total) ctx->responseBody.append(ptr, total);
    return total;
}

size_t HttpService::headerCallback(char* buffer, size_t size, size_t nitems, void* userdata) {
    auto* ctx = static_cast<RequestCtx*>(userdata);
    std::size_t len = size * nitems;
    std::string line(buffer, len);
    auto pos = line.find(':');
    if (pos != std::string::npos) {
        std::string key = line.substr(0, pos);
        std::string val = line.substr(pos + 1);
        while (!val.empty() && (val.front()==' ' || val.front()=='\t')) val.erase(val.begin());
        while (!val.empty() && (val.back()=='\r' || val.back()=='\n')) val.pop_back();
        if (!key.empty()) ctx->respHeaders[key] = val;
    }
    return len;
}

void HttpService::performOnce(int waitTimeoutMs) {
    int numfds = 0;
    curl_multi_wait(multi_, nullptr, 0, waitTimeoutMs, &numfds);

    int running = 0;
    curl_multi_perform(multi_, &running);
    collectCompletions();
}

void HttpService::collectCompletions() {
    int msgs = 0;
    CURLMsg* msg;
    while ((msg = curl_multi_info_read(multi_, &msgs))) {
        if (msg->msg == CURLMSG_DONE) {
            CURL* easy = msg->easy_handle;
            auto idIt = easyToId_.find(easy);
            if (idIt == easyToId_.end()) continue;
            RequestId id = idIt->second;

            std::lock_guard lk(mtx_);
            auto ctxIt = active_.find(id);
            if (ctxIt != active_.end()) {
                finishRequest(ctxIt->second, msg->data.result);
                cleanupEasy(ctxIt->second);
                active_.erase(ctxIt);
            }
            easyToId_.erase(easy);
        }
    }
}

void HttpService::finishRequest(RequestCtx& ctx, CURLcode result) {
    HttpResponse resp;
    resp.elapsedMs = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - ctx.startTime).count();

    long code = 0;
    curl_easy_getinfo(ctx.easy, CURLINFO_RESPONSE_CODE, &code);
    resp.statusCode = code;
    char* eff = nullptr;
    curl_easy_getinfo(ctx.easy, CURLINFO_EFFECTIVE_URL, &eff);
    if (eff) resp.effectiveUrl = eff;
    resp.body = std::move(ctx.responseBody);
    resp.headers = std::move(ctx.respHeaders);

    if (ctx.canceled) {
        resp.error = HttpError{HttpErrorCode::Canceled, "Canceled"};
    } else if (result != CURLE_OK) {
        HttpErrorCode ec = HttpErrorCode::Network;
        switch (result) {
            case CURLE_OPERATION_TIMEDOUT:      ec = HttpErrorCode::Timeout; break;
            case CURLE_COULDNT_RESOLVE_HOST:    ec = HttpErrorCode::Resolve; break;
            case CURLE_SSL_CONNECT_ERROR:       ec = HttpErrorCode::SSL; break;
            default: break;
        }
        resp.error = HttpError{ec, curl_easy_strerror(result)};
    }

    if (ctx.req.onComplete) {
        try {
            ctx.req.onComplete(resp);
        } catch (...) {
            // Nuốt exception để không làm vỡ worker loop.
        }
    }
}

void HttpService::cleanupEasy(RequestCtx& ctx) {
    curl_multi_remove_handle(multi_, ctx.easy);
    curl_easy_cleanup(ctx.easy);
    ctx.easy = nullptr;
    if (ctx.headerList) {
        curl_slist_free_all(ctx.headerList);
        ctx.headerList = nullptr;
    }
}

} // namespace sw_task::net