# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgeGC
  allocator.cpp
)

target_link_libraries(wasmedgeGC
  PUBLIC
  wasmedgeSystem
)
