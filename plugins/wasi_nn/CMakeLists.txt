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
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PUGLINS_STATIC)
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasiNN
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginWasiNN DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)

# Add backends building flags.
foreach(BACKEND ${WASMEDGE_PLUGIN_WASI_NN_BACKEND})
  if(BACKEND MATCHES "OpenVINO")
    message(STATUS "Build OpenVIN1O backend for WASI-NN")
    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_OPENVINO)
    if(NOT OPENVINO_VERSION)
      set(OPENVINO_VERSION "2022.2")
    endif()
    find_package(InferenceEngine QUIET)
    if(${InferenceEngine_FOUND})
      message(STATUS "OpenVINO has been installed in ${InferenceEngine_DIR}")
      target_link_libraries(wasmedgePluginWasiNN
        PUBLIC
        ${InferenceEngine_LIBRARIES}
      )
      break()
    endif()
    if(APPLE)
      message(STATUS "This is APPLE")
      set(WASMEDGE_OPENVINO_BIN_SUFFIX "${OPENVINO_VERSION}/macos/m_openvino_toolkit_osx_2022.2.0.7713.af16ea1d79a_x86_64.tgz")
      set(WASMEDGE_OPENVINO_HASH "450979453f254c630b459106d9682625be48a05a354760dfb86f993a9945a256")
    elseif(UNIX)
      execute_process(COMMAND bash -c "cat /etc/*release | grep ^NAME"
        RESULT_VARIABLE DOWNLOAD_ERROR 
        OUTPUT_VARIABLE OS_PLATFORM
        OUTPUT_STRIP_TRAILING_WHITESPACE)
        # install OpenVINO on Ubuntu
      if(${OS_PLATFORM} MATCHES "Ubuntu")
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
          execute_process(COMMAND bash -c "cat /etc/*release | grep ^VERSION_ID"
            RESULT_VARIABLE DOWNLOAD_ERROR 
            OUTPUT_VARIABLE OS_VERSION
            OUTPUT_STRIP_TRAILING_WHITESPACE)

            if(OS_VERSION MATCHES "18")
              set(WASMEDGE_OPENVINO_BIN_SUFFIX "${OPENVINO_VERSION}/linux/l_openvino_toolkit_ubuntu18_2022.2.0.7713.af16ea1d79a_x86_64.tgz")
              set(WASMEDGE_OPENVINO_HASH "c82f7d170e20b0816246bc3684005bf8ed014c07621341052d9cf5d58f841b22")
            elseif(OS_VERSION MATCHES "20")
              set(WASMEDGE_OPENVINO_BIN_SUFFIX "${OPENVINO_VERSION}/linux/l_openvino_toolkit_ubuntu20_2022.2.0.7713.af16ea1d79a_x86_64.tgz")
              set(WASMEDGE_OPENVINO_HASH "0d1b985aa46d65da7392f86ddb88f87be7627c04c6f8ba996aa2d14b39ab67e5")
            endif()
        else()
          message(WARNING "No suitable OpenVINO version was found, try build and compile OpenVINO yourself")
        endif()
      elseif(${OS_PLATFORM} MATCHES "CentOS")
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "AMD64")
          set(WASMEDGE_OPENVINO_BIN_SUFFIX "2021.4/l_openvino_toolkit_runtime_centos7_p_2021.4.582.tgz")
          set(WASMEDGE_OPENVINO_HASH "0b13a0eb5cea61bfab3b918ffcda23d914694918555dc23f6f134cd2cccf5eec")
          set(OpenVINO_2021 TRUE)
        else()
          message(WARNING "No suitable OpenVINO version was found, try build and compile OpenVINO yourself")
        endif()
      elseif(${OS_PLATFORM} MATCHES "Red")
        set(WASMEDGE_OPENVINO_BIN_SUFFIX "${OPENVINO_VERSION}/linux/l_openvino_toolkit_rhel8_2022.2.0.7713.af16ea1d79a_x86_64.tgz")
        set(WASMEDGE_OPENVINO_HASH "73bb867d9b3f514fafa67231a6f5e16aa6e450bd98571aa00f33cd5c2af9716a")
      elseif(${OS_PLATFORM} MATCHES "Debian")
        if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
          set(WASMEDGE_OPENVINO_BIN_SUFFIX "${OPENVINO_VERSION}/linux/l_openvino_toolkit_debian9_arm_2022.2.0.7713.af16ea1d79a_armhf.tgz")
          set(WASMEDGE_OPENVINO_HASH "f0e591667745c35d4c8dd9facccf1e7cf65fe2425069a9584dff4e89e032cb67")
        endif()
      endif()
      
    elseif(WIN32)
      set(WASMEDGE_OPENVINO_BIN_SUFFIX "${OPENVINO_VERSION}/windows/w_openvino_toolkit_windows_2022.2.0.7713.af16ea1d79a_x86_64.zip")
      set(WASMEDGE_OPENVINO_HASH "450979453f254c630b459106d9682625be48a05a354760dfb86f993a9945a256")
    else()
      message(FATAL_ERROR "WASI-NN: backend ${BACKEND} not found or should be installed by yourself.")
    endif()
    
    # Fetch OpenVINO library.
    FetchContent_Declare(
      wasmopenvino
      URL "https://storage.openvinotoolkit.org/repositories/openvino/packages/${WASMEDGE_OPENVINO_BIN_SUFFIX}"
      URL_HASH "SHA256=${WASMEDGE_OPENVINO_HASH}"
    )
    FetchContent_GetProperties(wasmopenvino)
    
    if(NOT wasmopenvino_POPULATED)
      message(STATUS "Downloading : OpenVINO")
      FetchContent_Populate(wasmopenvino)
      message(STATUS "Downloading : OpenVINO - done")
    endif()
    

    message(STATUS "OpenVINO downloaded in ${wasmopenvino_SOURCE_DIR}")
    
    if(OpenVINO_2021)
      set(InferenceEngine_DIR ${wasmopenvino_SOURCE_DIR}/deployment_tools/inference_engine/share CACHE INTERNAL "OpenVINO Library location")
      find_package(InferenceEngine REQUIRED)
      target_link_libraries(wasmedgePluginWasiNN
        PUBLIC
        ${InferenceEngine_LIBRARIES}
      )
    else()
      set(OpenVINO_DIR ${wasmopenvino_SOURCE_DIR}/runtime/cmake CACHE INTERNAL "OpenVINO Library location")
      set(InferenceEngine_DIR ${wasmopenvino_SOURCE_DIR}/runtime/cmake CACHE INTERNAL "OpenVINO Library location")
      find_package(OpenVINO REQUIRED)
      target_link_libraries(wasmedgePluginWasiNN
        PUBLIC
        openvino::runtime::c
      )
    endif()
  elseif(BACKEND MATCHES "PyTorch")
    message(STATUS "Build PyTorch backend for WASI-NN")
    find_package(Torch REQUIRED)
    add_definitions(-DWASMEDGE_PLUGIN_WASI_NN_BACKEND_TORCH)
    target_link_libraries(wasmedgePluginWasiNN
      PUBLIC
      ${TORCH_LIBRARIES}
    )
  else()
    # Add the other backends here.
    message(FATAL_ERROR "WASI-NN backend ${BACKEND} not found or unimplemented.")
  endif()
endforeach()