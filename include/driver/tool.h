// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/driver/tool.h - Tool entrypoint --------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the entrypoint for the tooling executable.
///
//===----------------------------------------------------------------------===//
#pragma once

namespace WasmEdge {
namespace Driver {

int Tool(int Argc, const char *Argv[]) noexcept;

} // namespace Driver
} // namespace WasmEdge
