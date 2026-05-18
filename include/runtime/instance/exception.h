// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/exception.h - Exception Instance --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the exception instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/types.h"
#include "runtime/instance/tag.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ExceptionInstance {
public:
  ExceptionInstance() = delete;
  ExceptionInstance(TagInstance *Tag,
                    std::vector<ValVariant> &&Payload) noexcept
      : TgInst(Tag), Data(std::move(Payload)) {
    assuming(TgInst);
  }

  /// Getter for the referenced tag instance.
  TagInstance *getTag() const noexcept { return TgInst; }

  /// Getter for the captured argument values.
  const std::vector<ValVariant> &getPayload() const noexcept { return Data; }

private:
  /// \name Data of exception instance.
  /// @{
  TagInstance *TgInst;
  std::vector<ValVariant> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
