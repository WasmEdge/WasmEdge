# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_library(wasmedgePluginWasiNN
  SHARED
  wasinnenv.cpp
  wasinnfunc.cpp
  wasinnmodule.cpp
)

target_compile_options(wasmedgePluginWasiNN
  PUBLIC
  -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginWasiNN
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

target_include_directories(wasmedgePluginWasiNN
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgePluginWasiNN
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
)

install(TARGETS wasmedgePluginWasiNN DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)

wasmedge_add_library(wasmedgeHostModuleWasiNN
  wasinnenv.cpp
  wasinnfunc.cpp
  wasinnmodule.cpp
)

target_include_directories(wasmedgeHostModuleWasiNN
  PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgeHostModuleWasiNN
  PUBLIC
  wasmedgeCommon
  wasmedgeSystem
  wasmedgePlugin
)

if(CMAKE_SYSTEM_NAME MATCHES "Linux")
  target_link_options(wasmedgeHostModuleWasiNN
    PUBLIC
    -u_ZN8WasmEdge4Host6WASINN17WasiNNEnvironment8RegisterE
  )
elseif(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgeHostModuleWasiNN
    PUBLIC
    -u__ZN8WasmEdge4Host6WASINN17WasiNNEnvironment8RegisterE
  )
endif()

# Add backends building flags.
foreach(BACKEND ${WASMEDGE_WASINN_BACKEND})
  if(BACKEND MATCHES "OpenVINO")
    message(STATUS "Build ${BACKEND} backend for WASI-NN")
    find_package(InferenceEngine REQUIRED)
    add_definitions(-DWASMEDGE_WASINN_BACKEND_OPENVINO)
    target_link_libraries(wasmedgePluginWasiNN
      PUBLIC
      ${InferenceEngine_LIBRARIES}
    )
    target_link_libraries(wasmedgeHostModuleWasiNN
      PUBLIC
      ${InferenceEngine_LIBRARIES}
    )
  else()
    # Add the other backends here.
    message(FATAL_ERROR "WASI-NN backend ${BACKEND} not found or unimplemented.")
  endif()
endforeach()
