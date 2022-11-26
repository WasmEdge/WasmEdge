# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(OpenCV REQUIRED)

wasmedge_add_library(wasmedgePluginOpenCV 
	SHARED
	opencvenv.cpp
)

target_compile_options(wasmedgePluginOpenCV
	PUBLIC
	-DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
  target_link_options(wasmedgePluginOpenCV
    PUBLIC
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
    -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
  )
endif()

option(WASMEDGE_OPENCV_CORE "Link OpenCV core module" ON)

if(WASMEDGE_OPENCV_CORE)
	add_subdirectory(core)
	list(APPEND EXTRA_LIBS core)
	list(APPEND EXTRA_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}/core")
endif()

target_include_directories(wasmedgePluginOpenCV
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginOpenCV
  PUBLIC
  ${EXTRA_LIBS}
  ${OpenCV_LIBS}
)

if(WASMEDGE_LINK_PUGLINS_STATIC)
  target_link_libraries(wasmedgePluginOpenCV
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginOpenCV
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginOpenCV DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
