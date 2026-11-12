include(FetchContent)

# Download wasm-bpf, copy & compile two bpf programs from that
FetchContent_Declare(
    wasm_bpf
    GIT_REPOSITORY https://github.com/eunomia-bpf/wasm-bpf
    GIT_TAG b76be32d44c2ec1933ca28eab875b50e713855b8
)

message("Downloading wasm-bpf")
FetchContent_MakeAvailable(wasm_bpf)
message("Downloaded wasm-bpf")

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/bootstrap.bpf.o
    COMMAND make -C ${wasm_bpf_SOURCE_DIR}/examples/bootstrap/ bootstrap.bpf.o
    COMMAND cp ${wasm_bpf_SOURCE_DIR}/examples/bootstrap/bootstrap.bpf.o ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${wasm_bpf_SOURCE_DIR}/examples/bootstrap/
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/runqlat.bpf.o
    COMMAND make -C ${wasm_bpf_SOURCE_DIR}/examples/runqlat/ runqlat.bpf.o
    COMMAND cp ${wasm_bpf_SOURCE_DIR}/examples/runqlat/runqlat.bpf.o ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${wasm_bpf_SOURCE_DIR}/examples/runqlat/
)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/simple_map.bpf.o ${CMAKE_CURRENT_SOURCE_DIR}/simple_ringbuf.bpf.o
    COMMAND make -C ${CMAKE_CURRENT_SOURCE_DIR}/bpf-sources
    COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/bpf-sources/*.bpf.o ${CMAKE_CURRENT_SOURCE_DIR}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_custom_target(
    wasmBpfTestsAssets
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/bootstrap.bpf.o
    ${CMAKE_CURRENT_SOURCE_DIR}/runqlat.bpf.o
    ${CMAKE_CURRENT_SOURCE_DIR}/simple_ringbuf.bpf.o
    ${CMAKE_CURRENT_SOURCE_DIR}/simple_map.bpf.o
)
