#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# SPDX-FileCopyrightText: 2019-2022 Second State INC

curl -s -L -O --remote-name-all https://boostorg.jfrog.io/artifactory/main/release/1.82.0/source/boost_1_82_0.tar.bz2
echo "a6e1ab9b0860e6a2881dd7b21fe9f737a095e5f33a3a874afc6a345228597ee6  boost_1_82_0.tar.bz2" | sha256sum -c
git config --global --add safe.directory $(pwd)
bzip2 -dc boost_1_82_0.tar.bz2 | tar -xf -

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
  if ! cmake -Bbuild -GNinja -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DWASMEDGE_BUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM;DEB" -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_INCLUDEDIR=$(pwd)/boost_1_82_0/ ${CMAKE_OPTS} .; then
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
  if ! cmake -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE} -DWASMEDGE_BUILD_PACKAGE="TGZ;TBZ2;TXZ;TZST;RPM;DEB" -DBoost_NO_SYSTEM_PATHS=TRUE -DBOOST_INCLUDEDIR=$(pwd)/../boost_1_82_0/ ${CMAKE_OPTS} ..; then
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
