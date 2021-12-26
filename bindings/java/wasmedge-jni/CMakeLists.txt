cmake_minimum_required(VERSION 3.16)
project(wasmedge_jni C)

set(CMAKE_C_STANDARD 11)
set(TARGET wasmedge_jni)

find_package(JNI REQUIRED)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(WasmEdge REQUIRED)


include_directories(${JNI_INCLUDE_DIRS} ${WasmEdge_INCLUDE_DIRS})

add_library(${TARGET} SHARED
        lib/WasmEdge.c
        lib/WasmEdgeVM.c
        lib/common.c
        lib/StoreContext.c
        lib/ConfigureContext.c
        lib/FunctionTypeContext.c lib/FunctionTypeContext.h lib/GlobalTypeContext.c lib/TableTypeContext.c lib/CompilerContext.c lib/LoaderContext.c lib/AstModuleContext.c lib/AstModuleContext.h)
target_link_libraries(${TARGET} wasmedge_c)

install(TARGETS ${TARGET}
        LIBRARY DESTINATION lib)
