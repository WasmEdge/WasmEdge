cmake_minimum_required(VERSION 3.16)
project(wasmedge_jni C)

set(CMAKE_C_STANDARD 11)
set(TARGET wasmedge_jni)

find_package(JNI REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(WasmEdge REQUIRED)

include_directories(${JNI_INCLUDE_DIRS} ${WASMEDGE_INCLUDE_DIRS})

add_library(${TARGET} SHARED
        jni/org_wasmedge_WasmEdge.h
        lib/WasmEdge.c
        lib/WasmEdgeVM.c
        lib/common.c
        )
target_link_libraries(${TARGET} wasmedge_c)

install(TARGETS ${TARGET}
        LIBRARY DESTINATION lib)
