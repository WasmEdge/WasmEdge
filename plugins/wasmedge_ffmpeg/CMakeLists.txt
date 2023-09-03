wasmedge_add_library(wasmedgePluginWasmEdgeFfmpeg
        SHARED
        ffmpeg_env.cpp
        avcodec/module.cpp
        avcodec/avcodec_base.h
        avformat/module.cpp
        avformat/avformat_base.h
)

target_link_directories(wasmedgePluginWasmEdgeFfmpeg PUBLIC ${FFMPEG_LIB})
target_compile_options(wasmedgePluginWasmEdgeFfmpeg
        PUBLIC
        -DWASMEDGE_PLUGIN
)

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    target_link_options(wasmedgePluginWasmEdgeFfmpeg
            PUBLIC
            -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterC1EPKNS0_6Plugin16PluginDescriptorE
            -Wl,-U,__ZN8WasmEdge6Plugin14PluginRegisterD1Ev
    )
endif()

target_include_directories(wasmedgePluginWasmEdgeFfmpeg
        PUBLIC
        $<TARGET_PROPERTY:wasmedgePlugin,INCLUDE_DIRECTORIES>
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${FFMPEG_INCLUDE}
        "/usr/local/include" # Only for testing.
)

if(WASMEDGE_LINK_PLUGINS_STATIC)
    target_link_libraries(wasmedgePluginWasmEdgeFfmpeg
            PRIVATE
            wasmedgeCAPI
    )
else()
    target_link_libraries(wasmedgePluginWasmEdgeFfmpeg
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
endif()

install(TARGETS wasmedgePluginWasmEdgeFfmpeg DESTINATION ${CMAKE_INSTALL_LIBDIR}/wasmedge)
