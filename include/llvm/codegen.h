// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/llvm/codegen.h - Code Generator class definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Compiler class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/span.h"
#include "llvm/data.h"

#include <mutex>

namespace WasmEdge::LLVM {

/// Compiling LLVM Module into loadable executable binary.
class CodeGen {
public:
  CodeGen(const Configure &Conf) noexcept : Conf(Conf) {}
  Expect<void> codegen(Span<const Byte> WasmData, Data D,
                       std::filesystem::path OutputPath) noexcept;

private:
  const Configure Conf;
};

} // namespace WasmEdge::LLVM
