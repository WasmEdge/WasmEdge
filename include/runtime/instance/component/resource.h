// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/component/resource.h - Resource Type ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the resource type instance definition: the runtime
/// identity of one component model resource type.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>
#include <functional>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;
class FunctionInstance;

namespace Component {

/// Runtime identity of a resource type: the instance that defines it and the
/// destructor to run, either a core function or a host callback.
struct ResourceTypeInstance {
  const ComponentInstance *Impl = nullptr;
  FunctionInstance *Dtor = nullptr;
  std::function<void(uint64_t)> HostDtor;
  /// True when the resource is represented by i64 (memory64 proposal).
  bool RepI64 = false;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
