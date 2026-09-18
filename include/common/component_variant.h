// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/common/component_variant.h - Component value variant -----===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the host-side value model of the Component Model: the
/// variant holding one value of any component value type, and the aggregate
/// values it holds by reference.
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

/// Forward declaration of the aggregate component value container.
/// Defined after ComponentValVariant so members can hold ComponentValVariant.
struct ValComp;

using ComponentValVariant = std::variant<
    // Primitive types in the Component Model value model, held in typed arms
    // so aggregate-internal and top-level primitives share one representation.
    uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t,
    float, double, bool, std::string,
    // Aggregate types, held via shared_ptr because the variant is copied
    // often and a ValComp recursively holds further component values.
    std::shared_ptr<ValComp>>;

// Per-aggregate value structs. Labels are kept on the value side for
// round-trip debugging; the DefValType stays the source of truth.
struct RecordVal {
  std::vector<std::pair<std::string, ComponentValVariant>> Fields;
};
struct TupleVal {
  std::vector<ComponentValVariant> Values;
};
struct VariantVal {
  uint32_t Case = 0;
  std::optional<ComponentValVariant> Payload;
  // Case label recorded by the value decoder; a host-supplied value may
  // carry the label alone, which the lowering resolves against the type.
  std::string Label{};
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
  // Set-label form recorded by the value decoder; a host-supplied value may
  // carry the labels alone, which the lowering resolves against the type.
  std::vector<std::string> SetLabels{};
};
struct EnumVal {
  uint32_t Case = 0;
  // Case label recorded like VariantVal::Label.
  std::string Label{};
};
struct OwnVal {
  // The host side carries the resource representation, which the memory64
  // proposal widens to 64 bits.
  uint64_t Handle;
};
struct BorrowVal {
  uint64_t Handle;
};
// An error-context value. Only the debug message is observable, so lifting
// copies it out of the handles table rather than sharing the entry.
struct ErrorContextVal {
  std::string Message;
};
struct StreamFutureVal {
  // A transferred stream/future end: the shared rendezvous object,
  // type-erased to keep the value layer free of runtime headers.
  std::shared_ptr<void> Shared;
  bool IsStream = true;
};

struct ValComp {
  std::variant<RecordVal, TupleVal, VariantVal, ListVal, OptionVal, ResultVal,
               FlagsVal, EnumVal, OwnVal, BorrowVal, StreamFutureVal,
               ErrorContextVal>
      V;
};

/// Wrap an aggregate value in a heap-allocated ValComp and lift it into a
/// ComponentValVariant. Saves the make_shared + V= + variant-wrap dance at the
/// many lift/load call sites.
template <typename T> inline ComponentValVariant makeComponentVal(T &&Inner) {
  auto Comp = std::make_shared<ValComp>();
  Comp->V = std::forward<T>(Inner);
  return ComponentValVariant{std::move(Comp)};
}

/// The aggregate of alternative T held by a component value. The caller
/// establishes the alternative first, as checkValue does for host values.
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
