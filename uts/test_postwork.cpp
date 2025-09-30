#include <gtest/gtest.h>
#include "SLLooper.h"
#include <memory>
#include <thread>
#include <chrono>

TEST(PostWorkTest, BasicReturnValue) {
    std::cout << "-- Starting BasicReturnValue test" << std::endl;
    auto looper = std::make_shared<SLLooper>();
    auto promise = looper->postWork([]() -> int {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return 123;
    });

    // Chain result
    bool then_called = false;
    promise.then(looper, [&](int result) {
        EXPECT_EQ(result, 123);
        then_called = true;
        return result + 1;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    EXPECT_TRUE(then_called);
}

TEST(PostWorkTest, VoidReturn) {
    std::cout << "-- Starting VoidReturn test" << std::endl;
    auto looper = std::make_shared<SLLooper>();
    auto promise = looper->postWork([]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    });

    bool then_called = false;
    promise.then(looper, [&]() {
        then_called = true;
    });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    EXPECT_TRUE(then_called);
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}