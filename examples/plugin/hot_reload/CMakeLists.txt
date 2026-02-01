# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

cmake_minimum_required(VERSION 3.16)
project(WasmEdgeHotReloadExample)

find_package(wasmedge CONFIG REQUIRED)

add_executable(hot_reload_example
  hot_reload_example.cpp
)

target_link_libraries(hot_reload_example
  PRIVATE
  wasmedge
)

target_compile_features(hot_reload_example PRIVATE cxx_std_17)
