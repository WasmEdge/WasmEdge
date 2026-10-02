# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeFFmpegTests
  main.cpp

  avcodec/FFmpegAVCodecCtxTest.cpp
  avcodec/FFmpegAVCodecFuncTest.cpp
  avcodec/FFmpegAVCodecParametersTest.cpp
  avcodec/FFmpegAVCodecTest.cpp
  avcodec/FFmpegAVPacketTest.cpp

  avfilter/FFmpegAVFilterFuncTest.cpp
  avfilter/FFmpegAVFilterTest.cpp

  avformat/FFmpegAVChapterTest.cpp
  avformat/FFmpegAVFormatContextTest.cpp
  avformat/FFmpegAVFormatFuncTest.cpp
  avformat/FFmpegAVInputOutputContextTest.cpp
  avformat/FFmpegAVStreamTest.cpp

  avutil/FFmpegAVDictionaryTest.cpp
  avutil/FFmpegAVErrorTest.cpp
  avutil/FFmpegAVFrameTest.cpp
  avutil/FFmpegAVPixFmtTest.cpp
  avutil/FFmpegAVRationalTest.cpp
  avutil/FFmpegAVSampleFmtTest.cpp
  avutil/FFmpegAVUtilFuncTest.cpp

  swresample/FFmpegSWResampleFuncTest.cpp

  swscale/FFmpegSWScaleFuncTest.cpp

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
