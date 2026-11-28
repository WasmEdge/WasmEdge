// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/common/component_variant.h - Component value variant -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host-side value model of the Component Model.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace WasmEdge {

namespace Runtime::Instance::Component {
class StreamInstance;
} // namespace Runtime::Instance::Component

/// The aggregate component value container, defined below.
struct ValComp;

using ComponentValVariant = std::variant<
    // Primitive values.
    uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t,
    float, double, bool, std::string,
    // The elements of a list of `u8`.
    std::vector<uint8_t>,
    // Aggregate values, shared because a ValComp nests component values.
    std::shared_ptr<ValComp>>;

// Per-aggregate value structs; a field, a case or a flag is its position.
struct RecordVal {
  std::vector<ComponentValVariant> Fields;
};
struct TupleVal {
  std::vector<ComponentValVariant> Values;
};
struct VariantVal {
  uint32_t Case = 0;
  std::optional<ComponentValVariant> Payload;
};
struct ListVal {
  std::vector<ComponentValVariant> Elements;
};
struct OptionVal {
  std::optional<ComponentValVariant> Value;
};
struct ResultVal {
  bool IsOk;
  std::optional<ComponentValVariant> Payload;
};
struct FlagsVal {
  std::vector<bool> Bits;
};
struct EnumVal {
  uint32_t Case = 0;
};
// The code units of the source of a string in transfer: UTF-16, or the
// Latin-1 or UTF-16 a `latin1+utf16` guest chose.
enum class StringHint : uint8_t { UTF16, DynamicLatin1, DynamicUTF16 };
// A string a guest stored as UTF-16 or Latin-1, in transfer from its lift to
// its lower, which allocates by the source. A host has a std::string instead.
struct StringVal {
  std::string Str;
  StringHint Hint;
};
struct OwnVal {
  // The resource representation, 64-bit under memory64.
  uint64_t Handle;
};
struct BorrowVal {
  uint64_t Handle;
};
// An error-context value: only its debug message is observable.
struct ErrorContextVal {
  std::string Message;
};
struct StreamFutureVal {
  // The readable end in transfer, from its lift to its lower. A host body
  // names it by its index in the host table of its root instead.
  Runtime::Instance::Component::StreamInstance *Stream = nullptr;
  uint32_t HandleIdx = 0;
  bool IsStream = true;
};

struct ValComp {
  std::variant<RecordVal, TupleVal, VariantVal, ListVal, OptionVal, ResultVal,
               FlagsVal, EnumVal, StringVal, OwnVal, BorrowVal, StreamFutureVal,
               ErrorContextVal>
      V;
};

/// Wrap an aggregate value into a ComponentValVariant.
template <typename T> inline ComponentValVariant makeComponentVal(T &&Inner) {
  auto Comp = std::make_shared<ValComp>();
  Comp->V = std::forward<T>(Inner);
  return ComponentValVariant{std::move(Comp)};
}

/// The aggregate of alternative T held by a component value; it must hold T.
template <typename T>
inline const T &getComponentVal(const ComponentValVariant &V) noexcept {
  const auto *Comp = std::get_if<std::shared_ptr<ValComp>>(&V);
  assuming(Comp != nullptr && *Comp);
  const auto *Inner = std::get_if<T>(&(*Comp)->V);
  assuming(Inner != nullptr);
  return *Inner;
}

/// True when the component value holds an aggregate of alternative T.
template <typename T>
inline bool isComponentVal(const ComponentValVariant &V) noexcept {
  const auto *Comp = std::get_if<std::shared_ptr<ValComp>>(&V);
  return Comp != nullptr && *Comp && std::holds_alternative<T>((*Comp)->V);
}

} // namespace WasmEdge
