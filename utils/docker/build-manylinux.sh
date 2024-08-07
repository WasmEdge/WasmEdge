#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2024 Second State INC

git config --global --add safe.directory $(pwd)

CMAKE_BUILD_TYPE="Release"
IS_BUILD_TARGET=true
IS_NINJA=true
CMAKE_OPTS=""

for i in "$@"; do
  case $i in
    --release|--Release)
      CMAKE_BUILD_TYPE="Release"
      shift
      ;;
    --debug|--Debug)
      CMAKE_BUILD_TYPE="Debug"
      shift
      ;;
    --not-build)
      IS_BUILD_TARGET=false
      shift
      ;;
    --not-ninja)
      IS_NINJA=false
      shift
      ;;
    *)
      CMAKE_OPTS="${CMAKE_OPTS} $i"
      shift
      ;;
  esac
done

if $IS_NINJA; then
  if ! cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DWASMEDGE_BUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM" ${CMAKE_OPTS} .; then
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
  fi
else
  rm -rf build
  mkdir build
  cd build
  if ! cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DWASMEDGE_BUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM;DEB" ${CMAKE_OPTS} ..; then
    cd ..
    echo === CMakeOutput.log ===
    cat build/CMakeFiles/CMakeOutput.log
    echo === CMakeError.log ===
    cat build/CMakeFiles/CMakeError.log
    exit 1
  fi
  cd ..
fi

if ${IS_BUILD_TARGET}; then
  cmake --build build
  cmake --build build --target package
fi
