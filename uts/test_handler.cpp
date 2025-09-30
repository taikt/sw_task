// Google Test unit test for Handler and related classes
#include <gtest/gtest.h>
#include "Handler.h"
#include "SLLooper.h"
#include "Message.h"
#include "myhandler.h"
#include <memory>
#include <thread>
#include <chrono>

#ifndef TEST1
#define TEST1 1
#endif
#ifndef TEST2
#define TEST2 2
#endif

class HandlerTest : public ::testing::Test {
protected:
    void SetUp() override {
        looper = std::make_shared<SLLooper>();
        handler = std::make_shared<myhandler>(looper);
    }
    std::shared_ptr<SLLooper> looper;
    std::shared_ptr<myhandler> handler;
};

// Test obtainMessage (basic)
TEST_F(HandlerTest, ObtainMessage) {
    cout << "Testing obtainMessage\n";
    auto msg = handler->obtainMessage(TEST1);
    ASSERT_NE(msg, nullptr);
    EXPECT_EQ(msg->what, TEST1);
    EXPECT_EQ(msg->mHandler, handler);
}

// Test sendMessage (should return true and message is queued)
TEST_F(HandlerTest, SendMessage) {
    cout << "Testing sendMessage\n";
    auto msg = handler->obtainMessage(TEST1);
    bool sent = handler->sendMessage(msg);
    EXPECT_TRUE(sent);
}

// Test sendMessageDelayed (should return true)
TEST_F(HandlerTest, SendMessageDelayed) {
    cout << "Testing sendMessageDelayed\n";
    auto msg = handler->obtainMessage(TEST2);
    bool sent = handler->sendMessageDelayed(msg, 50); // 50ms delay
    EXPECT_TRUE(sent);
}

// Test hasMessages (should be true after sending, false after processing)
TEST_F(HandlerTest, HasMessages) {
    cout << "Testing hasMessages\n";
    auto msg = handler->obtainMessage(TEST1);
    handler->sendMessage(msg);
    EXPECT_TRUE(handler->hasMessages(TEST1));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_FALSE(handler->hasMessages(TEST1));
}

// Test removeMessages (hiện tại trả về false, kiểm tra đúng logic)
TEST_F(HandlerTest, RemoveMessages) {
    cout << "Testing removeMessages\n";
    EXPECT_FALSE(handler->removeMessages(TEST1));
    EXPECT_FALSE(handler->removeMessages(TEST1, nullptr));
}

// Test sendMessageAtTime (should return true)
TEST_F(HandlerTest, SendMessageAtTime) {
    cout << "Testing sendMessageAtTime\n";
    auto msg = handler->obtainMessage(TEST1);
    int64_t now = handler->uptimeMicros();
    bool sent = handler->sendMessageAtTime(msg, now + 100000); // 100ms sau
    EXPECT_TRUE(sent);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}