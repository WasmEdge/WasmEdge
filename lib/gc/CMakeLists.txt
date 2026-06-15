# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_library(wasmedgeGC
  allocator.cpp
)

target_link_libraries(wasmedgeGC
  PUBLIC
  wasmedgeSystem
  Threads::Threads
)
