cmake_minimum_required(VERSION 3.16)
project(wasmedge_java C)

set(CMAKE_C_STANDARD 11)
find_package(JNI)

if (JNI_FOUND)
    message (STATUS "JNI_INCLUDE_DIRS=${JNI_INCLUDE_DIRS}")
    message (STATUS "JNI_LIBRARIES=${JNI_LIBRARIES}")
endif()

include_directories(${JNI_INCLUDE_DIRS})

add_library(wasmedge_java SHARED
        org_wasmedge_WasmEdgeVM.h
        WasmEdgeVM.c)
target_link_libraries(wasmedge_java wasmedge_c)

install(TARGETS wasmedge_java
        LIBRARY DESTINATION lib)
