// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/executable.h - Executable Code definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Executable, which holds interface
/// to executable binary objects.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/defines.h"
#include "common/symbol.h"
#include "common/types.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace WasmEdge {

/// Holder class for library handle
class Executable : public std::enable_shared_from_this<Executable> {
  Executable(const Executable &) = delete;
  Executable &operator=(const Executable &) = delete;
  Executable(Executable &&) = delete;
  Executable &operator=(Executable &&) = delete;

public:
  Executable() noexcept = default;
  virtual ~Executable() noexcept = default;

  /// Function type wrapper for symbols.
  using Wrapper = void(void *ExecCtx, void *Function, const ValVariant *Args,
                       ValVariant *Rets);

  enum class Intrinsics : uint32_t {
    kTrap,
    kCall,
    kCallIndirect,
    kCallRef,
    kRefFunc,
    kStructNew,
    kStructGet,
    kStructSet,
    kArrayNew,
    kArrayNewData,
    kArrayNewElem,
    kArrayGet,
    kArraySet,
    kArrayLen,
    kArrayFill,
    kArrayCopy,
    kArrayInitData,
    kArrayInitElem,
    kTableGet,
    kTableSet,
    kTableInit,
    kElemDrop,
    kTableCopy,
    kTableGrow,
    kTableSize,
    kTableFill,
    kMemGrow,
    kMemSize,
    kMemInit,
    kDataDrop,
    kMemCopy,
    kMemFill,
    kMemAtomicNotify,
    kMemAtomicWait,
    kTableGetFuncSymbol,
    kRefGetFuncSymbol,
    kIntrinsicMax,
  };
  using IntrinsicsTable = void * [uint32_t(Intrinsics::kIntrinsicMax)];

  virtual Symbol<const IntrinsicsTable *> getIntrinsics() noexcept = 0;

  virtual std::vector<Symbol<Wrapper>> getTypes(size_t Size) noexcept = 0;

  virtual std::vector<Symbol<void>> getCodes(size_t Offset,
                                             size_t Size) noexcept = 0;

protected:
  template <typename T> Symbol<T> createSymbol(T *Pointer) const noexcept {
    return Symbol<T>(shared_from_this(), Pointer);
  }
};

} // namespace WasmEdge
