cmake_minimum_required(VERSION 2.8.8)
include(ExternalProject)

find_package(Threads REQUIRED)

# Prefer static libraries.
if(WIN32)
    set(CMAKE_FIND_LIBRARY_SUFFIXES .lib)
else()
    set(CMAKE_FIND_LIBRARY_SUFFIXES .a)
endif()

# Download gmock.
set(GMOCK_DIR "${CMAKE_CURRENT_BINARY_DIR}/gmock")
set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")

file(MAKE_DIRECTORY ${GENERATED_DIR})
file(WRITE ${GENERATED_DIR}/CMakeLists.txt
    "cmake_minimum_required(VERSION 2.8.8)\n"
    "add_subdirectory(googlemock)\n"
)

ExternalProject_Add(gmock
    GIT_REPOSITORY https://github.com/google/googletest.git
    PREFIX "${GMOCK_DIR}"
    UPDATE_COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_BINARY_DIR}/generated/CMakeLists.txt ${GMOCK_DIR}/src/gmock/
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(gmock source_dir binary_dir)
set(GMOCK_INCLUDE_DIR "${source_dir}/googlemock/include" PARENT_SCOPE)
set(GMOCK_LIBS_DIR "${binary_dir}/googlemock" PARENT_SCOPE)
set(GMOCK_LIB_PATH "${binary_dir}/googlemock/${CMAKE_FIND_LIBRARY_PREFIXES}gmock${CMAKE_FIND_LIBRARY_SUFFIXES}")

add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gmock)
set_target_properties(libgmock PROPERTIES
    IMPORTED_LOCATION "${GMOCK_LIB_PATH}"
    IMPORTED_LINK_INTERFACE_LIBRARIES "${CMAKE_THREAD_LIBS_INIT}"
)

