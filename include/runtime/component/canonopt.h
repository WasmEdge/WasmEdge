// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/runtime/component/canonopt.h -----------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the canonical options of a component model lift or
/// lower.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>

namespace WasmEdge {
namespace Runtime {

namespace Instance {
class ComponentInstance;
class FunctionInstance;
class MemoryInstance;
} // namespace Instance

namespace Component {

/// The `string-encoding` canon option. The host always holds component
/// strings as UTF-8; this selects the guest's linear-memory layout.
enum class StringEncoding : uint8_t { UTF8, UTF16, Latin1UTF16 };

/// The `canonopt` of the Component Model specification, plus the component
/// instance whose type-index space these options are read against. A holder
/// leaves the options its construct has no syntax for at their defaults: a
/// `canon lower` has no post-return, a synchronous lift has no callback.
struct CanonOptions {
  const Instance::ComponentInstance *Inst = nullptr;
  Instance::MemoryInstance *Mem = nullptr;
  Instance::FunctionInstance *Realloc = nullptr;
  Instance::FunctionInstance *PostReturn = nullptr;
  Instance::FunctionInstance *Callback = nullptr;
  /// Guest string encoding; defaults to UTF-8.
  StringEncoding Enc = StringEncoding::UTF8;
  bool Async = false;
};

} // namespace Component
} // namespace Runtime
} // namespace WasmEdge
