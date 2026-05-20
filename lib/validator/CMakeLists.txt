# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgeValidator
  formchecker.cpp
  validator.cpp
  component_context.cpp
  component_validator.cpp
  component_name.cpp
)

target_link_libraries(wasmedgeValidator
  PUBLIC
  wasmedgeCommon
  # The validator reuses the executor's pure type-shape helpers
  # (flatten_functype / containsListOrString in
  # include/executor/component/canonical_abi.h) for the canon lift/lower
  # flatten-derived checks. The dep is one-way — executor does not link back
  # to the validator.
  wasmedgeExecutor
)
