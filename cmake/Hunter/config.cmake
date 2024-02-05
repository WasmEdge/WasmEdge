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
    CMAKE_ARGS
        LLVM_ENABLE_PROJECTS=lld;clang
        LLVM_TARGETS_TO_BUILD=${ARCHITECTURE};BPF
)
