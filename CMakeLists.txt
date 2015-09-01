cmake_minimum_required(VERSION 2.8.8)
set(PROJECT_NAME_STR disruptor4cpp)
project(${PROJECT_NAME_STR} C CXX)

include(ExternalProject)

find_package(Threads REQUIRED)
if(UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")
endif()

#-------------------
# set common include folder for module
#-------------------
set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_TEST_DIR ${PROJECT_SOURCE_DIR}/test)
set(EXT_PROJECTS_DIR ${PROJECT_SOURCE_DIR}/ext)

#-------------------
# Module source
#-------------------
add_subdirectory(${EXT_PROJECTS_DIR}/gtest)
add_subdirectory(${EXT_PROJECTS_DIR}/gmock)

#-------------------
# Test
#-------------------
enable_testing()
set(PROJECT_TEST_NAME ${PROJECT_NAME_STR}_test)
file(GLOB TEST_SRC_FILES ${PROJECT_TEST_DIR}/*.cpp)
add_executable(${PROJECT_TEST_NAME} ${TEST_SRC_FILES})

add_dependencies(${PROJECT_TEST_NAME} gmock gtest)
include_directories(${GMOCK_INCLUDE_DIR} ${GTEST_INCLUDE_DIR} ${PROJECT_INCLUDE_DIR})

target_link_libraries(${PROJECT_TEST_NAME} libgmock)
target_link_libraries(${PROJECT_TEST_NAME} libgtest)
target_link_libraries(${PROJECT_TEST_NAME} ${CMAKE_THREAD_LIBS_INIT})

add_test(disruptor4cpp_test ${PROJECT_TEST_NAME})
