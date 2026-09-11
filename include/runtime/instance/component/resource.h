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

#include "common/types.h"

#include <cstdint>
#include <functional>
#include <utility>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ComponentInstance;
class FunctionInstance;

namespace Component {

/// Runtime identity of a resource type: the instance that defines it and the
/// destructor to run, either a core function or a host callback.
class ResourceTypeInstance {
public:
  ResourceTypeInstance(const ComponentInstance *ImplInst,
                       FunctionInstance *DtorFunc,
                       std::function<void(uint64_t)> HostDtorFunc,
                       bool IsRepI64) noexcept
      : Impl(ImplInst), Dtor(DtorFunc), HostDtor(std::move(HostDtorFunc)),
        RepI64(IsRepI64) {}

  const ComponentInstance *getImpl() const noexcept { return Impl; }
  FunctionInstance *getDtor() const noexcept { return Dtor; }
  const std::function<void(uint64_t)> &getHostDtor() const noexcept {
    return HostDtor;
  }
  /// The core type of a representation.
  ValType getRepType() const noexcept {
    return ValType(RepI64 ? TypeCode::I64 : TypeCode::I32);
  }
  /// The representation held by a flat slot of getRepType().
  uint64_t getRep(const ValVariant &V) const noexcept {
    return RepI64 ? V.get<uint64_t>()
                  : static_cast<uint64_t>(V.get<uint32_t>());
  }
  /// The flat slot of getRepType() holding Rep.
  ValVariant getRepSlot(uint64_t Rep) const noexcept {
    return RepI64 ? ValVariant(Rep) : ValVariant(static_cast<uint32_t>(Rep));
  }

private:
  const ComponentInstance *Impl;
  FunctionInstance *Dtor;
  std::function<void(uint64_t)> HostDtor;
  bool RepI64;
};

} // namespace Component
} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
