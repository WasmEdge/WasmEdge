// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/executor/registered_roots_claim.h - GC root claim --------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the RegisteredRootsClaim class, which the Executor uses
/// to take ownership of a module's GC roots for its allocator.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "gc/allocator.h"
#include "runtime/instance/global.h"
#include "runtime/instance/table.h"

#include <cstdint>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {

/// An RAII transaction over the GC roots a single registration newly claimed
/// for an executor's allocator. The claim is reversed on destruction unless
/// commit() is called, so it spans the whole registration rather than just the
/// attach walk: a registration failing after the attach (for example
/// StoreManager::registerModule rejecting a duplicate name) must not leave the
/// module's tables and globals durably owned by this allocator. A leaked claim
/// is not memory-unsafe, but it would keep scanning a module that was never
/// published and make a later registration of the same module into a different
/// executor fail the foreign-hazard check, turning a recoverable name conflict
/// into a permanent cross-executor rejection.
class RegisteredRootsClaim {
public:
  RegisteredRootsClaim() noexcept = default;
  explicit RegisteredRootsClaim(GC::Allocator &A) noexcept : Alloc(&A) {}
  RegisteredRootsClaim(const RegisteredRootsClaim &) = delete;
  RegisteredRootsClaim &operator=(const RegisteredRootsClaim &) = delete;
  RegisteredRootsClaim(RegisteredRootsClaim &&Other) noexcept
      : Alloc(Other.Alloc), Tables(std::move(Other.Tables)),
        Globals(std::move(Other.Globals)), Gen(Other.Gen),
        FullIndexSpace(Other.FullIndexSpace),
        AllocatorSpecific(Other.AllocatorSpecific) {
    // Disarm the source so the moved-from guard cannot double-detach.
    Other.disarm();
  }
  RegisteredRootsClaim &operator=(RegisteredRootsClaim &&Other) noexcept {
    if (this != &Other) {
      rollback();
      Alloc = Other.Alloc;
      Tables = std::move(Other.Tables);
      Globals = std::move(Other.Globals);
      Gen = Other.Gen;
      FullIndexSpace = Other.FullIndexSpace;
      AllocatorSpecific = Other.AllocatorSpecific;
      Other.disarm();
    }
    return *this;
  }
  ~RegisteredRootsClaim() noexcept { rollback(); }

  /// Record a hold this registration took on a root (and must release on
  /// failure). A freely-shareable foreign attach is not recorded: it left a
  /// prior owner in place and holds nothing.
  void recordTable(Runtime::Instance::TableInstance &T) noexcept {
    Tables.push_back(&T);
  }
  void recordGlobal(Runtime::Instance::GlobalInstance &G) noexcept {
    Globals.push_back(&G);
  }

  /// The registration completed: keep every hold permanently, so nothing can
  /// detach these roots under the published module.
  void commit() noexcept { disarm(); }

  /// \name What the walk saw, read under the module lock it held.
  /// @{
  /// Index-space generation the walk observed; the stamp is checked against it
  /// (ModuleInstance::stampGCRootsOwner).
  uint64_t generation() const noexcept { return Gen; }
  /// The walked roots are the module's whole table/global index space: it
  /// imports no table and no global (registerModule walks owned roots only),
  /// or the walk included the imports.
  bool coversFullIndexSpace() const noexcept { return FullIndexSpace; }
  /// Some walked root ties the module to one allocator: a managed-capable
  /// table or global, or a growable table. Without one, any executor may run
  /// the module (ModuleInstance::kSharedGCRootsOwner).
  bool allocatorSpecific() const noexcept { return AllocatorSpecific; }
  void noteWalk(uint64_t G, bool Full) noexcept {
    Gen = G;
    FullIndexSpace = Full;
  }
  void noteAllocatorSpecificRoot() noexcept { AllocatorSpecific = true; }
  /// @}

private:
  void disarm() noexcept {
    Alloc = nullptr;
    Tables.clear();
    Globals.clear();
  }
  void rollback() noexcept {
    if (Alloc == nullptr) {
      return;
    }
    for (auto *T : Tables) {
      T->detachAllocator(*Alloc);
    }
    for (auto *G : Globals) {
      G->detachAllocator(*Alloc);
    }
    disarm();
  }
  GC::Allocator *Alloc = nullptr;
  std::vector<Runtime::Instance::TableInstance *> Tables;
  std::vector<Runtime::Instance::GlobalInstance *> Globals;
  uint64_t Gen = 0;
  bool FullIndexSpace = false;
  bool AllocatorSpecific = false;
};

} // namespace Executor
} // namespace WasmEdge
