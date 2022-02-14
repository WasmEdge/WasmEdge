set(WasmEdge_CURRENT_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(WasmEdge_CURRENT_DIR)

add_compile_options(-O2 -g)
add_subdirectory(${WasmEdge_CURRENT_DIR} wasmedge)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(WasmEdge DEFAULT_MSG WasmEdge_CURRENT_DIR)
