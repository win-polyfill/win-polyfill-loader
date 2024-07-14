# Copyright 2024 Yonggang Luo
# SPDX-License-Identifier: MIT

add_library(win-polyfill-loader SHARED
  LdrEntry.cpp
  BaseAddressIndex.cpp
  ImportTable.cpp
  ../deps/win32-crt/win32_crt_seh.cpp
  Main.cpp
)
target_link_libraries(win-polyfill-loader PRIVATE
  win-polyfill-phnt
  ntdll
)
target_compile_options(win-polyfill-loader PRIVATE /Zl /GR- /GS-)
