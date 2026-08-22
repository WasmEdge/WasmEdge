# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

wasmedge_add_executable(wasmedgeLLVMCoreTests
  LinkerTest.cpp
  LLVMcoreTest.cpp
  NativeWriterTest.cpp
)

add_test(wasmedgeLLVMCoreTests wasmedgeLLVMCoreTests)

if(NOT WASMEDGE_RELOCATION_BACKEND_CONFIGURE_TESTING)
  if(APPLE AND CMAKE_OSX_ARCHITECTURES)
    wasmedge_llvm_linker_backends_for_processors(
      WASMEDGE_RELOCATION_BACKEND_EXPECTED_HOST
      "${CMAKE_OSX_ARCHITECTURES}"
    )
  else()
    wasmedge_llvm_linker_backend_for_processor(
      WASMEDGE_RELOCATION_BACKEND_EXPECTED_HOST
      "${CMAKE_SYSTEM_PROCESSOR}"
    )
  endif()
  set(WASMEDGE_RELOCATION_BACKEND_CONFIGURE_TEST
    ${CMAKE_CURRENT_BINARY_DIR}/relocation_backend_configure.cmake)
  file(GENERATE OUTPUT ${WASMEDGE_RELOCATION_BACKEND_CONFIGURE_TEST} CONTENT
"set(ConfigureCommand
  \"${CMAKE_COMMAND}\"
  -S \"${PROJECT_SOURCE_DIR}\"
  -G \"${CMAKE_GENERATOR}\")
include(\"${PROJECT_SOURCE_DIR}/cmake/LLVMRelocationBackends.cmake\")
if(NOT \"${CMAKE_GENERATOR_PLATFORM}\" STREQUAL \"\")
  list(APPEND ConfigureCommand -A \"${CMAKE_GENERATOR_PLATFORM}\")
endif()
if(NOT \"${CMAKE_GENERATOR_TOOLSET}\" STREQUAL \"\")
  list(APPEND ConfigureCommand -T \"${CMAKE_GENERATOR_TOOLSET}\")
endif()
if(NOT \"${CMAKE_GENERATOR_INSTANCE}\" STREQUAL \"\")
  list(APPEND ConfigureCommand
    \"-DCMAKE_GENERATOR_INSTANCE:INTERNAL=${CMAKE_GENERATOR_INSTANCE}\")
endif()
function(append_cache_argument Name Type Value)
  wasmedge_cmake_cache_argument(Argument \"\${Name}\" \"\${Type}\" \"\${Value}\")
  list(APPEND ConfigureCommand \"\${Argument}\")
  set(ConfigureCommand \"\${ConfigureCommand}\" PARENT_SCOPE)
endfunction()
append_cache_argument(CMAKE_C_COMPILER FILEPATH [==[${CMAKE_C_COMPILER}]==])
append_cache_argument(CMAKE_CXX_COMPILER FILEPATH [==[${CMAKE_CXX_COMPILER}]==])
append_cache_argument(CMAKE_TOOLCHAIN_FILE FILEPATH [==[${CMAKE_TOOLCHAIN_FILE}]==])
append_cache_argument(CMAKE_BUILD_TYPE STRING [==[${CMAKE_BUILD_TYPE}]==])
append_cache_argument(CMAKE_PREFIX_PATH PATH [==[${CMAKE_PREFIX_PATH}]==])
append_cache_argument(LLVM_DIR PATH [==[${LLVM_DIR}]==])
append_cache_argument(LLVM_CMAKE_PATH PATH [==[${LLVM_CMAKE_PATH}]==])
list(APPEND ConfigureCommand
  -DWASMEDGE_BUILD_SHARED_LIB=${WASMEDGE_BUILD_SHARED_LIB}
  -DWASMEDGE_BUILD_STATIC_LIB=${WASMEDGE_BUILD_STATIC_LIB}
  -DWASMEDGE_LINK_LLVM_STATIC=${WASMEDGE_LINK_LLVM_STATIC}
  -DWASMEDGE_LINK_TOOLS_STATIC=${WASMEDGE_LINK_TOOLS_STATIC}
  -DWASMEDGE_FORCE_DISABLE_LTO=${WASMEDGE_FORCE_DISABLE_LTO}
  -DWASMEDGE_BUILD_TOOLS=OFF
  -DWASMEDGE_BUILD_PLUGINS=OFF
  -DWASMEDGE_RELOCATION_BACKEND_CONFIGURE_TESTING=ON)
set(ExpectedOsxArchitectures [==[${CMAKE_OSX_ARCHITECTURES}]==])
set(ExpectedOsxSysroot [==[${CMAKE_OSX_SYSROOT}]==])
set(ExpectedOsxDeploymentTarget [==[${CMAKE_OSX_DEPLOYMENT_TARGET}]==])
if(NOT ExpectedOsxArchitectures STREQUAL \"\")
  wasmedge_cmake_cache_argument(Argument CMAKE_OSX_ARCHITECTURES STRING
    \"\${ExpectedOsxArchitectures}\")
  list(APPEND ConfigureCommand \"\${Argument}\")
endif()
if(NOT ExpectedOsxSysroot STREQUAL \"\")
  wasmedge_cmake_cache_argument(Argument CMAKE_OSX_SYSROOT PATH
    \"\${ExpectedOsxSysroot}\")
  list(APPEND ConfigureCommand \"\${Argument}\")
endif()
if(NOT ExpectedOsxDeploymentTarget STREQUAL \"\")
  wasmedge_cmake_cache_argument(Argument CMAKE_OSX_DEPLOYMENT_TARGET STRING
    \"\${ExpectedOsxDeploymentTarget}\")
  list(APPEND ConfigureCommand \"\${Argument}\")
endif()

function(check_cache_value Build Name Type Expected)
  if(NOT Expected STREQUAL \"\")
    wasmedge_read_cmake_cache_value(
      Actual \"\${Build}/CMakeCache.txt\" \"\${Name}\" \"\${Type}\")
    if(NOT Actual STREQUAL Expected)
      message(FATAL_ERROR
        \"\${Name} cache value '\${Actual}' did not preserve '\${Expected}'\")
    endif()
  endif()
endfunction()

function(configure_case Name)
  file(REMOVE_RECURSE \"${CMAKE_CURRENT_BINARY_DIR}/configure-\${Name}\")
  execute_process(
    COMMAND \${ConfigureCommand}
      -B \"${CMAKE_CURRENT_BINARY_DIR}/configure-\${Name}\"
      \${ARGN}
    RESULT_VARIABLE Result
    OUTPUT_VARIABLE Output
    ERROR_VARIABLE Error
  )
  string(CONCAT Output \"\${Output}\" \"\${Error}\")
  set(ConfigureResult \"\${Result}\" PARENT_SCOPE)
  set(ConfigureOutput \"\${Output}\" PARENT_SCOPE)
endfunction()

foreach(Case IN ITEMS unknown empty)
  if(Case STREQUAL unknown)
    set(Processor unknown)
  else()
    set(Processor)
  endif()
  configure_case(auto-\${Case}
    -DWASMEDGE_USE_LLVM=ON
    -DWASMEDGE_BUILD_TESTS=OFF
    -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
    -DCMAKE_OSX_ARCHITECTURES=
    -DCMAKE_SYSTEM_PROCESSOR=\${Processor})
  string(CONCAT ExpectedProcessorDiagnostic
    [=[CMAKE_SYSTEM_PROCESSOR=']=] \${Processor} [=[']=])
  if(ConfigureResult EQUAL 0 OR NOT ConfigureOutput MATCHES
     [=[AUTO could not select an LLVM linker relocation backend]=] OR
     NOT ConfigureOutput MATCHES \${ExpectedProcessorDiagnostic})
    message(FATAL_ERROR [=[unresolved AUTO configure result: ]=]
      \${ConfigureOutput})
  endif()
endforeach()

configure_case(auto-apple-x86-64h
  -DWASMEDGE_USE_LLVM=ON
  -DWASMEDGE_BUILD_TESTS=OFF
  -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
  -DCMAKE_SYSTEM_PROCESSOR=x86_64
  -DCMAKE_OSX_ARCHITECTURES=x86_64h)
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR [=[x86_64h Apple AUTO configure failed: ]=]
    \${ConfigureOutput})
endif()
wasmedge_read_cmake_cache_value(Backend
  \"${CMAKE_CURRENT_BINARY_DIR}/configure-auto-apple-x86-64h/CMakeCache.txt\"
  WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS_EFFECTIVE INTERNAL)
if(NOT Backend STREQUAL X86_64)
  message(FATAL_ERROR [=[x86_64h Apple AUTO selected: ]=] \${Backend})
endif()

wasmedge_cmake_cache_argument(AppleArchitectures CMAKE_OSX_ARCHITECTURES STRING
  [=[x86_64;arm64e]=])
configure_case(auto-apple-mixed
  -DWASMEDGE_USE_LLVM=ON
  -DWASMEDGE_BUILD_TESTS=OFF
  -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
  -DCMAKE_SYSTEM_PROCESSOR=x86_64
  \"\${AppleArchitectures}\")
if(ConfigureResult EQUAL 0 OR NOT ConfigureOutput MATCHES
   [=[AUTO could not select an LLVM linker relocation backend]=] OR
   NOT ConfigureOutput MATCHES [=[CMAKE_OSX_ARCHITECTURES.*arm64e]=])
  message(FATAL_ERROR [=[mixed Apple AUTO configure result: ]=]
    \${ConfigureOutput})
endif()

configure_case(auto-empty-llvm-off
  -DWASMEDGE_USE_LLVM=OFF
  -DWASMEDGE_BUILD_TESTS=OFF
  -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
  -DCMAKE_SYSTEM_PROCESSOR=)
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR [=[LLVM-off empty processor failed: ]=]
    \${ConfigureOutput})
endif()

configure_case(explicit-unknown-processor
  -DWASMEDGE_USE_LLVM=ON
  -DWASMEDGE_BUILD_TESTS=OFF
  -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
  -DCMAKE_SYSTEM_PROCESSOR=unknown
  -DWASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS=X86_64)
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR [=[explicit backend configure failed: ]=]
    \${ConfigureOutput})
endif()

configure_case(invalid
  -DWASMEDGE_USE_LLVM=OFF
  -DWASMEDGE_BUILD_TESTS=OFF
  \"-DWASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS=X86_64\\;UNKNOWN\")
if(ConfigureResult EQUAL 0 OR NOT ConfigureOutput MATCHES
   \"Unknown WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS value: UNKNOWN\")
  message(FATAL_ERROR \"invalid backend configure result: \${ConfigureOutput}\")
endif()

set(ExpectedHostBackend ${WASMEDGE_RELOCATION_BACKEND_EXPECTED_HOST})
set(HostSystemArguments
  -DCMAKE_SYSTEM_NAME=${CMAKE_SYSTEM_NAME}
  -DCMAKE_SYSTEM_PROCESSOR=${CMAKE_SYSTEM_PROCESSOR})

configure_case(auto-off-on
  -DWASMEDGE_USE_LLVM=OFF
  -DWASMEDGE_BUILD_TESTS=OFF
  \${HostSystemArguments})
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR \"host default configure failed: \${ConfigureOutput}\")
endif()
check_cache_value(
  \"${CMAKE_CURRENT_BINARY_DIR}/configure-auto-off-on\"
  CMAKE_OSX_ARCHITECTURES STRING \"\${ExpectedOsxArchitectures}\")
check_cache_value(
  \"${CMAKE_CURRENT_BINARY_DIR}/configure-auto-off-on\"
  CMAKE_OSX_SYSROOT PATH \"\${ExpectedOsxSysroot}\")
check_cache_value(
  \"${CMAKE_CURRENT_BINARY_DIR}/configure-auto-off-on\"
  CMAKE_OSX_DEPLOYMENT_TARGET STRING \"\${ExpectedOsxDeploymentTarget}\")
execute_process(
  COMMAND \${ConfigureCommand}
    -B \"${CMAKE_CURRENT_BINARY_DIR}/configure-auto-off-on\"
    -DWASMEDGE_USE_LLVM=OFF
    -DWASMEDGE_BUILD_TESTS=ON
    \${HostSystemArguments}
  RESULT_VARIABLE ConfigureResult
  OUTPUT_VARIABLE ConfigureOutput
  ERROR_VARIABLE ConfigureError)
string(CONCAT ConfigureOutput \"\${ConfigureOutput}\" \"\${ConfigureError}\")
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR \"AUTO OFF to ON reconfigure failed: \${ConfigureOutput}\")
endif()
wasmedge_read_cmake_cache_value(Backend
  \"${CMAKE_CURRENT_BINARY_DIR}/configure-auto-off-on/CMakeCache.txt\"
  WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS_EFFECTIVE INTERNAL)
if(NOT Backend STREQUAL \"X86_64;AARCH64;ARM;RISCV64;S390X\")
  message(FATAL_ERROR \"AUTO OFF to ON selected \${Backend}\")
endif()

configure_case(auto-on-off
  -DWASMEDGE_USE_LLVM=OFF
  -DWASMEDGE_BUILD_TESTS=ON
  \${HostSystemArguments})
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR \"test default configure failed: \${ConfigureOutput}\")
endif()
execute_process(
  COMMAND \${ConfigureCommand}
    -B \"${CMAKE_CURRENT_BINARY_DIR}/configure-auto-on-off\"
    -DWASMEDGE_USE_LLVM=OFF
    -DWASMEDGE_BUILD_TESTS=OFF
    \${HostSystemArguments}
  RESULT_VARIABLE ConfigureResult
  OUTPUT_VARIABLE ConfigureOutput
  ERROR_VARIABLE ConfigureError)
string(CONCAT ConfigureOutput \"\${ConfigureOutput}\" \"\${ConfigureError}\")
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR \"AUTO ON to OFF reconfigure failed: \${ConfigureOutput}\")
endif()
wasmedge_read_cmake_cache_value(Backend
  \"${CMAKE_CURRENT_BINARY_DIR}/configure-auto-on-off/CMakeCache.txt\"
  WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS_EFFECTIVE INTERNAL)
if(NOT Backend STREQUAL ExpectedHostBackend)
  message(FATAL_ERROR \"AUTO ON to OFF selected \${Backend}\")
endif()

configure_case(explicit-subset
  -DWASMEDGE_USE_LLVM=OFF
  -DWASMEDGE_BUILD_TESTS=OFF
  -DWASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS=X86_64)
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR \"explicit subset configure failed: \${ConfigureOutput}\")
endif()
execute_process(
  COMMAND \${ConfigureCommand}
    -B \"${CMAKE_CURRENT_BINARY_DIR}/configure-explicit-subset\"
    -DWASMEDGE_USE_LLVM=OFF
    -DWASMEDGE_BUILD_TESTS=ON
  RESULT_VARIABLE ConfigureResult
  OUTPUT_VARIABLE ConfigureOutput
  ERROR_VARIABLE ConfigureError)
string(CONCAT ConfigureOutput \"\${ConfigureOutput}\" \"\${ConfigureError}\")
if(NOT ConfigureResult EQUAL 0)
  message(FATAL_ERROR \"explicit subset reconfigure failed: \${ConfigureOutput}\")
endif()
wasmedge_read_cmake_cache_value(Backend
  \"${CMAKE_CURRENT_BINARY_DIR}/configure-explicit-subset/CMakeCache.txt\"
  WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS_EFFECTIVE INTERNAL)
if(NOT Backend STREQUAL X86_64)
  message(FATAL_ERROR \"explicit subset changed: \${Backend}\")
endif()

foreach(Case IN ITEMS minimal all)
  if(Case STREQUAL minimal)
    set(BackendArguments -DWASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS=X86_64)
  else()
    set(AllBackends \"${CMAKE_CURRENT_BINARY_DIR}/all-backends.cmake\")
    file(WRITE \"\${AllBackends}\"
      \"set(WASMEDGE_LLVM_LINKER_RELOCATION_BACKENDS X86_64 AARCH64 ARM RISCV64 S390X CACHE STRING \\\"\\\")\\n\")
    set(BackendArguments -C \"\${AllBackends}\")
  endif()
  configure_case(\${Case}
    -DWASMEDGE_USE_LLVM=ON
    -DWASMEDGE_BUILD_TESTS=OFF
    \${BackendArguments})
  if(NOT ConfigureResult EQUAL 0)
    message(FATAL_ERROR \"\${Case} configure failed: \${ConfigureOutput}\")
  endif()
  include(
    \"${CMAKE_CURRENT_BINARY_DIR}/configure-\${Case}/lib/llvm/relocation_backends.cmake\")
  set(WASMEDGE_LINKER_RELOCATION_SOURCES
    \${WASMEDGE_LINKER_TARGET_SOURCES})
  list(FILTER WASMEDGE_LINKER_RELOCATION_SOURCES INCLUDE REGEX
    \"linker/relocation_(x86_64|aarch64|arm|riscv|s390x)\\.cpp$\")
  set(WASMEDGE_LINKER_HAS_DEFINITIONS
    \${WASMEDGE_LINKER_RELOCATION_DEFINITIONS})
  list(FILTER WASMEDGE_LINKER_HAS_DEFINITIONS INCLUDE REGEX
    \"^WASMEDGE_LINKER_HAS_\")
  if(Case STREQUAL minimal)
    if(NOT WASMEDGE_LINKER_RELOCATION_SOURCES STREQUAL
       \"linker/relocation_x86_64.cpp\" OR
       NOT WASMEDGE_LINKER_HAS_DEFINITIONS STREQUAL
       \"WASMEDGE_LINKER_HAS_X86_64=1;WASMEDGE_LINKER_HAS_AARCH64=0;WASMEDGE_LINKER_HAS_ARM=0;WASMEDGE_LINKER_HAS_RISCV64=0;WASMEDGE_LINKER_HAS_S390X=0\" OR
       NOT WASMEDGE_LINKER_RELOCATION_PROPERTIES STREQUAL \"1;0;0;0;0\")
      message(FATAL_ERROR \"unexpected minimal backend manifest\")
    endif()
  else()
    if(NOT WASMEDGE_LINKER_RELOCATION_SOURCES STREQUAL
       \"linker/relocation_x86_64.cpp;linker/relocation_aarch64.cpp;linker/relocation_arm.cpp;linker/relocation_riscv.cpp;linker/relocation_s390x.cpp\" OR
       NOT WASMEDGE_LINKER_HAS_DEFINITIONS STREQUAL
       \"WASMEDGE_LINKER_HAS_X86_64=1;WASMEDGE_LINKER_HAS_AARCH64=1;WASMEDGE_LINKER_HAS_ARM=1;WASMEDGE_LINKER_HAS_RISCV64=1;WASMEDGE_LINKER_HAS_S390X=1\" OR
       NOT WASMEDGE_LINKER_RELOCATION_PROPERTIES STREQUAL \"1;1;1;1;1\")
      message(FATAL_ERROR \"unexpected all backend manifest\")
    endif()
  endif()
endforeach()
")
  add_test(NAME wasmedgeLLVMRelocationBackendConfigure
    COMMAND ${CMAKE_COMMAND} -P ${WASMEDGE_RELOCATION_BACKEND_CONFIGURE_TEST})
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
endif()

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

target_compile_definitions(wasmedgeLLVMCoreTests PRIVATE
  WASMEDGE_LINKER_HAS_X86_64=$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_X86_64>
  WASMEDGE_LINKER_HAS_AARCH64=$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_AARCH64>
  WASMEDGE_LINKER_HAS_ARM=$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_ARM>
  WASMEDGE_LINKER_HAS_RISCV64=$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_RISCV64>
  WASMEDGE_LINKER_HAS_S390X=$<TARGET_PROPERTY:wasmedgeLLVM,WASMEDGE_LINKER_HAS_S390X>
)

target_include_directories(wasmedgeLLVMCoreTests
  PRIVATE
  ${PROJECT_SOURCE_DIR}/lib/llvm
)

target_include_directories(wasmedgeLLVMCoreTests
  SYSTEM
  PRIVATE
  $<TARGET_PROPERTY:wasmedgeLLVM,INCLUDE_DIRECTORIES>
)

wasmedge_add_executable(wasmedgeLazyJITTests
  LazyJITTest.cpp
)

add_test(wasmedgeLazyJITTests wasmedgeLazyJITTests)

target_link_libraries(wasmedgeLazyJITTests
  PRIVATE
  wasmedgeVM
  GTest::gtest
)
