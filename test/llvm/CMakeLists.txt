# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeLLVMCoreTests
  LinkerTest.cpp
  LLVMcoreTest.cpp
  NativeWriterTest.cpp
)

add_test(wasmedgeLLVMCoreTests wasmedgeLLVMCoreTests)

add_test(NAME wasmedgeLLVMRelocationBackendProcessor
  COMMAND ${CMAKE_COMMAND}
    -DWASMEDGE_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/RelocationBackendTest.cmake)
add_test(NAME wasmedgeLLVMCMakeArgument
  COMMAND ${CMAKE_COMMAND}
    -DWASMEDGE_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/CMakeArgumentTest.cmake)
add_test(NAME wasmedgeLLVMStaticZstdTarget
  COMMAND ${CMAKE_COMMAND}
    -DWASMEDGE_SOURCE_DIR=${PROJECT_SOURCE_DIR}
    -P ${CMAKE_CURRENT_SOURCE_DIR}/StaticZstdTargetTest.cmake)

target_link_libraries(wasmedgeLLVMCoreTests
  PRIVATE
  wasmedgeTestSpec
  wasmedgeVM
)

if(NOT WASMEDGE_LINK_LLVM_STATIC)
  find_package(LLVM REQUIRED HINTS "${LLVM_DIR}")

  if(LLVM_LINK_LLVM_DYLIB AND TARGET LLVM)
    set(WASMEDGE_LLVM_TEST_TARGET_LIBS LLVM)
  else()
    set(WASMEDGE_LLVM_TEST_TARGET_COMPONENTS)
    foreach(LLVM_TARGET IN LISTS LLVM_TARGETS_TO_BUILD)
      if(LLVM${LLVM_TARGET}CodeGen IN_LIST LLVM_AVAILABLE_LIBS)
        list(APPEND WASMEDGE_LLVM_TEST_TARGET_COMPONENTS
          ${LLVM_TARGET}CodeGen)
      elseif(LLVM${LLVM_TARGET} IN_LIST LLVM_AVAILABLE_LIBS)
        list(APPEND WASMEDGE_LLVM_TEST_TARGET_COMPONENTS ${LLVM_TARGET})
      endif()
      foreach(LLVM_TARGET_COMPONENT IN ITEMS AsmParser Desc Info Utils)
        if(LLVM${LLVM_TARGET}${LLVM_TARGET_COMPONENT} IN_LIST LLVM_AVAILABLE_LIBS)
          list(APPEND WASMEDGE_LLVM_TEST_TARGET_COMPONENTS
            ${LLVM_TARGET}${LLVM_TARGET_COMPONENT})
        endif()
      endforeach()
    endforeach()
    llvm_map_components_to_libnames(WASMEDGE_LLVM_TEST_TARGET_LIBS
      ${WASMEDGE_LLVM_TEST_TARGET_COMPONENTS})
  endif()
  target_link_libraries(wasmedgeLLVMCoreTests PRIVATE
    ${WASMEDGE_LLVM_TEST_TARGET_LIBS}
  )
endif()

foreach(BACKEND IN LISTS WASMEDGE_LLVM_LINKER_ALL_RELOCATION_BACKENDS)
  target_compile_definitions(wasmedgeLLVMCoreTests PRIVATE
    WASMEDGE_LINKER_HAS_${BACKEND}=$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_${BACKEND}>
  )
endforeach()

target_include_directories(wasmedgeLLVMCoreTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/lib/llvm
)

target_include_directories(wasmedgeLLVMCoreTests
  SYSTEM
  PRIVATE
  $<TARGET_PROPERTY:wasmedgeLLVM,INCLUDE_DIRECTORIES>
)

if(MSVC)
  target_compile_options(wasmedgeLLVMCoreTests PRIVATE /wd4244 /wd4267)
endif()

wasmedge_add_executable(wasmedgeLazyJITTests
  LazyJITTest.cpp
)

add_test(wasmedgeLazyJITTests wasmedgeLazyJITTests)

target_link_libraries(wasmedgeLazyJITTests
  PRIVATE
  wasmedgeVM
  GTest::gtest
)
