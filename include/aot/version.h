// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/aot/version.h - version definition ---------------------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the binary version signature of SSVM.
///
//===----------------------------------------------------------------------===//
#pragma once

#include <cstdint>

namespace SSVM {
namespace AOT {

static inline uint32_t kBinaryVersion [[maybe_unused]] = 1;

} // namespace AOT
} // namespace SSVM
