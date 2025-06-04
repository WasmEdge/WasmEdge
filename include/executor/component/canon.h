// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2025 Second State INC

//=== wasmedge/executor/component/canon.h - canonical internal conversions ===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Executor class, which instantiate
/// and run Wasm modules.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "ast/component/instance.h"
#include "ast/module.h"
#include "common/errcode.h"
#include "common/types.h"
#include "executor/executor.h"
#include "runtime/instance/module.h"
#include "spdlog/spdlog.h"

#include <cmath>
#include <sstream>
#include <string_view>
#include <variant>

namespace WasmEdge {
namespace Executor {

using namespace std::literals;
using namespace AST::Component;
using namespace Runtime;

#define MAX_FLAT_PARAMS 16
#define MAX_FLAT_RESULTS 1

} // namespace Executor
} // namespace WasmEdge