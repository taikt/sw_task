cmake_minimum_required(VERSION 3.20)
project(sw_task_examples)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# cmake -DCMAKE_BUILD_TYPE=Debug ..  (enable DEBUG macro)
# cmake -DCMAKE_BUILD_TYPE=Release .. (disable DEBUG macro)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    # GCC coroutine support
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    # Clang coroutine support  
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fcoroutines-ts")
    # Clang có thể cần stdlib=libc++ trên một số hệ thống
    # set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

include_directories(../../include)

set(SW_TASK_SRC
    ./../../src/SLLooper.cpp
    ./../../src/EventQueue.cpp
    ./../../src/Promise.cpp
    ./../../src/Handler.cpp
    ./../../src/Message.cpp
    ./../../src/State.cpp
    ./../../src/Buffer.cpp
    ./../../src/TimerManager.cpp
    ./../../src/Timer.cpp
)

# Debug options
option(ENABLE_TIMER_DEBUG "Enable Timer debug logging" OFF)
option(ENABLE_SLLOOPER_DEBUG "Enable SLLooper debug logging" OFF)
option(ENABLE_EventQueue_DEBUG "Enable EventQueue debug logging" OFF)
option(ENABLE_ALL_DEBUG "Enable all debug logging" OFF)

if(ENABLE_ALL_DEBUG)
    add_compile_definitions(
        TIMER_DEBUG_ENABLED=1
        SLLOOPER_DEBUG_ENABLED=1
        EventQueue_DEBUG_ENABLED=1
    )
    message(STATUS "All debug logging ENABLED")
else()
    # Enable individual components
    if(ENABLE_TIMER_DEBUG)
        add_compile_definitions(TIMER_DEBUG_ENABLED=1)
        message(STATUS "Timer debug logging ENABLED")
    endif()
    
    if(ENABLE_SLLOOPER_DEBUG)
        add_compile_definitions(SLLOOPER_DEBUG_ENABLED=1)
        message(STATUS "SLLooper debug logging ENABLED")
    endif()
    
    if(ENABLE_EventQueue_DEBUG)
        add_compile_definitions(EventQueue_DEBUG_ENABLED=1)
        message(STATUS "EventQueue debug logging ENABLED")
    endif()
endif()

# Show what's enabled
if(NOT ENABLE_ALL_DEBUG AND NOT ENABLE_TIMER_DEBUG AND NOT ENABLE_SLLOOPER_DEBUG AND NOT ENABLE_EventQueue_DEBUG)
    message(STATUS "Production build - no debug logging")
endif()


add_executable(timer_load timer_operation/timer_load.cpp ${SW_TASK_SRC})
target_link_libraries(timer_load pthread)

add_executable(cpu_task cpu_bound_task/cpu_task.cpp ${SW_TASK_SRC})
target_link_libraries(cpu_task pthread)

add_executable(response_time task_execution/response_time.cpp ${SW_TASK_SRC})
target_link_libraries(response_time pthread)

add_executable(co_await ../examples/co_await.cpp ${SW_TASK_SRC})
target_link_libraries(co_await pthread)

# Debug info về compiler và coroutine support
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "10.0")
    message(WARNING "GCC version < 10.0 may have limited coroutine support")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.0") 
    message(WARNING "Clang version < 12.0 may have limited coroutine support")
endif()