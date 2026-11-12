// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/instance/struct.h - Struct Instance definition ---===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the struct instance marker type in the reference heap.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "runtime/refheap.h"

namespace WasmEdge {
namespace Runtime {
namespace Instance {

/// Marker type of a struct in a data slot of the reference heap. The fields
/// follow the slot header, and the header holds the canonical type id.
class StructInstance {
public:
  StructInstance() = delete;
};

} // namespace Instance

template <> struct HeapTraits<Instance::StructInstance> {
  static constexpr HeapKind Kind = HeapKind::Struct;
};

} // namespace Runtime
} // namespace WasmEdge
