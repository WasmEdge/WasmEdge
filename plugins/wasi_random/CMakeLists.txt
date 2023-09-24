wasmedge_add_library(wasmedgePluginWasiRandom
  SHARED
  env.cpp
  func.cpp
  module.cpp
  details/random_linux.cpp
  details/random_macos.cpp
  details/random_windows.cpp
)

target_compile_options(wasmedgePluginWasiRandom
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasiRandom
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasiRandom
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasiRandom
    PRIVATE
    wasmedge_shared
  )
endif()

if(WIN32)
  target_link_libraries(wasmedgePluginWasiRandom
  PRIVATE
  bcrypt
)
endif()

install(TARGETS wasmedgePluginWasiRandom DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
