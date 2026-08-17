// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/elem.h - Element Instance definition ----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the element instance definition in store manager.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/errinfo.h"
#include "common/span.h"
#include "common/spdlog.h"
#include "common/types.h"
#include "gc/allocator.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {

class ElementInstance {
public:
  ElementInstance() = delete;
  ElementInstance(const uint64_t Offset, const ValType &EType,
                  Span<const RefVariant> Init) noexcept
      : Off(Offset), Type(EType), Refs(Init.begin(), Init.end()) {
    assuming(Type.isRefType());
  }

  ~ElementInstance() noexcept {
    if (Allocator) {
      Allocator->removeElem(*this);
    }
  }

  // Rooted by address: a copy/move would leave the new object unregistered and
  // its destructor's removeElem pointing at an address never stored.
  ElementInstance(const ElementInstance &) = delete;
  ElementInstance(ElementInstance &&) = delete;
  ElementInstance &operator=(const ElementInstance &) = delete;
  ElementInstance &operator=(ElementInstance &&) = delete;

  /// Register this element instance with the GC allocator so its references are
  /// scanned as roots (a passive segment can hold the only reference to a
  /// struct/array created in its init expression).
  void setAllocator(GC::Allocator &A) noexcept {
    assuming(Allocator == nullptr);
    Allocator = &A;
    Allocator->addElem(*this);
  }

  /// Get offset in element instance.
  uint64_t getOffset() const noexcept { return Off; }

  /// Get reference type of element instance.
  const ValType &getRefType() const noexcept { return Type; }

  /// Get reference lists in element instance.
  Span<const RefVariant> getRefs() const noexcept { return Refs; }

  /// Get the slice Refs[Offset : Offset + Length - 1].
  Expect<Span<const RefVariant>> getRefs(uint32_t Offset,
                                         uint32_t Length) const noexcept {
    // Check the accessing boundary. Compare without computing Offset + Length,
    // which would overflow uint32_t and could spuriously pass the check.
    if (Offset > Refs.size() || Length > Refs.size() - Offset) {
      spdlog::error(ErrCode::Value::TableOutOfBounds);
      spdlog::error(ErrInfo::InfoBoundary(Offset, Length, Refs.size()));
      return Unexpect(ErrCode::Value::TableOutOfBounds);
    }
    if (Length == 0) {
      // Empty result; avoid forming data() + Offset on a possibly-null span
      // base (e.g. an empty element segment / zero-length table.init).
      return Span<const RefVariant>{};
    }
    return Span<const RefVariant>(Refs).subspan(Offset, Length);
  }

  /// Clear references in element instance (elem.drop). No ElemMutex needed: the
  /// root scan reads Refs under that lock, but clear() does not reallocate (it
  /// keeps the buffer), so a concurrent scan reads either the determinate old
  /// ref bits (conservative over-retention, safe) or size 0 -- the same benign
  /// race in-place table writes rely on. Only reallocation needs the lock.
  void clear() { Refs.clear(); }

private:
  friend class GC::Allocator;
  void clearAllocator(GC::Allocator &A) noexcept {
    if (Allocator == &A) {
      Allocator = nullptr;
    }
  }

  /// \name Data of element instance.
  /// @{
  GC::Allocator *Allocator = nullptr;
  const uint64_t Off;
  const ValType Type;
  std::vector<RefVariant> Refs;
  /// @}
};

} // namespace Instance
} // namespace Runtime
} // namespace WasmEdge
