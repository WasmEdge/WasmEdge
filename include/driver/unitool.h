// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/driver/unitool.h - UniTool entry point ------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the entry point for the wasmedge unified tool executable.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace WasmEdge {
namespace Driver {

enum class ToolType : char { All, Compiler, Tool };

int UniTool(int Argc, const char *Argv[], const ToolType ToolSelect) noexcept;

} // namespace Driver
} // namespace WasmEdge
