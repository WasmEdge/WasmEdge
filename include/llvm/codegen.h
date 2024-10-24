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

class MemoryBuffer;

/// Compiling LLVM Module into loadable executable binary.
class CodeGen {
public:
  CodeGen(const Configure &Conf) noexcept : Conf(Conf) {}
  Expect<void> codegen(Span<const Byte> WasmData, Data D,
                       std::filesystem::path OutputPath) noexcept;
  // returns number of bytes written to output buffer
  Expect<void> codegen_buffer(Span<const Byte> WasmData,
                       Data D, Span<const Byte> OutByte, // not actually const
                       uint32_t* OutSize) noexcept; 

private:
  Expect<LLVM::MemoryBuffer> codegen_osvec
                       (Span<const Byte>& WasmData, Data& D) noexcept;

  const Configure Conf;
};

} // namespace WasmEdge::LLVM
