# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

add_definitions(-DWASMEDGE_COMPILE_LIBRARY)
if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_BUILD_AOT_RUNTIME)
endif()

# Helper function to construct commands and dependencies.
function(wasmedge_add_static_lib_component_command target)
  list(APPEND CMDS
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/${target}
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:${target}> --output=objs/${target}
  )
  set(WASMEDGE_STATIC_LIB_CMDS ${WASMEDGE_STATIC_LIB_CMDS} ${CMDS} PARENT_SCOPE)
  set(WASMEDGE_STATIC_LIB_DEPS ${WASMEDGE_STATIC_LIB_DEPS} ${target} PARENT_SCOPE)
endfunction()

# Helper function to construct commands about collecting libraries with paths.
function(wasmedge_add_libs_component_command target target_path)
  list(APPEND CMDS
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/${target}
    COMMAND ${CMAKE_AR} -x ${target_path}/lib${target}.a --output=objs/${target}
  )
  set(WASMEDGE_STATIC_LIB_CMDS ${WASMEDGE_STATIC_LIB_CMDS} ${CMDS} PARENT_SCOPE)
endfunction()

set(WASMEDGE_CAPI_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/api/wasmedge/wasmedge.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/version.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/enum.inc
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/enum_configure.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/enum_errcode.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/enum_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/api/wasmedge/int128.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/dense_enum_map.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/spare_enum_map.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/wasm/wasm.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/wasm/wasm.hh
)

# Object library of the C API wrapper.
wasmedge_add_library(wasmedgeCAPI OBJECT
  wasm_c.cpp
  wasmedge.cpp
)

target_link_libraries(wasmedgeCAPI
  PUBLIC
  wasmedgeVM
)

if (WASMEDGE_BUILD_AOT_RUNTIME)
  target_link_libraries(wasmedgeCAPI
    PUBLIC
    wasmedgeAOT
  )
endif()

target_include_directories(wasmedgeCAPI
  PUBLIC
  ${PROJECT_BINARY_DIR}/include/api
  ${PROJECT_SOURCE_DIR}/include/api
)

if(WASMEDGE_BUILD_SHARED_LIB)

  wasmedge_add_library(wasmedge_c_shared SHARED
    ../../include/api/wasmedge/wasmedge.h
  )

  set_target_properties(wasmedge_c_shared PROPERTIES
    PUBLIC_HEADER "${WASMEDGE_CAPI_HEADERS}"
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
  )

  target_link_libraries(wasmedge_c_shared
    PUBLIC
    wasmedgeCAPI
  )

  set_target_properties(wasmedge_c_shared PROPERTIES OUTPUT_NAME wasmedge_c)

  install(TARGETS wasmedge_c_shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/wasmedge
  )

endif()

if(WASMEDGE_BUILD_STATIC_LIB)

  wasmedge_add_static_lib_component_command(spdlog::spdlog)
  wasmedge_add_static_lib_component_command(wasmedgeSystem)
  wasmedge_add_static_lib_component_command(wasmedgeCommon)
  wasmedge_add_static_lib_component_command(wasmedgeLoaderFileMgr)
  wasmedge_add_static_lib_component_command(wasmedgeLoader)
  wasmedge_add_static_lib_component_command(wasmedgeValidator)
  wasmedge_add_static_lib_component_command(wasmedgeExecutor)
  wasmedge_add_static_lib_component_command(wasmedgeHostModuleWasi)
  wasmedge_add_static_lib_component_command(wasmedgeHostModuleWasmEdgeProcess)
  wasmedge_add_static_lib_component_command(wasmedgeVM)

  if (WASMEDGE_BUILD_AOT_RUNTIME)
    # Pack the LLVM and lld static libraries.
    find_package(LLVM REQUIRED HINTS "${LLVM_CMAKE_PATH}")
    execute_process(
      COMMAND ${LLVM_BINARY_DIR}/bin/llvm-config --libs --link-static
      core lto native nativecodegen option passes support transformutils all-targets
      OUTPUT_VARIABLE WASMEDGE_LLVM_LINK_LIBS
    )
    string(REPLACE "-l" "" WASMEDGE_LLVM_LINK_LIBS ${WASMEDGE_LLVM_LINK_LIBS})
    string(REGEX REPLACE "[\r\n]" "" WASMEDGE_LLVM_LINK_LIBS ${WASMEDGE_LLVM_LINK_LIBS})
    string(REPLACE " " "\;" WASMEDGE_LLVM_LINK_LIBS ${WASMEDGE_LLVM_LINK_LIBS})
    set(WASMEDGE_LLVM_LINK_LIBS ${WASMEDGE_LLVM_LINK_LIBS})
    foreach(LIB_NAME IN LISTS WASMEDGE_LLVM_LINK_LIBS)
      wasmedge_add_libs_component_command(${LIB_NAME} ${LLVM_LIBRARY_DIR})
    endforeach()
    wasmedge_add_libs_component_command("lldELF" ${LLVM_LIBRARY_DIR})
    wasmedge_add_libs_component_command("lldCommon" ${LLVM_LIBRARY_DIR})
    wasmedge_add_libs_component_command("lldCore" ${LLVM_LIBRARY_DIR})
    wasmedge_add_libs_component_command("lldDriver" ${LLVM_LIBRARY_DIR})
    wasmedge_add_libs_component_command("lldReaderWriter" ${LLVM_LIBRARY_DIR})
    wasmedge_add_libs_component_command("lldYAML" ${LLVM_LIBRARY_DIR})

    # Pack the zlib.
    get_filename_component(ZLIB_PATH "${ZLIB_LIBRARIES}" DIRECTORY)
    wasmedge_add_libs_component_command("z" ${ZLIB_PATH})

    # Pack the tinfo.
    wasmedge_add_libs_component_command("tinfo" ${ZLIB_PATH})

    wasmedge_add_static_lib_component_command(utilBlake3)
    wasmedge_add_static_lib_component_command(wasmedgeAOT)
  endif()

  add_custom_target(wasmedge_c_static ALL
    ${WASMEDGE_STATIC_LIB_CMDS}
    COMMAND ${CMAKE_AR} -qcs libwasmedge_c.a $<TARGET_OBJECTS:wasmedgeCAPI> objs/*/*.o
    COMMAND ${CMAKE_COMMAND} -E rm -rf objs
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS
    ${WASMEDGE_STATIC_LIB_DEPS}
    wasmedgeCAPI
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libwasmedge_c.a
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(FILES ${WASMEDGE_CAPI_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/wasmedge
  )
  install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../../thirdparty/wasm/wasm.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

endif()
