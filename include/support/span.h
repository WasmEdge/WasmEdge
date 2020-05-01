// SPDX-License-Identifier: Apache-2.0
//===-- ssvm/support/span.h - helper class for memory region --------------===//
//
// Part of the SSVM Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the common-use definitions in Compiler.
///
//===----------------------------------------------------------------------===//
#pragma once
#include "experimental/span.hpp"

namespace SSVM {

template <typename T, std::size_t N = std::dynamic_extent>
using Span = std::span<T, N>;

} // namespace SSVM
