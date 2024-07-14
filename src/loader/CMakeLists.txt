# Copyright 2024 Yonggang Luo
# SPDX-License-Identifier: MIT

add_library(win-polyfill-loader STATIC
  BaseAddressIndex.cpp
  ImportTable.cpp
  Initialize.cpp
  InvertedFunctionTable.cpp
  LdrEntry.cpp
  LoadDllMemoryApi.cpp
  Loader.cpp
  MemoryModule.cpp
  Utils.cpp
)
target_link_libraries(win-polyfill-loader PRIVATE win-polyfill-phnt)
