// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2022 Second State INC

//===-- wasmedge/common/coredump.h - Executor coredump definition -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the coredump class of runtime.
///
//===----------------------------------------------------------------------===//

#pragma once
#include <iostream>

namespace WasmEdge{
namespace Coredump{

class Coredump {
public:
  Coredump() {
    std::cout<<("Coredump class Constructor called\n");
  }
  ~Coredump() = default;
};

} // namespace Coredump
} // namespace WasmEdge