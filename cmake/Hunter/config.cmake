## Identify native architecture to reduce amount of targets to build
set(ARCHITECTURE "${CMAKE_SYSTEM_PROCESSOR}")

if(ARCHITECTURE MATCHES "^(aarch64.*|AARCH64.*|arm.*|ARM.*)")
    set(ARCHITECTURE AArch64)
elseif(ARCHITECTURE MATCHES "^(x86_64.*|AMD64.*|i386.*|i686.*)")
    set(ARCHITECTURE X86)
elseif(ARCHITECTURE MATCHES "^(riscv.*)")
    set(ARCHITECTURE RISCV)
else()
    message(WARNING "Unknown architecture: ${ARCHITECTURE}, using all architectures to build LLVM")
    set(ARCHITECTURE AArch64;X86;RISCV)
endif()

hunter_config(
    LLVM
    VERSION 17.0.6
    CMAKE_ARGS # inspired by https://github.com/WasmEdge/WasmEdge/blob/5e8556afa5a71f3d3ef9615334ecf1a9d4d0f1e8/utils/docker/Dockerfile.manylinux2014_x86_64#L57
        LLVM_ENABLE_PROJECTS=lld;clang
        LLVM_TARGETS_TO_BUILD=${ARCHITECTURE};BPF
)

hunter_config(
    fmt
    URL
        https://github.com/fmtlib/fmt/archive/refs/tags/10.2.1.tar.gz
    SHA1
        d223964b782d2562d6722ffe67027204c6035453
    CMAKE_ARGS
        CMAKE_POSITION_INDEPENDENT_CODE=TRUE
)

hunter_config(
    spdlog
    VERSION 1.12.0-p0
    CMAKE_ARGS
        SPDLOG_BUILD_PIC=ON
        SPDLOG_FMT_EXTERNAL=ON
)
