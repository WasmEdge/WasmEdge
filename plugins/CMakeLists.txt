# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

add_subdirectory(wasmedge_process)
if(WASMEDGE_WASINN_BACKEND)
  add_subdirectory(wasi_nn)
endif()
