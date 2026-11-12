#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: Copyright The WasmEdge Authors

ldconfig
export LD_LIBRARY_PATH="$(pwd)/build/lib/api:$LD_LIBRARY_PATH"

cd build
ctest
cd -
