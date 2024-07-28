cmake_minimum_required(VERSION 3.16)
project(wasmedge_jni C)

set(CMAKE_C_STANDARD 11)
set(TARGET wasmedge_jni)

find_package(JNI REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(WasmEdge REQUIRED)
configure_file(wasmedge-version.properties.in ${CMAKE_CURRENT_BINARY_DIR}/wasmedge-version.properties)

include_directories(${JNI_INCLUDE_DIRS} ${WasmEdge_INCLUDE_DIRS})
include_directories(jni)
add_library(${TARGET} SHARED
        lib/WasmEdge.c
        lib/WasmEdgeVM.c
        lib/common.c
        lib/StoreContext.c
        lib/ConfigureContext.c
        lib/FunctionTypeContext.c
        lib/FunctionTypeContext.h
        lib/GlobalTypeContext.c
        lib/TableTypeContext.c
        lib/CompilerContext.c
        lib/LoaderContext.c
        lib/AstModuleContext.c
        lib/StatisticsContext.c
        lib/ExecutorContext.c
        lib/ValidatorContext.c
        lib/GlobalInstanceContext.c
        lib/ValueType.c
        lib/TableInstanceContext.c
        lib/TableTypeContext.h
        lib/MemoryInstanceContext.c
        lib/MemoryTypeContext.c
        lib/MemoryTypeContext.h
        lib/ExternRef.c
        lib/ExportTypeContext.c
        lib/ImportTypeContext.c
        lib/ModuleInstanceContext.c
        lib/FunctionTypeInstance.c
        lib/FunctionTypeInstance.h
        lib/GlobalInstanceContext.h
        lib/MemoryInstanceContext.h
        lib/TableInstanceContext.h
        lib/ModuleInstanceContext.h
        lib/Async.h
        lib/Async.c)

target_link_libraries(${TARGET} ${WasmEdge_LIBRARIES})
