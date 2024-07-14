# Copyright 2024 Yonggang Luo
# SPDX-License-Identifier: MIT
cmake_minimum_required(VERSION 3.17)
if ("${CMAKE_BINARY_DIR}" STREQUAL "${CMAKE_CURRENT_BINARY_DIR}")
  project(win-polyfill-loader VERSION 0.1.0.0 LANGUAGES C CXX)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
set(CMAKE_CXX_STANDARD 23)
string(APPEND CMAKE_CXX_FLAGS " -wd4312 -wd4311")

include(cmake/CpkHelpers.cmake)

if ("${CMAKE_MSVC_RUNTIME_LIBRARY}" STREQUAL "")
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_subdirectory(src/deps/phnt)

if(BUILD_TESTING)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
