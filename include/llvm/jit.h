// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/llvm/jit.h - JIT Engine class definition -----------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of JIT engine class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "llvm/data.h"
#include <vector>

namespace WasmEdge::LLVM {
class OrcLLJIT;

class JITLibrary : public Executable {
public:
  JITLibrary(OrcLLJIT JIT) noexcept;
  ~JITLibrary() noexcept override;

  Symbol<const IntrinsicsTable *> getIntrinsics() noexcept override;

  std::vector<Symbol<Wrapper>> getTypes(size_t Size) noexcept override;

  std::vector<Symbol<void>> getCodes(size_t Offset,
                                     size_t Size) noexcept override;

private:
  OrcLLJIT *J;
};

class JIT {
public:
  JIT(const Configure &Conf) noexcept : Conf(Conf) {}
  Expect<std::shared_ptr<Executable>> load(Data D) noexcept;

private:
  const Configure Conf;
};

} // namespace WasmEdge::LLVM
