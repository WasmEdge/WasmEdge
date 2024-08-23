# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_library(wasmedgePluginWasmEdgeImage
  SHARED
  image_env.cpp
  image_func.cpp
  image_module.cpp
)

target_compile_options(wasmedgePluginWasmEdgeImage
  PUBLIC
  -DWASMEDGE_PLUGIN
)

target_include_directories(wasmedgePluginWasmEdgeImage
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Find the libjpeg and libpng.
add_library(wasmedgePluginWasmEdgeImageJPEG STATIC IMPORTED GLOBAL)
add_library(wasmedgePluginWasmEdgeImagePNG STATIC IMPORTED GLOBAL)
if(APPLE)
  # For MacOS, use the installed libjpeg, libpng, and zlib static library.
  find_package(JPEG REQUIRED)
  find_package(PNG REQUIRED)
  # The find_package will get the shared library. Therefore find the static one.
  find_library(JPEG_STATIC NAMES libjpeg.a)
  find_library(PNG_STATIC NAMES libpng16.a)
  set_target_properties(wasmedgePluginWasmEdgeImageJPEG
    PROPERTIES
    IMPORTED_LOCATION ${JPEG_STATIC}
    INTERFACE_INCLUDE_DIRECTORIES "${JPEG_INCLUDE_DIR}"
  )
  set_target_properties(wasmedgePluginWasmEdgeImagePNG
    PROPERTIES
    IMPORTED_LOCATION ${PNG_STATIC}
    INTERFACE_INCLUDE_DIRECTORIES "${PNG_INCLUDE_DIR}"
  )
elseif(UNIX)
  # Fetch and build libjpeg and libpng.
  include(FetchContent)
  FetchContent_Declare(
    wasmedge_image_libpng
    URL "https://downloads.sourceforge.net/libpng/libpng-1.6.39.tar.gz"
    URL_HASH "SHA256=af4fb7f260f839919e5958e5ab01a275d4fe436d45442a36ee62f73e5beb75ba"
  )
  FetchContent_GetProperties(wasmedge_image_libpng)
  if(NOT wasmedge_image_libpng_POPULATED)
    message(STATUS "Downloading libpng source")
    FetchContent_Populate(wasmedge_image_libpng)
    message(STATUS "Downloading libpng source - done")
  endif()

  FetchContent_Declare(
    wasmedge_image_libjpeg
    URL "http://ijg.org/files/jpegsrc.v9e.tar.gz"
    URL_HASH "SHA256=4077d6a6a75aeb01884f708919d25934c93305e49f7e3f36db9129320e6f4f3d"
  )
  FetchContent_GetProperties(wasmedge_image_libjpeg)
  if(NOT wasmedge_image_libjpeg_POPULATED)
    message(STATUS "Downloading libjpeg source")
    FetchContent_Populate(wasmedge_image_libjpeg)
    message(STATUS "Downloading libjpeg source - done")
  endif()

  add_custom_command(
    OUTPUT ${wasmedge_image_libjpeg_SOURCE_DIR}/.libs/libjpeg.a
    COMMAND ${CMAKE_COMMAND} -E env CFLAGS=-fPIC ./configure --enable-shared=off
    COMMAND make
    WORKING_DIRECTORY ${wasmedge_image_libjpeg_SOURCE_DIR}
  )
  add_custom_command(
    OUTPUT ${wasmedge_image_libpng_SOURCE_DIR}/.libs/libpng16.a
    COMMAND ${CMAKE_COMMAND} -E env CFLAGS=-fPIC ./configure --enable-shared=off
    COMMAND make
    WORKING_DIRECTORY ${wasmedge_image_libpng_SOURCE_DIR}
  )
  add_custom_target(wasmedgePluginWasmEdgeImageJPEG_target
      ALL DEPENDS
      ${wasmedge_image_libjpeg_SOURCE_DIR}/.libs/libjpeg.a
  )
  add_custom_target(wasmedgePluginWasmEdgeImagePNG_target
      ALL DEPENDS
      ${wasmedge_image_libpng_SOURCE_DIR}/.libs/libpng16.a
  )
  add_dependencies(wasmedgePluginWasmEdgeImageJPEG wasmedgePluginWasmEdgeImageJPEG_target)
  add_dependencies(wasmedgePluginWasmEdgeImagePNG wasmedgePluginWasmEdgeImagePNG_target)
  set_target_properties(wasmedgePluginWasmEdgeImageJPEG
    PROPERTIES
    IMPORTED_LOCATION ${wasmedge_image_libjpeg_SOURCE_DIR}/.libs/libjpeg.a
    INTERFACE_INCLUDE_DIRECTORIES ${wasmedge_image_libjpeg_SOURCE_DIR}
  )
  set_target_properties(wasmedgePluginWasmEdgeImagePNG
    PROPERTIES
    IMPORTED_LOCATION ${wasmedge_image_libpng_SOURCE_DIR}/.libs/libpng16.a
    INTERFACE_INCLUDE_DIRECTORIES ${wasmedge_image_libpng_SOURCE_DIR}
  )
endif()

# Need zlib and boost.
find_package(ZLIB REQUIRED)
find_package(Boost 1.74.0)
if(${Boost_FOUND})
else()
  include(FetchContent)
  FetchContent_Declare(
    Boost
    URL http://sources.buildroot.net/boost/boost_1_82_0.tar.bz2
    URL_HASH SHA256=a6e1ab9b0860e6a2881dd7b21fe9f737a095e5f33a3a874afc6a345228597ee6
  )
  set(BOOST_ENABLE_CMAKE ON)
  set(BOOST_RUNTIME_LINK static)
  message(STATUS "Downloading boost 1.82.0 source")
  FetchContent_MakeAvailable(Boost)
  message(STATUS "Downloading boost 1.82.0 source - done")
  add_library(Boost_boost INTERFACE)
  add_library(Boost::boost ALIAS Boost_boost)
  target_include_directories(Boost_boost SYSTEM INTERFACE ${boost_SOURCE_DIR})
endif()

target_link_libraries(wasmedgePluginWasmEdgeImage
  PUBLIC
  Boost::boost
  wasmedgePluginWasmEdgeImageJPEG
  wasmedgePluginWasmEdgeImagePNG
  z
)
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeImage
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeImage
    PRIVATE
    wasmedge_shared
  )
endif()

install(
  TARGETS wasmedgePluginWasmEdgeImage
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
