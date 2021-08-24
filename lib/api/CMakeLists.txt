# SPDX-License-Identifier: Apache-2.0

add_definitions(-DWASMEDGE_COMPILE_LIBRARY)
if(WASMEDGE_BUILD_AOT_RUNTIME)
  add_definitions(-DWASMEDGE_BUILD_AOT_RUNTIME)
endif()

wasmedge_add_library(wasmedgeCAPI OBJECT
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

if(WASMEDGE_BUILD_SHARED_LIB)

  wasmedge_add_library(wasmedge_c_shared SHARED
    ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
  )

  set_target_properties(wasmedge_c_shared PROPERTIES
    PUBLIC_HEADER ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
    MSVC_RUNTIME_LIBRARY "MultiThreadedDLL"
  )

  target_link_libraries(wasmedge_c_shared
    PUBLIC
    wasmedgeCAPI
  )

  set_target_properties(wasmedge_c_shared PROPERTIES OUTPUT_NAME wasmedge_c)

  install(TARGETS wasmedge_c_shared
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

endif()

if(WASMEDGE_BUILD_STATIC_LIB)

  add_custom_target(wasmedge_c_static_runtime ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/spdlog
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:spdlog::spdlog> --output=objs/spdlog
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeSystem
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeSystem> --output=objs/wasmedgeSystem
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeCommon
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeCommon> --output=objs/wasmedgeCommon
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeLoaderFileMgr
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeLoaderFileMgr> --output=objs/wasmedgeLoaderFileMgr
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeAST
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeAST> --output=objs/wasmedgeAST
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeLoader
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeLoader> --output=objs/wasmedgeLoader
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeValidator
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeValidator> --output=objs/wasmedgeValidator
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeInterpreter
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeInterpreter> --output=objs/wasmedgeInterpreter
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeHostModuleWasi
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeHostModuleWasi> --output=objs/wasmedgeHostModuleWasi
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeHostModuleWasmEdgeProcess
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeHostModuleWasmEdgeProcess> --output=objs/wasmedgeHostModuleWasmEdgeProcess
    COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeVM
    COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeVM> --output=objs/wasmedgeVM
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    DEPENDS 
    spdlog::spdlog
    wasmedgeSystem
    wasmedgeCommon
    wasmedgeLoaderFileMgr
    wasmedgeAST
    wasmedgeLoader
    wasmedgeValidator
    wasmedgeInterpreter
    wasmedgeHostModuleWasi
    wasmedgeHostModuleWasmEdgeProcess
    wasmedgeVM
    wasmedgeCAPI
  )

  if (WASMEDGE_BUILD_AOT_RUNTIME)
    add_custom_target(wasmedge_c_static ALL
      COMMAND ${CMAKE_COMMAND} -E make_directory objs/utilBlake3
      COMMAND ${CMAKE_AR} -x $<TARGET_FILE:utilBlake3> --output=objs/utilBlake3
      COMMAND ${CMAKE_COMMAND} -E make_directory objs/wasmedgeAOT
      COMMAND ${CMAKE_AR} -x $<TARGET_FILE:wasmedgeAOT> --output=objs/wasmedgeAOT
      COMMAND ${CMAKE_AR} -qcs libwasmedge_c.a $<TARGET_OBJECTS:wasmedgeCAPI> objs/*/*.o
      COMMAND ${CMAKE_COMMAND} -E rm -rf objs
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS
      utilBlake3
      wasmedgeAOT
      wasmedge_c_static_runtime
    )
  else()
    add_custom_target(wasmedge_c_static ALL
      COMMAND ${CMAKE_AR} -qcs libwasmedge_c.a $<TARGET_OBJECTS:wasmedgeCAPI> objs/*/*.o
      COMMAND ${CMAKE_COMMAND} -E rm -rf objs
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      DEPENDS
      wasmedge_c_static_runtime
    )
  endif()

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/libwasmedge_c.a
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )
  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/../../include/api/wasmedge.h
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

endif()
