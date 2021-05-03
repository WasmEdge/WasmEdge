// SPDX-License-Identifier: Apache-2.0
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
#include "api/wasmedge.h"

namespace WasmEdge {

WasmEdge_ConfigureContext *createConf(const Configure &Conf);

ErrCode convResult(WasmEdge_Result Res);

std::vector<ValVariant> convToValVec(const std::vector<WasmEdge_Value> &CVals);

std::vector<WasmEdge_Value> convFromValVec(const std::vector<ValVariant> &Vals,
                                           const std::vector<ValType> &Types);

} // namespace WasmEdge
