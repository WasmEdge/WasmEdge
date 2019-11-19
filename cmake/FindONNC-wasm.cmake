find_library(ONNC_wasm_lib NAMES onnc-wasm
    PATH_SUFFIXES lib
    HINTS ${ONNC_WASM_ROOT}
)

set(ONNC_WASM_LIBRARY ${ONNC_wasm_lib})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ONNC-wasm DEFAULT_MSG
    ONNC_WASM_LIBRARY
)