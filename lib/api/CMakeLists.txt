# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

find_package(spdlog QUIET)
add_definitions(-DWASMEDGE_COMPILE_LIBRARY)
if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_BUILD_AOT_RUNTIME)
endif()

get_filename_component(CMAKE_AR_NAME "${CMAKE_AR}" NAME)

# Helper function to construct commands and dependencies.
function(wasmedge_add_static_lib_component_command target)
  if(APPLE)
    if(CMAKE_AR_NAME STREQUAL "ar")
      list(APPEND CMDS
        COMMAND ${CMAKE_COMMAND} -E make_directory objs/${target}
        COMMAND ${CMAKE_AR} -x $<TARGET_FILE:${target}>
        COMMAND ${CMAKE_AR} -t $<TARGET_FILE:${target}> | xargs -I '{}' mv '{}' objs/${target}
      )
      set(WASMEDGE_STATIC_LIB_AR_CMDS ${WASMEDGE_STATIC_LIB_AR_CMDS} ${CMDS} PARENT_SCOPE)
    elseif(CMAKE_AR_NAME STREQUAL "libtool")
      set(WASMEDGE_STATIC_LIB_LIBTOOL_FILES ${WASMEDGE_STATIC_LIB_LIBTOOL_FILES} $<TARGET_FILE:${target}> PARENT_SCOPE)
    endif()
  else()
    list(APPEND CMDS
      COMMAND ${CMAKE_COMMAND} -E make_directory objs/${target}
      COMMAND ${CMAKE_COMMAND} -E chdir objs/${target_name} ${CMAKE_AR} -x $<TARGET_FILE:${target}>
    )
    set(WASMEDGE_STATIC_LIB_AR_CMDS ${WASMEDGE_STATIC_LIB_AR_CMDS} ${CMDS} PARENT_SCOPE)
  endif()
  set(WASMEDGE_STATIC_LIB_DEPS ${WASMEDGE_STATIC_LIB_DEPS} ${target} PARENT_SCOPE)
endfunction()

# Helper function to construct commands about packaging llvm and dependency libraries with paths.
function(wasmedge_add_libs_component_command target_path)
  get_filename_component(target_name ${target_path} NAME)
  string(REGEX REPLACE "^lib" "" target_name ${target_name})
  string(REGEX REPLACE "\.a$" "" target_name ${target_name})
  if(APPLE)
    get_filename_component(CMAKE_AR_NAME "${CMAKE_AR}" NAME)
    if(CMAKE_AR_NAME STREQUAL "ar")
      list(APPEND CMDS
        COMMAND ${CMAKE_COMMAND} -E make_directory objs/${target_name}
        COMMAND ${CMAKE_AR} -x ${target_path}
        COMMAND ${CMAKE_AR} -t ${target_path} | xargs -I '{}' mv '{}' objs/${target_name}
      )
      set(WASMEDGE_STATIC_LLVM_LIB_AR_CMDS ${WASMEDGE_STATIC_LLVM_LIB_AR_CMDS} ${CMDS} PARENT_SCOPE)
    elseif(CMAKE_AR_NAME STREQUAL "libtool")
      set(WASMEDGE_STATIC_LIB_LIBTOOL_FILES ${WASMEDGE_STATIC_LIB_LIBTOOL_FILES} ${target_path} PARENT_SCOPE)
    endif()
  else()
    list(APPEND CMDS
      COMMAND ${CMAKE_COMMAND} -E make_directory objs/${target_name}
      COMMAND ${CMAKE_COMMAND} -E chdir objs/${target_name} ${CMAKE_AR} -x ${target_path}
    )
    set(WASMEDGE_STATIC_LLVM_LIB_AR_CMDS ${WASMEDGE_STATIC_LLVM_LIB_AR_CMDS} ${CMDS} PARENT_SCOPE)
  endif()
endfunction()

set(WASMEDGE_CAPI_HEADERS
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/api/wasmedge/wasmedge.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/version.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/enum.inc
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/enum_configure.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/enum_errcode.h
  ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge/enum_types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/../../include/api/wasmedge/int128.h
)

# Object library of the C API wrapper.
wasmedge_add_library(wasmedgeCAPI OBJECT
  wasmedge.cpp
)

target_link_libraries(wasmedgeCAPI
  PUBLIC
  wasmedgeVM
  wasmedgeDriver
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

  wasmedge_add_library(wasmedge_shared SHARED
    ../../include/api/wasmedge/wasmedge.h
  )

  set_target_properties(wasmedge_shared PROPERTIES
    PUBLIC_HEADER "${WASMEDGE_CAPI_HEADERS}"
    VERSION "${WASMEDGE_CAPI_VERSION}"
    SOVERSION "${WASMEDGE_CAPI_SOVERSION}"
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
    OUTPUT_NAME wasmedge
  )

  if(CMAKE_SYSTEM_NAME MATCHES "Linux|Android")
    set_target_properties(wasmedge_shared PROPERTIES
      LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/libwasmedge.lds"
    )
    target_link_options(wasmedge_shared
      PRIVATE
      "-Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/libwasmedge.lds"
    )
  endif()

  target_link_libraries(wasmedge_shared
    PRIVATE
    wasmedgeCAPI
  )
  target_link_libraries(wasmedge_shared
    PUBLIC
    spdlog::spdlog
  )

  target_include_directories(wasmedge_shared
    PUBLIC
    ${PROJECT_BINARY_DIR}/include/api
    ${PROJECT_SOURCE_DIR}/include/api
  )

  install(TARGETS wasmedge_shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/wasmedge
  )

endif()

if(WASMEDGE_BUILD_STATIC_LIB)

  wasmedge_add_static_lib_component_command(spdlog::spdlog)
  wasmedge_add_static_lib_component_command(wasmedgeSystem)
  wasmedge_add_static_lib_component_command(wasmedgeCommon)
  wasmedge_add_static_lib_component_command(wasmedgePO)
  wasmedge_add_static_lib_component_command(wasmedgeLoaderFileMgr)
  wasmedge_add_static_lib_component_command(wasmedgeLoader)
  wasmedge_add_static_lib_component_command(wasmedgeValidator)
  wasmedge_add_static_lib_component_command(wasmedgeExecutor)
  wasmedge_add_static_lib_component_command(wasmedgeHostModuleWasi)
  wasmedge_add_static_lib_component_command(wasmedgePlugin)
  wasmedge_add_static_lib_component_command(wasmedgeVM)
  wasmedge_add_static_lib_component_command(wasmedgeDriver)

  if(WASMEDGE_BUILD_AOT_RUNTIME)
    foreach(LIB_NAME IN LISTS WASMEDGE_LLVM_LINK_STATIC_COMPONENTS)
      wasmedge_add_libs_component_command(${LIB_NAME})
    endforeach()
    wasmedge_add_static_lib_component_command(utilBlake3)
    wasmedge_add_static_lib_component_command(wasmedgeAOT)
  endif()

  if(CMAKE_AR_NAME STREQUAL "libtool")
    add_custom_command(OUTPUT "libwasmedge.a"
      COMMAND ${CMAKE_AR} -static -o libwasmedge.a ${WASMEDGE_STATIC_LIB_LIBTOOL_FILES} $<TARGET_OBJECTS:wasmedgeCAPI>
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${WASMEDGE_STATIC_LIB_DEPS} wasmedgeCAPI
    )
  else()
    add_custom_command(OUTPUT "libwasmedge.a"
      ${WASMEDGE_STATIC_LIB_AR_CMDS}
      ${WASMEDGE_STATIC_LLVM_LIB_AR_CMDS}
      COMMAND ${CMAKE_AR} -qcs libwasmedge.a $<TARGET_OBJECTS:wasmedgeCAPI> objs/*/*.o
      COMMAND ${CMAKE_COMMAND} -E remove_directory objs
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS ${WASMEDGE_STATIC_LIB_DEPS} wasmedgeCAPI
    )
  endif()

  add_custom_target(wasmedge_static_target ALL DEPENDS "libwasmedge.a")
  add_library(wasmedge_static STATIC IMPORTED GLOBAL)
  add_dependencies(wasmedge_static wasmedge_static_target)

  set_target_properties(wasmedge_static
    PROPERTIES
    IMPORTED_LOCATION "${CMAKE_CURRENT_BINARY_DIR}/libwasmedge.a"
    INTERFACE_INCLUDE_DIRECTORIES ${PROJECT_BINARY_DIR}/include/api
  )

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libwasmedge.a
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(FILES ${WASMEDGE_CAPI_HEADERS}
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/wasmedge
  )

endif()
