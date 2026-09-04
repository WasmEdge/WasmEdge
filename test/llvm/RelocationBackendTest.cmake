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

function(check_selection Name ExpectedBackends ExpectedError)
  wasmedge_select_llvm_linker_backends(Backends Error ${ARGN})
  if(NOT Backends STREQUAL ExpectedBackends)
    message(FATAL_ERROR
      "${Name} selected '${Backends}', expected '${ExpectedBackends}'")
  endif()
  if(NOT Error STREQUAL ExpectedError)
    message(FATAL_ERROR
      "${Name} returned error '${Error}', expected '${ExpectedError}'")
  endif()
endfunction()

function(check_requested_selection Name ExpectedBackends ExpectedError Requested)
  wasmedge_select_llvm_linker_backends(
    Backends Error ${ARGN} REQUESTED "${Requested}")
  if(NOT Backends STREQUAL ExpectedBackends)
    message(FATAL_ERROR
      "${Name} selected '${Backends}', expected '${ExpectedBackends}'")
  endif()
  if(NOT Error STREQUAL ExpectedError)
    message(FATAL_ERROR
      "${Name} returned error '${Error}', expected '${ExpectedError}'")
  endif()
endfunction()

check_selection(host-x86 X86_64 ""
  PROCESSOR x86_64 REQUESTED AUTO)
check_selection(default-request X86_64 ""
  USE_LLVM PROCESSOR x86_64)
check_selection(all-test "X86_64;AARCH64;ARM;RISCV64;S390X" ""
  BUILD_TESTS PROCESSOR unknown REQUESTED AUTO)
check_selection(universal-apple "X86_64;AARCH64" ""
  ARCHITECTURES x86_64 arm64 REQUESTED AUTO)
check_selection(explicit-unknown ""
  "Unknown WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS value: unknown"
  REQUESTED unknown)
check_requested_selection(explicit-normalization "X86_64;AARCH64" ""
  "x86_64;aarch64" PROCESSOR unknown)
check_requested_selection(explicit-keyword ""
  "Unknown WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS value: BUILD_TESTS"
  BUILD_TESTS)
check_requested_selection(explicit-empty "" "" ""
  USE_LLVM PROCESSOR mystery)
check_requested_selection(explicit-duplicate ""
  "Duplicate WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS value: X86_64"
  "x86_64;X86_64")
check_selection(unknown-host-llvm ""
  "AUTO could not select an LLVM linker relocation backend: CMAKE_SYSTEM_PROCESSOR='mystery', CMAKE_OSX_ARCHITECTURES='', unmatched architectures=''. Set WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS explicitly."
  USE_LLVM PROCESSOR mystery REQUESTED AUTO)
check_selection(unknown-host-no-llvm "" ""
  PROCESSOR mystery REQUESTED AUTO)
check_selection(mixed-architectures X86_64
  "AUTO could not select an LLVM linker relocation backend: CMAKE_SYSTEM_PROCESSOR='', CMAKE_OSX_ARCHITECTURES='x86_64;arm64e', unmatched architectures='arm64e'. Set WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS explicitly."
  USE_LLVM ARCHITECTURES x86_64 arm64e REQUESTED AUTO)
