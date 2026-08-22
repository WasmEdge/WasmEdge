include("${WASMEDGE_SOURCE_DIR}/cmake/LLVMRelocationBackends.cmake")

function(check_processor Processor ExpectedBackend)
  wasmedge_llvm_linker_backend_for_processor(Backend "${Processor}")
  if(NOT Backend STREQUAL ExpectedBackend)
    message(FATAL_ERROR "${Processor} selected '${Backend}', expected '${ExpectedBackend}'")
  endif()
endfunction()

foreach(Processor IN ITEMS
    x86_64 x86_64h amd64 x86_64-linux-gnu amd64-unknown-freebsd)
  check_processor("${Processor}" X86_64)
endforeach()
foreach(Processor IN ITEMS
    aarch64 arm64 aarch64-linux-gnu arm64-apple-darwin)
  check_processor("${Processor}" AARCH64)
endforeach()
foreach(Processor IN ITEMS
    arm armv7 armv7-a armv7a armv7l armv8 armv8-a armv8a armv8l
    arm-linux-gnueabihf armv7-linux-gnueabihf armv7a-linux-gnueabihf)
  check_processor("${Processor}" ARM)
endforeach()
foreach(Processor IN ITEMS riscv64 riscv64gc riscv64-unknown-linux-gnu)
  check_processor("${Processor}" RISCV64)
endforeach()
foreach(Processor IN ITEMS
    s390x systemz s390x-linux-gnu systemz-unknown-linux-gnu)
  check_processor("${Processor}" S390X)
endforeach()
foreach(Processor IN ITEMS
    aarch64_be armv64 armv7garbage riscv64foo unknown "")
  check_processor("${Processor}" "")
endforeach()

wasmedge_llvm_linker_backends_for_processors(
  Backends "x86_64;arm64;x86_64"
)
if(NOT Backends STREQUAL "X86_64;AARCH64")
  message(FATAL_ERROR "universal architectures selected '${Backends}'")
endif()

wasmedge_llvm_linker_backends_for_processors(
  Backends "x86_64;arm64e" Unmatched
)
if(NOT Backends STREQUAL "X86_64" OR NOT Unmatched STREQUAL "arm64e")
  message(FATAL_ERROR
    "mixed architectures selected '${Backends}', unmatched '${Unmatched}'")
endif()
