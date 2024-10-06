# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

wasmedge_add_executable(wasmedgeFFmpegTests
  main.cpp

  avcodec/avcodec_func.cpp
  avcodec/avCodec.cpp
  avcodec/avCodecParameters.cpp
  avcodec/avPacket.cpp
  avcodec/avCodecCtx.cpp

  avfilter/avfilter_func.cpp
  avfilter/avfilter.cpp

  avformat/avformat_func.cpp
  avformat/avformatContext.cpp
  avformat/avInputOutputContext.cpp
  avformat/avStream.cpp
  avformat/avChapter.cpp

  avutil/avRational.cpp
  avutil/avDictionary.cpp
  avutil/avFrame.cpp
  avutil/avutil_func.cpp
  avutil/avError.cpp
  avutil/avSampleFmt.cpp
  avutil/avPixfmt.cpp

  swresample/swresample_func.cpp
  
  swscale/swscale_func.cpp

  utils.cpp
)

# Downloading a sample file
execute_process(
  COMMAND bash ${CMAKE_SOURCE_DIR}/utils/ffmpeg/download-ffmpeg-sample-video.sh ${CMAKE_CURRENT_BINARY_DIR}/ffmpeg-assets
  RESULT_VARIABLE DOWNLOAD_ERROR
  OUTPUT_STRIP_TRAILING_WHITESPACE
)

add_dependencies(wasmedgeFFmpegTests
  wasmedgePluginWasmEdgeFFmpeg
)

target_include_directories(wasmedgeFFmpegTests
  PUBLIC
  $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
  $<TARGET_PROPERTY:wasmedgePluginWasmEdgeFFmpeg,INCLUDE_DIRECTORIES>
  ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(wasmedgeFFmpegTests
  PRIVATE
  ${GTEST_BOTH_LIBRARIES}
)

# Link to the WasmEdge library
if(WASMEDGE_LINK_PLUGINS_STATIC)
  target_link_libraries(wasmedgeFFmpegTests
    PRIVATE
    wasmedgeCAPI
  )
else()
  target_link_libraries(wasmedgeFFmpegTests
    PRIVATE
    wasmedge_shared
  )
endif()

add_test(wasmedgeFFmpegTests wasmedgeFFmpegTests)
