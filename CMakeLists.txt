cmake_minimum_required(VERSION 3.10)
project(sw_task)

# Thêm tất cả file nguồn
file(GLOB SW_TASK_SRC
    src/*.cpp
)

# Tạo shared library
add_library(sw_task SHARED ${SW_TASK_SRC})

# Thêm include directory nếu có
target_include_directories(sw_task PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)