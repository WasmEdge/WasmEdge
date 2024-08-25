# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBAV REQUIRED IMPORTED_TARGET
  libavdevice
  libavfilter
  libavformat
  libavcodec
  libswresample
  libswscale
  libavutil
)

wasmedge_add_library(wasmedgePluginWasmEdgeFFmpeg
  SHARED
  
  avcodec/avCodecContext.cpp
  avcodec/avCodec.cpp
  avcodec/avCodecParameters.cpp
  avcodec/avPacket.cpp
  avcodec/avcodec_func.cpp
  avcodec/module.cpp

  avdevice/avDevice_func.cpp
  avdevice/module.cpp

  avfilter/buffer_source_sink.cpp
  avfilter/avFilter.cpp
  avfilter/avfilter_func.cpp
  avfilter/module.cpp

  avformat/avformatContext.cpp
  avformat/avInputOutputFormat.cpp
  avformat/avStream.cpp
  avformat/avChapter.cpp
  avformat/avformat_func.cpp
  avformat/module.cpp

  avutil/error.cpp
  avutil/avRational.cpp
  avutil/avFrame.cpp
  avutil/pixfmt.cpp
  avutil/samplefmt.cpp
  avutil/avDictionary.cpp
  avutil/avTime.cpp
  avutil/avutil_func.cpp
  avutil/module.cpp

  swresample/swresample_func.cpp
  swresample/module.cpp

  swscale/swscale_func.cpp
  swscale/module.cpp
  
  ffmpeg_env.cpp
)

target_compile_options(wasmedgePluginWasmEdgeFFmpeg
  PUBLIC
  -DWASMEDGE_PLUGIN
  -Wno-deprecated-declarations
)

target_include_directories(wasmedgePluginWasmEdgeFFmpeg
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgePluginWasmEdgeFFmpeg
  PUBLIC
  PkgConfig::LIBAV
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgePluginWasmEdgeFFmpeg
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgePluginWasmEdgeFFmpeg
    PRIVATE
    wasmedge_shared
  )
endif()

install(
  TARGETS wasmedgePluginWasmEdgeFFmpeg
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge
)
