cmake_minimum_required(VERSION 3.10.2)

project("Antik Unit Tests" VERSION 0.1.0 DESCRIPTION "Antik C++ Library Google Unit Tests")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror -Wall -Wextra")

set(TEST_EXECUTABLE ${ANTIK_LIBRARY_NAME}_tests)

file(GLOB TEST_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/UTC*.cpp")

set(SOURCES ${TEST_SOURCES})

add_executable(${TEST_EXECUTABLE} ${TEST_SOURCES})
target_include_directories(${TEST_EXECUTABLE} PUBLIC ../include ../classes/implementation)

add_test(NAME ${TEST_EXECUTABLE} COMMAND ${TEST_EXECUTABLE})

target_link_libraries(${TEST_EXECUTABLE} PUBLIC gtest_main antik gtest)