cmake_minimum_required(VERSION 3.20)
project(message_passing LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

#set(Boost_USE_STATIC_LIBS ON)
#set(Boost_USE_MULTITHREADED ON)

#set(Boost_NO_BOOST_CMAKE OFF)

set(BOOST_DIR "C:/Program Files/boost/boost_1_88_0/tools/cmake/")

#SET(CMAKE_INCLUDE_PATH ${CMAKE_INCLUDE_PATH} "C:/Program Files/boost/boost_1_88_0")
#SET(CMAKE_LIBRARY_PATH ${CMAKE_LIBRARY_PATH} "C:/Program Files/boost/boost_1_88_0/libs")

find_package(Boost 1.88 REQUIRED CONFIG COMPONENTS interprocess system filesystem process)
include_directories(${Boost_INCLUDE_DIRS})

if(Boost_FOUND)
	add_executable(Receiver
    Receiver.cpp
    Common.h
	)
    target_link_libraries(Receiver 
	Boost::interprocess
    Boost::system
    Boost::filesystem
    Boost::process
	)
	add_executable(Sender
    Sender.cpp
    Common.h
	)
    target_link_libraries(Sender 
	Boost::interprocess
    Boost::system
	)
	include(FetchContent)
	FetchContent_Declare(
		googletest
		GIT_REPOSITORY https://github.com/google/googletest.git
		GIT_TAG release-1.12.1
	)
	set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
	FetchContent_MakeAvailable(googletest)
	add_executable(Test tests.cpp Common.h)
    target_link_libraries(Test ${Boost_LIBRARIES} gtest_main)
    message(STATUS "Found BOOST ${Boost_VERSION_STRING}")
else()
    message(STATUS "WEBSOCKET BOOST Not Found")
endif()
