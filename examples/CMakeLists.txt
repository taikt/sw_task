cmake_minimum_required(VERSION 3.10)
project(sw_task_examples)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# cmake -DCMAKE_BUILD_TYPE=Debug ..  (enable DEBUG macro)
# cmake -DCMAKE_BUILD_TYPE=Release .. (disable DEBUG macro)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG")

include_directories(../include)

set(SW_TASK_SRC
    ../src/SLLooper.cpp
    ../src/EventQueue.cpp
    ../src/Promise.cpp
    ../src/Handler.cpp
    ../src/Message.cpp
    ../src/State.cpp
    ../src/Buffer.cpp
    ../src/TimerManager.cpp  
    ../src/Timer.cpp  
)

# Debug options
option(ENABLE_TIMER_DEBUG "Enable Timer debug logging" OFF)
option(ENABLE_SLLOOPER_DEBUG "Enable SLLooper debug logging" OFF)
option(ENABLE_EventQueue_DEBUG "Enable EventQueue debug logging" OFF)
option(ENABLE_ALL_DEBUG "Enable all debug logging" OFF)

# ✅ FIX: Sửa logic để có thể enable multiple components
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

add_executable(timer_load timer_load.cpp ${SW_TASK_SRC})
target_link_libraries(timer_load pthread)

add_executable(fetch_data fetch_data.cpp ${SW_TASK_SRC})
target_link_libraries(fetch_data pthread)

add_executable(post_work post_work.cpp ${SW_TASK_SRC})
target_link_libraries(post_work pthread)

add_executable(handle_message handle_message.cpp ${SW_TASK_SRC})
target_link_libraries(handle_message pthread)

add_executable(fetch_data_OOP fetch_data_OOP.cpp ${SW_TASK_SRC})
target_link_libraries(fetch_data_OOP pthread)

add_executable(timer timer.cpp ${SW_TASK_SRC})
target_link_libraries(timer pthread)

add_executable(timer_bind timer_bind.cpp ${SW_TASK_SRC})
target_link_libraries(timer_bind pthread)

add_executable(timer_lambda timer_lambda.cpp ${SW_TASK_SRC})
target_link_libraries(timer_lambda pthread)

add_executable(exception_out exception_out.cpp ${SW_TASK_SRC})
target_link_libraries(exception_out pthread)

add_executable(exception_throw exception_throw.cpp ${SW_TASK_SRC})
target_link_libraries(exception_throw pthread)

add_executable(exception_fetch exception_fetch.cpp ${SW_TASK_SRC})
target_link_libraries(exception_fetch pthread)

add_executable(exception_zero exception_zero.cpp ${SW_TASK_SRC})
target_link_libraries(exception_zero pthread)

add_executable(timer_vector timer_vector.cpp ${SW_TASK_SRC})
target_link_libraries(timer_vector pthread)

add_executable(task_runner task_runner.cpp ${SW_TASK_SRC})
target_link_libraries(task_runner pthread)

add_executable(response_time response_time.cpp ${SW_TASK_SRC})
target_link_libraries(response_time pthread)

add_executable(sw all_test.cpp ${SW_TASK_SRC})
target_link_libraries(sw pthread)


