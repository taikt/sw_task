cmake_minimum_required(VERSION 3.16)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/heads/main.zip
)
FetchContent_MakeAvailable(googletest)
enable_testing()


file(GLOB SRC_FILES "../src/*.cpp")
list(FILTER SRC_FILES EXCLUDE REGEX "test_simple.cpp$")

add_executable(test_handler test_handler.cpp ${SRC_FILES})
target_include_directories(test_handler PRIVATE ../include)
target_link_libraries(test_handler gtest_main pthread)
add_test(NAME HandlerTest COMMAND test_handler)

add_executable(test_postwork test_postwork.cpp ${SRC_FILES})
target_include_directories(test_postwork PRIVATE ../include)
target_link_libraries(test_postwork gtest_main pthread)
add_test(NAME PostWorkTest COMMAND test_postwork)