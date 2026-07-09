# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

# Function for preparing the PyTorch dependency.
function(wasmedge_setup_pytorch_target target)
  find_package(Torch REQUIRED)
  target_link_libraries(${target}
    PRIVATE
    ${TORCH_LIBRARIES}
  )
  target_compile_options(${target}
    PRIVATE
    -Wno-error=unused-parameter
  )
endfunction()
