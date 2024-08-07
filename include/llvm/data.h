// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/llvm/data.h - Data class definition ----------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file is the definition class of Data class.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/configure.h"
#include "common/errcode.h"
#include "common/filesystem.h"
#include "common/span.h"

#include <mutex>

namespace WasmEdge::LLVM {

/// Holds llvm-relative runtime data, like llvm::Context, llvm::Module, etc.
class Data {
public:
  struct DataContext;
  Data() noexcept;
  ~Data() noexcept;
  Data(Data &&) noexcept;
  Data &operator=(Data &&) noexcept;
  DataContext &extract() noexcept { return *Context; }

private:
  std::unique_ptr<DataContext> Context;
  const Configure Conf;
};

} // namespace WasmEdge::LLVM
