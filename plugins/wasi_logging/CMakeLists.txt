wasmedge_add_library(wasmedgePluginWasiLogging
  SHARED
  env.cpp
  func.cpp
  module.cpp
)

target_compile_options(wasmedgePluginWasiLogging
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasiLogging
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasiLogging
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasiLogging
    PRIVATE
    wasmedge_shared
  )
endif()

install(TARGETS wasmedgePluginWasiLogging DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
