# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2022 Second State INC

wasmedge_add_executable(wasmedgeFFmpegTests
        avcodec/avcodec_func.cpp
        avcodec.cpp

        avutil/avRational.cpp

        swscale/swscale_func.cpp

        swresample/swresample_func.cpp
)

add_dependencies(wasmedgeFFmpegTests
        wasmedgePluginWasmEdgeFFmpeg
)

target_include_directories(wasmedgeFFmpegTests
        PUBLIC
        $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:wasmedgePluginWasmEdgeFFmpeg,INCLUDE_DIRECTORIES>
)

target_link_libraries(wasmedgeFFmpegTests
        PRIVATE
        ${GTEST_BOTH_LIBRARIES}
)

# Link to the WasmEdge library
if (WASMEDGE_LINK_PLUGINS_STATIC)
    target_link_libraries(wasmedgeFFmpegTests
            PRIVATE
            wasmedgeCAPI
    )
else ()
    target_link_libraries(wasmedgeFFmpegTests
            PRIVATE
            wasmedge_shared
    )
endif ()

add_test(wasmedgeFFmpegTests wasmedgeFFmpegTests)
