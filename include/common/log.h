// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
#include "spdlog/fmt/ostr.h"
#include "spdlog/spdlog.h"

namespace WasmEdge {
namespace Log {

void setLogOff();

void setDebugLoggingLevel();

void setInfoLoggingLevel();

void setWarnLoggingLevel();

void setErrorLoggingLevel();

} // namespace Log
} // namespace WasmEdge
