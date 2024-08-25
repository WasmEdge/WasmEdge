// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/po/parser.h - Argument error -----------------------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
#pragma once

#include "experimental/expected.hpp"
#include <string>
#include <string_view>
#include <utility>

namespace WasmEdge {
namespace PO {

enum class ErrCode {
  InvalidArgument,
  OutOfRange,
};

class Error {
public:
  Error(const Error &) = default;
  Error &operator=(const Error &) = default;
  Error(Error &&) noexcept = default;
  Error &operator=(Error &&) noexcept = default;

  Error(ErrCode C, std::string M) noexcept : Code(C), Message(std::move(M)) {}
  ErrCode code() const noexcept { return Code; }
  std::string_view message() const &noexcept { return Message; }
  std::string message() &&noexcept { return std::move(Message); }

private:
  ErrCode Code;
  std::string Message;
};

} // namespace PO
} // namespace WasmEdge
