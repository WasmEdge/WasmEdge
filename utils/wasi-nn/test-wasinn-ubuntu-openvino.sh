#!/usr/bin/env bash
# SPDX-License-Identifier: Apache-2.0
# SPDX-FileCopyrightText: 2019-2024 Second State INC

ldconfig
export LD_LIBRARY_PATH="$(pwd)/build/lib/api:$LD_LIBRARY_PATH"

cd build
ctest
cd -
