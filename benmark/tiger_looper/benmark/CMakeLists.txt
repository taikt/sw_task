cmake_minimum_required(VERSION 3.10)
project(binary_build)

include_directories(./../include)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

# ✅ THÊM: Debug symbols cho Valgrind
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -g -O0 -fno-omit-frame-pointer")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2 -DNDEBUG")

# ✅ THÊM: Valgrind build option
option(ENABLE_VALGRIND "Enable Valgrind-friendly build (debug symbols, no optimization)" OFF)
if(ENABLE_VALGRIND)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fno-omit-frame-pointer -fno-inline")
    message(STATUS "🔍 Valgrind build ENABLED - debug symbols, no optimization")
endif()

set(LOOPER_SRC
    ./../src/SLLooper.cpp
    ./../src/Handler.cpp
    ./../src/Message.cpp
    ./../src/MessageQueue.cpp
    ./../src/TimerManager.cpp
)


# add_executable(tiger_timer_load timer_operation/tiger_timer_load.cpp ${LOOPER_SRC})
# target_include_directories(tiger_timer_load PRIVATE include)

# add_executable(tiger_cpu_task cpu_bound_task/tiger_cpu_task.cpp ${LOOPER_SRC})
# target_include_directories(tiger_cpu_task PRIVATE include)

# add_executable(tiger_timer_load_light timer_operation/tiger_timer_load_light.cpp ${LOOPER_SRC})
# target_include_directories(tiger_timer_load_light PRIVATE include)

# add_executable(tiger_response_time task_execution/tiger_response_time.cpp ${LOOPER_SRC})
# target_include_directories(tiger_response_time PRIVATE include)

add_executable(tiger_oneshot timer_test/tiger_oneshot.cpp ${LOOPER_SRC})
target_include_directories(tiger_oneshot PRIVATE include)