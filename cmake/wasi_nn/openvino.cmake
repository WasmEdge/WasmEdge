# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

# Function for preparing the OpenVINO dependency.
function(wasmedge_setup_openvino_target target)
  find_package(OpenVINO REQUIRED)
  target_link_libraries(${target}
    PRIVATE
    openvino::runtime
    openvino::runtime::c
  )
endfunction()
