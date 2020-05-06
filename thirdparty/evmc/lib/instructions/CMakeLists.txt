# EVMC: Ethereum Client-VM Connector API.
# Copyright 2018-2019 The EVMC Authors.
# Licensed under the Apache License, Version 2.0.

add_library(utilEVMCInstructions
  STATIC
  instruction_metrics.c
  instruction_names.c
)

target_include_directories(utilEVMCInstructions
  PUBLIC
  ${PROJECT_SOURCE_DIR}/thirdparty/evmc/include
)
