# SPDX-License-Identifier: Apache-2.0

set(TOOLSCHAIN_PATH "${OHOS_DIR_PATH}/prebuilts/clang/ohos/linux-x86_64/llvm")
set(TOOLCHAIN_HOST "${TOOLSCHAIN_PATH}/bin")

set(OHOS_SYSROOT_PATH "${OHOS_DIR_PATH}/out/ohos-arm-release/obj/third_party/musl")
set(CMAKE_SYSROOT ${OHOS_SYSROOT_PATH})

set(CMAKE_CROSSCOMPILING TRUE)
set(CMAKE_SYSTEM_NAME "Generic")
set(CMAKE_CXX_COMPILER_ID Clang)
set(CMAKE_TOOLCHAIN_PREFIX llvm-)

set(LLVM_PATH "${OHOS_DIR_PATH}/prebuilts/clang/ohos/linux-x86_64/llvm")
include_directories(${LLVM_PATH}/include/c++/v1)
  
include_directories(${OHOS_SYSROOT_PATH}/usr/include/arm-linux-ohosmusl)
link_directories(${OHOS_SYSROOT_PATH}/usr/lib/arm-linux-ohosmusl)

set(TOOLCHAIN_CC "${TOOLCHAIN_HOST}/clang")
set(TOOLCHAIN_CXX "${TOOLCHAIN_HOST}/clang++")

set(CMAKE_C_COMPILER ${TOOLCHAIN_CC})
set(CMAKE_C_FLAGS "--target=arm-linux-ohosmusl -D__clang__ -march=armv7-a -mfloat-abi=softfp -mtune=generic-armv7-a -mfpu=neon -mthumb -w --sysroot=${OHOS_SYSROOT_PATH}")

set(CMAKE_CXX_COMPILER ${TOOLCHAIN_CXX})
set(CMAKE_CXX_FLAGS "--target=arm-linux-ohosmusl -D__clang__ -march=armv7-a -mfloat-abi=softfp -mtune=generic-armv7-a -mfpu=neon -mthumb -w --sysroot=${OHOS_SYSROOT_PATH}")

set(MY_LINK_FLAGS "--target=arm-linux-ohosmusl --sysroot=${OHOS_SYSROOT_PATH}")
set(CMAKE_LINKER clang)
set(CMAKE_CXX_LINKER clang++)
set(CMAKE_C_LINKER clang)
set(CMAKE_C_LINK_EXECUTABLE "${CMAKE_C_LINKER} ${MY_LINK_FLAGS} <FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")
set(CMAKE_CXX_LINK_EXECUTABLE "${CMAKE_CXX_LINKER} ${MY_LINK_FLAGS} <FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")


