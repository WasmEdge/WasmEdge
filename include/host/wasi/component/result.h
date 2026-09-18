// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/component/result.h - Shared result helpers -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the result helpers of the 0.2 and 0.3 hosts.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/expected.h"
#include "runtime/component/wit.h"

namespace WasmEdge {
namespace Host {
namespace WasiComponent {

/// The error case of a `result<T>` whose error carries no payload.
template <typename T>
Expected<T, Runtime::Component::WitUnit> failUnit() noexcept {
  return Unexpected<Runtime::Component::WitUnit>(Runtime::Component::WitUnit{});
}

} // namespace WasiComponent
} // namespace Host
} // namespace WasmEdge
