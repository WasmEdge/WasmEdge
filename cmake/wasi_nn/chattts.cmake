# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

include_guard()

# Function for preparing the chatTTS dependency.
function(wasmedge_setup_chattts_target target)
  wasmedge_setup_simdjson()
  find_package(Python3 COMPONENTS Interpreter Development)
  if(NOT Python3_FOUND)
    message(FATAL_ERROR "Can not find python3.")
  endif()
  target_compile_definitions(${target}
    PRIVATE
    PYTHON_LIB_PATH="${Python3_LIBRARIES}"
  )
  target_include_directories(${target}
    PRIVATE
    ${Python3_INCLUDE_DIRS}
  )
  target_link_directories(${target}
    PRIVATE
    ${Python3_RUNTIME_LIBRARY_DIRS}
  )
  target_link_libraries(${target}
    PRIVATE
    ${Python3_LIBRARIES}
    simdjson::simdjson
  )
endfunction()
