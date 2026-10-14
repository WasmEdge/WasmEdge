// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

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
#include "gc/allocator.h"
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

  ~ExceptionInstance() noexcept {
    if (Allocator) {
      Allocator->removeException(*this);
    }
  }

  // Rooted by address: a copy/move would leave the new object unregistered and
  // its destructor's removeException pointing at an address never stored.
  ExceptionInstance(const ExceptionInstance &) = delete;
  ExceptionInstance(ExceptionInstance &&) = delete;
  ExceptionInstance &operator=(const ExceptionInstance &) = delete;
  ExceptionInstance &operator=(ExceptionInstance &&) = delete;

  /// Register this exception instance with the GC allocator so its payload
  /// values are scanned as roots: a struct/array reference in the payload
  /// survives only here once the on-stack copies are consumed, and throw_ref
  /// re-pushes the payload.
  void setAllocator(GC::Allocator &A) noexcept {
    assuming(Allocator == nullptr);
    Allocator = &A;
    Allocator->addException(*this);
  }

  /// Getter for the referenced tag instance.
  TagInstance *getTag() const noexcept { return TgInst; }

  /// Getter for the captured argument values.
  const std::vector<ValVariant> &getPayload() const noexcept { return Data; }

private:
  friend class GC::Allocator;
  void clearAllocator(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      Allocator = nullptr;
    }
  }

  /// \name Data of exception instance.
  /// @{
  GC::Allocator *Allocator = nullptr;
  TagInstance *TgInst;
  std::vector<ValVariant> Data;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
