cmake_minimum_required(VERSION 2.8.8)
include(ExternalProject)

find_package(Threads REQUIRED)

# Prefer static libraries.
if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Download gtest.
set(GTEST_DIR "${CMAKE_CURRENT_BINARY_DIR}/gtest")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(MAKE_DIRECTORY ${GENERATED_DIR})
file(WRITE ${GENERATED_DIR}/CMakeLists.txt
    "cmake_minimum_required(VERSION 2.8.8)\n"
    "add_subdirectory(googletest)\n"
)

ExternalProject_Add(gtest
    GIT_REPOSITORY https://github.com/google/googletest.git
    PREFIX "${GTEST_DIR}"
    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/generated/CMakeLists.txt ${GTEST_DIR}/src/gtest/
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(gtest source_dir binary_dir)
set(GTEST_INCLUDE_DIR "${source_dir}/googletest/include" PARENT_SCOPE)
set(GTEST_LIBS_DIR "${binary_dir}/googletest" PARENT_SCOPE)
set(GTEST_LIB_PATH "${binary_dir}/googletest/${CMAKE_FIND_LIBRARY_PREFIXES}gtest${CMAKE_FIND_LIBRARY_SUFFIXES}")

add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)
set_target_properties(libgtest PROPERTIES
    IMPORTED_LOCATION "${GTEST_LIB_PATH}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
)

