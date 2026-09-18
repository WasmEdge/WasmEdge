// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/host/wasi/p3/future.h - Status futures -------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the status future resolution of the 0.3 hosts.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "executor/component/executor.h"
#include "runtime/component/callingframe.h"
#include "runtime/component/wit.h"

#include <cstdint>
#include <utility>

namespace WasmEdge {
namespace Host {
namespace WasiP3 {

/// Resolve the future Done with Value from a detached host task.
template <typename T>
void resolveLater(Runtime::Component::CallingFrame &Frame, uint32_t Done,
                  T Value) noexcept {
  Frame.getExecutor().spawnStreamWrite(
      Frame, Done, Runtime::Component::Wit<T>::into(std::move(Value)));
}

} // namespace WasiP3
} // namespace Host
} // namespace WasmEdge
