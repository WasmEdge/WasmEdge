// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/array.h - Array Instance definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the array instance marker type in the reference heap.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/refheap.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {

/// Marker type of a array in a data slot of the reference heap. The fields
/// follow the slot header, and the header holds the canonical type id.
class ArrayInstance {
public:
  ArrayInstance() = delete;
};

} // namespace Instance

template <> struct HeapTraits<Instance::ArrayInstance> {
  static constexpr HeapKind Kind = HeapKind::Array;
};

} // namespace Runtime
} // namespace WasmEdge
