cmake_minimum_required(VERSION 3.14)
project(MessagePassing)

set(CMAKE_CXX_STANDARD 17)

add_executable(Receiver Receiver.cpp)
add_executable(Sender Sender.cpp)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(MessagePassingTests UnitTests.cpp)
target_link_libraries(MessagePassingTests gtest_main)
