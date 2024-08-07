// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/driver/unitool.h - UniTool entrypoint ------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the entrypoint for the wasmedge unified tool executable.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace WasmEdge {
namespace Driver {

enum class ToolType : char { All, Compiler, Tool };

int UniTool(int Argc, const char *Argv[], const ToolType ToolSelect) noexcept;

} // namespace Driver
} // namespace WasmEdge
