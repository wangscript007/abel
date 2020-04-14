
include(carbin_debug)

set(CMAKE_PROJECT_DESCRIPTION "abel c++ lib")
set(AUTHOR_MAIL lijippy@163.com)
set(CMAKE_PROJECT_VERSION_MAJOR 1)
set(CMAKE_PROJECT_VERSION_MINOR 2)
set(CMAKE_PROJECT_VERSION_PATCH 0)
set(CMAKE_PROJECT_VERSION "${CMAKE_PROJECT_VERSION_MAJOR}.${CMAKE_PROJECT_VERSION_MINOR}.${CMAKE_PROJECT_VERSION_PATCH}")

option(CARBIN_STATUS_PRINT "cmake toolchain print" ON)
option(CARBIN_STATUS_DEBUG "cmake toolchain debug info" ON)

option(ENABLE_TESTING "enable unit test" OFF)
option(CARBIN_PACKAGE_GEN "enable package gen" OFF)
option(ENABLE_BENCHMARK "enable benchmark" OFF)
option(ENABLE_EXAMPLE "enable benchmark" OFF)

set(CARBIN_STD -std=c++11)

set(CMAKE_VERBOSE_MAKEFILE ON)

set(CMAKE_MACOSX_RPATH 1)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)
set(LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)


carbin_print("project will install to ${CMAKE_INSTALL_PREFIX}")

set(PACKAGE_INSTALL_PREFIX "/usr/local")