# SPDX-License-Identifier: Apache-2.0

add_library(ssvmHostModuleONNC
  onncfunc.cpp
  onncmodule.cpp
)

if(ONNC_WASM_LIBRARY)
  target_link_libraries(ssvmHostModuleONNC
    PRIVATE
    ${ONNC_WASM_LIBRARY}
  )
endif()
