# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  ffmpeg_env.cpp

  avcodec/module.cpp
  avcodec/avcodec_func.cpp
  avcodec/avCodecContext.cpp
  avcodec/avCodec.cpp
  avcodec/avCodecParameters.cpp
  avcodec/avPacket.cpp

  avdevice/module.cpp
  avdevice/avDevice_func.cpp

  avfilter/module.cpp
  avfilter/avfilter_func.cpp
  avfilter/buffer_source_sink.cpp
  avfilter/avFilter.cpp

  avformat/module.cpp
  avformat/avformat_func.cpp
  avformat/avformatContext.cpp
  avformat/avInputOutputFormat.cpp
  avformat/avStream.cpp
  avformat/avChapter.cpp

  avutil/module.cpp
  avutil/avutil_func.cpp
  avutil/error.cpp
  avutil/avRational.cpp
  avutil/avFrame.cpp
  avutil/pixfmt.cpp
  avutil/samplefmt.cpp
  avutil/avDictionary.cpp
  avutil/avTime.cpp

  swresample/module.cpp
  swresample/swresample_func.cpp

  swscale/module.cpp
  swscale/swscale_func.cpp

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

install(TARGETS wasmedgePluginWasmEdgeFFmpeg DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
