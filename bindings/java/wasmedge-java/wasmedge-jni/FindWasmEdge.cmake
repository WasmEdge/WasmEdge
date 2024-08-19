# Find WasmEdge Headers/Libs
# Variables
# WasmEdge_ROOT -set this to location where WasmEdge may be found
# WasmEdge_FOUND - True if WasmEdge found
# WasmEdge_INCLUDE_DIRS - Location of WasmEdge includes
# WasmEdge_LIBRARIES - WasmEdge libraries

if(NOT WasmEdge_ROOT)
    set(WasmEdge_ROOT "$ENV{WasmEdge_ROOT}")
endif()

if(NOT WasmEdge_ROOT)
    find_path(_WasmEdge_ROOT NAMES include/wasmedge.h)
else()
    set(_WasmEdge_ROOT "${WasmEdge_ROOT}")
endif()

message(STATUS "wasmedge root: ${_WasmEdge_ROOT}")
find_path(WasmEdge_INCLUDE_DIRS NAMES wasmedge/wasmedge.h HINTS ${_WasmEdge_ROOT}/include ${_WasmEdge_ROOT}/include/api /usr/local/include /usr/include)
message(STATUS "wasmedge include dirs: ${WasmEdge_INCLUDE_DIRS}")

if(WasmEdge_INCLUDE_DIRS)
    set(_WasmEdge_H ${WasmEdge_INCLUDE_DIRS}/wasmedge/version.h)

    function(_wasmedgever_EXTRACT _WasmEdge_VER_COMPONENT _WasmEdge_VER_OUTPUT)
        set(CMAKE_MATCH_1 "0")
        set(_WasmEdge_expr "^[ \\t]*#define[ \\t]+${_WasmEdge_VER_COMPONENT}[ \\t]+([0-9]+)$")
        file(STRINGS "${_WasmEdge_H}" _WasmEdge_ver REGEX "${_WasmEdge_expr}")
        string(REGEX MATCH "${_WasmEdge_expr}" WasmEdge_ver "${_WasmEdge_ver}")
        set(${_WasmEdge_VER_OUTPUT} "${CMAKE_MATCH_1}" PARENT_SCOPE)
    endfunction()

    _wasmedgever_EXTRACT("WASMEDGE_VERSION_MAJOR" WasmEdge_VERSION_MAJOR)
    _wasmedgever_EXTRACT("WASMEDGE_VERSION_MINOR" WasmEdge_VERSION_MINOR)
    _wasmedgever_EXTRACT("WASMEDGE_VERSION_PATCH" WasmEdge_VERSION_PATCH)

    # We should provide version to find_package_handle_standard_args in the same format as it was requested,
    # otherwise it can't check whether version matches exactly.
    if(WasmEdge_FIND_VERSION_COUNT GREATER 2)
        set(WasmEdge_VERSION "${WasmEdge_VERSION_MAJOR}.${WasmEdge_VERSION_MINOR}.${WasmEdge_VERSION_PATCH}")
    else()
        # User has requested WasmEdge version without patch part => user is not interested in specific patch =>
        # any patch should be an exact match.
        set(WasmEdge_VERSION "${WasmEdge_VERSION_MAJOR}.${WasmEdge_VERSION_MINOR}")
    endif()

   find_library(WasmEdge_LIBRARIES
       NAMES
       wasmedge
       HINTS
       ${_WasmEdge_ROOT}/lib
       ${_WasmEdge_ROOT}/lib/x86_64-linux-gnu
       ${WasmEdge_ROOT}/lib/api/
       /usr/lib
       /usr/local/lib
       )
endif()

message(STATUS "wasmedge lib: ${WasmEdge_LIBRARIES}")

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(WasmEdge
        FOUND_VAR
        WasmEdge_FOUND
        REQUIRED_VARS
        WasmEdge_INCLUDE_DIRS
        WasmEdge_LIBRARIES
        VERSION_VAR
        WasmEdge_VERSION
        )
