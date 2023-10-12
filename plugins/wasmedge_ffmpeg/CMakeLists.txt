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


        avformat/module.cpp
        avformat/avformat_func.cpp
        avformat/avformatContext.cpp
        avformat/avio_func.cpp
        avformat/avInputOutputFormat.cpp
        avformat/avStream.cpp
        avformat/avChapter.cpp

        avutil/error.cpp
        avutil/module.cpp
        avutil/avRational.cpp
        avutil/avFrame.cpp
        avutil/pixfmt.cpp
        avutil/samplefmt.cpp
        avutil/channel_layout.cpp
        avutil/avDictionary.cpp

        swresample/module.cpp
        swresample/swresample_func.cpp

        swscale/module.cpp
        swscale/swscale_func.cpp

)

target_link_directories(wasmedgePluginWasmEdgeFFmpeg PUBLIC ${FFMPEG_LIB})
target_compile_options(wasmedgePluginWasmEdgeFFmpeg
        PUBLIC
        -DWASMEDGE_PLUGIN
)

if (CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_options(wasmedgePluginWasmEdgeFFmpeg
            PUBLIC
            -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
            -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
    )
endif ()

target_include_directories(wasmedgePluginWasmEdgeFFmpeg
        PUBLIC
        $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${FFMPEG_INCLUDE}
        "/usr/local/include" # Only for testing.
)

if (WASMEDGE_LINK_PLUGINS_STATIC)
    target_link_libraries(wasmedgePluginWasmEdgeFFmpeg
            PRIVATE
            wasmedgeCAPI
    )
else ()
    target_link_libraries(wasmedgePluginWasmEdgeFFmpeg
            PRIVATE
            wasmedge_shared
            avformat
            avcodec
            avdevice
            avfilter
            avutil
            swresample
            swscale
    )
endif ()

install(TARGETS wasmedgePluginWasmEdgeFFmpeg DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
