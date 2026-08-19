// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/component/instantiator.h ------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// The state of one component instantiation: the destination instance and
/// where its imports come from.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/component/storemgr.h"
#include "runtime/instance/component/component.h"

namespace WasmEdge {
namespace Executor {
namespace Component {

/// One component instantiation in progress. A root instantiation resolves its
/// imports through the store, a nested one through the instantiation
/// arguments. Everything else about the instantiation lives on the instance.
class Instantiator {
public:
  /// Root instantiation.
  Instantiator(Runtime::Component::StoreManager &S,
               Runtime::Instance::ComponentInstance &I) noexcept
      : Store(&S), Inst(I) {}

  /// Nested instantiation.
  Instantiator(Runtime::Instance::Component::ImportManager &M,
               Runtime::Instance::ComponentInstance &I) noexcept
      : ImportMgr(&M), Inst(I) {}

  /// True for a root instantiation, whose imports the embedder provides.
  bool isRoot() const noexcept { return Store != nullptr; }

  Runtime::Component::StoreManager &store() const noexcept { return *Store; }
  Runtime::Instance::Component::ImportManager &imports() const noexcept {
    return *ImportMgr;
  }
  Runtime::Instance::ComponentInstance &inst() const noexcept { return Inst; }

private:
  Runtime::Component::StoreManager *Store = nullptr;
  Runtime::Instance::Component::ImportManager *ImportMgr = nullptr;
  Runtime::Instance::ComponentInstance &Inst;
};

} // namespace Component
} // namespace Executor
} // namespace WasmEdge
