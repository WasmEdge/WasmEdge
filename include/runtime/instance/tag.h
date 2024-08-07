// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/runtime/instance/tag.h - Tag Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the tag instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/type.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class TagInstance {
public:
  TagInstance() = delete;
  TagInstance(const AST::TagType &T, const AST::SubType *F) noexcept
      : TgType(T.getTypeIdx(), F) {}

  /// Getter of tag type.
  const AST::TagType &getTagType() const noexcept { return TgType; }

private:
  /// \name Data of tag instance.
  /// @{
  AST::TagType TgType;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
