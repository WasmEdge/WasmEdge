// SPDX-License-Identifier: Apache-2.0
//===-- wasmedge/common/log.h - Logging system ----------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the linkage of logging system.
///
//===----------------------------------------------------------------------===//
#pragma once

#define SPDLOG_NO_EXCEPTIONS 1
#include "spdlog/spdlog.h"
#include "spdlog/fmt/ostr.h"

namespace WasmEdge {
namespace Log {

void setDebugLoggingLevel();

void setErrorLoggingLevel();

} // namespace Log
} // namespace WasmEdge
