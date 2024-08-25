// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/system/fault.h - Memory and arithmetic exception ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the software exception handler for various operating
/// system.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include <csetjmp>

namespace WasmEdge {

class Fault {
public:
  Fault();

  ~Fault() noexcept;

  [[noreturn]] static void emitFault(ErrCode Error);

  std::jmp_buf &buffer() noexcept { return Buffer; }

private:
  Fault *Prev = nullptr;
  std::jmp_buf Buffer;
};

} // namespace WasmEdge

#define PREPARE_FAULT(f) (static_cast<uint32_t>(setjmp((f).buffer())))
