# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgeRuntime
  instance/component/function.cpp
)

target_link_libraries(wasmedgeRuntime
  PUBLIC
  wasmedgeCommon
)
