cmake_minimum_required(VERSION 3.10)
project(binary_build)

include_directories(include)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

set(LOOPER_SRC
    src/SLLooper.cpp
    src/Handler.cpp
    src/Message.cpp
    src/MessageQueue.cpp
    src/TimerManager.cpp
)

add_executable(testMessage examples/testMessage.cpp ${LOOPER_SRC})
target_include_directories(testMessage PRIVATE include)

add_executable(testTimer examples/testTimer.cpp ${LOOPER_SRC})
target_include_directories(testTimer PRIVATE include)

add_executable(tiger_task_runner examples/task_runner.cpp ${LOOPER_SRC})
target_include_directories(tiger_task_runner PRIVATE include)

add_executable(tiger examples/all_test.cpp ${LOOPER_SRC})
target_include_directories(tiger PRIVATE include)


