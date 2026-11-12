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
#include "runtime/instance/tag.h"
#include "runtime/refheap.h"

#include <vector>

namespace WasmEdge {
namespace Runtime {
namespace Instance {
class ExceptionInstance;
} // namespace Instance

template <> struct HeapTraits<Instance::ExceptionInstance> {
  static constexpr HeapKind Kind = HeapKind::Exception;
};

namespace Instance {

class ExceptionInstance {
public:
  ExceptionInstance() = delete;
  ExceptionInstance(const ExceptionInstance &) = delete;
  ExceptionInstance(ExceptionInstance &&) = delete;
  ExceptionInstance &operator=(const ExceptionInstance &) = delete;
  ExceptionInstance &operator=(ExceptionInstance &&) = delete;

  /// Create an exception instance in a slot of the global reference heap.
  static Expect<UniqueHeapPtr<ExceptionInstance>>
  create(TagInstance *Tag, std::vector<ValVariant> &&Payload) noexcept {
    auto *Heap = RefHeap::getGlobal();
    if (Heap == nullptr) {
      return Unexpect(ErrCode::Value::RefHeapFull);
    }
    return Heap->makeUnique<ExceptionInstance>(Tag, std::move(Payload));
  }

private:
  friend class Runtime::RefHeap;

  ExceptionInstance(TagInstance *Tag,
                    std::vector<ValVariant> &&Payload) noexcept
      : TgInst(Tag), Data(std::move(Payload)) {
    assuming(TgInst);
  }

public:
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
