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

add_compile_options("/EHa" "/execution-charset:utf-8" "-DUNICODE" "-D_UNICODE")

set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /merge:.rdata=.text")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")

set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /merge:.rdata=.text")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /MANIFEST:NO")
string(REPLACE "/RTC1" "" CMAKE_CXX_FLAGS_DEBUG ${CMAKE_CXX_FLAGS_DEBUG})
string(REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/GR" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
string(REPLACE "/INCREMENTAL" "" CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG}")
string(REPLACE "/INCREMENTAL" "" CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG}")
string(APPEND CMAKE_CXX_FLAGS " -std:c++latest -Zc:threadSafeInit-")
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE,5.02")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:CONSOLE,5.02")
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    string(APPEND CMAKE_CXX_FLAGS " -mcx16")
  endif()
else()
  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_LINKER_TYPE STREQUAL "LLD")
    # https://github.com/llvm/llvm-project/issues/52791
    # https://reviews.llvm.org/D116358?large=true&id=&download=true
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE,4.00")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:CONSOLE,4.00")
  else()
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE,5.01")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /SUBSYSTEM:CONSOLE,5.01")
  endif()
endif()

# CMake bug, can not specify subsystem version, workaround it
string(REPLACE "/subsystem:windows" " " CMAKE_CXX_CREATE_WIN32_EXE "${CMAKE_CXX_CREATE_WIN32_EXE}")
string(REPLACE "/subsystem:console" " " CMAKE_CXX_CREATE_CONSOLE_EXE "${CMAKE_CXX_CREATE_CONSOLE_EXE}")
string(REPLACE "/subsystem:windows" " " CMAKE_C_CREATE_WIN32_EXE "${CMAKE_C_CREATE_WIN32_EXE}")
string(REPLACE "/subsystem:consol" " " CMAKE_C_CREATE_CONSOLE_EXE "${CMAKE_C_CREATE_CONSOLE_EXE}")

add_subdirectory(src/deps/phnt)
add_subdirectory(src/loader)

if(BUILD_TESTING)
  include(CTest)
  enable_testing()
  add_subdirectory(tests)
endif()
