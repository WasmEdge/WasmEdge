// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/test/api/helper.h - Spec test helpers for C API ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file parse and run tests of Wasm test suites extracted by wast2json.
/// Test Suits: https://github.com/WebAssembly/spec/tree/master/test/core
/// wast2json: https://webassembly.github.io/wabt/doc/wast2json.1.html
///
//===----------------------------------------------------------------------===//
#pragma once

#include "../spec/spectest.h"
#include "common/int128.h"
#include "wasmedge/wasmedge.h"
#include <utility>
#include <vector>

namespace WasmEdge {

WasmEdge_ConfigureContext *createConf(const Configure &Conf);

ErrCode convResult(WasmEdge_Result Res);

std::pair<ValVariant, ValType> convToVal(const WasmEdge_Value &CVal);

WasmEdge_Value convFromVal(const ValVariant &Val, const ValType &Type);

std::vector<std::pair<ValVariant, ValType>>
convToValVec(const std::vector<WasmEdge_Value> &CVals);

std::vector<WasmEdge_Value> convFromValVec(const std::vector<ValVariant> &Vals,
                                           const std::vector<ValType> &Types);

} // namespace WasmEdge
