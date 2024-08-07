// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/common/span.h - Helper template of std::span -------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contents the helper template aliasing of std::span.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "experimental/span.hpp"

namespace WasmEdge {

template <typename T, std::size_t N = cxx20::dynamic_extent>
using Span = cxx20::span<T, N>;

} // namespace WasmEdge
