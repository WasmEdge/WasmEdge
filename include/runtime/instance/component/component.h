// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/component.h -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the component instance definition.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <string>
#include <string_view>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

/// The component instance: the runtime state of one instantiation.
class ComponentInstance {
public:
  ComponentInstance(std::string_view Name) : CompName(Name) {}
  virtual ~ComponentInstance() noexcept = default;

  /// Getter for the component name.
  std::string_view getComponentName() const noexcept { return CompName; }

private:
  const std::string CompName;
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
