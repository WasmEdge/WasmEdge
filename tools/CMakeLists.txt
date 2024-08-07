# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

if(WASMEDGE_BUILD_TOOLS)
  add_subdirectory(wasmedge)
endif()
if(WASMEDGE_BUILD_FUZZING)
  add_subdirectory(fuzz)
endif()
