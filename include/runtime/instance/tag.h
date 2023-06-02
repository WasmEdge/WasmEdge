// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

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
  TagInstance(const AST::TagType &T, const AST::FunctionType* F) noexcept : TgType(T, F) {}

  /// Getter of tag type.
  const AST::TagType &getTagType() const noexcept { return TgType; }

  size_t getAssocValSize() const noexcept { return TgType.getAssocValSize(); }

private:
  /// \name Data of tag instance.
  /// @{
  AST::TagType TgType;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
