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
    // Primitive types in the Component Model value model. Held in typed arms
    // directly (no ValVariant wrapping) so that aggregate-internal primitives
    // and top-level primitives share a single representation. See
    // CanonicalABI.md L2920+ — component values are the spec's typed Python
    // values; core wasm ValVariants belong to the orthogonal Core layer and
    // never appear inside a component value.
    uint8_t, uint16_t, uint32_t, uint64_t, int8_t, int16_t, int32_t, int64_t,
    float, double, bool, std::string,
    // Aggregate types (record/variant/list/tuple/option/result/flags/enum/own/
    // borrow). Held via shared_ptr because the variant is frequently copied and
    // the contained ValComp recursively holds further ComponentValVariants.
    std::shared_ptr<ValComp>>;

// Per-aggregate value structs. Labels for records/variants/flags/enums are
// retained on the value side for easier round-trip debugging — the canonical
// source of truth remains the DefValType.
struct RecordVal {
  std::vector<std::pair<std::string, ComponentValVariant>> Fields;
};
struct TupleVal {
  std::vector<ComponentValVariant> Values;
};
struct VariantVal {
  uint32_t Case = 0;
  std::optional<ComponentValVariant> Payload;
  // Case label recorded by the value decoder. TODO: resolve host-supplied
  // values by label against the declared type at the lowering site.
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
  // Set-label form recorded by the value decoder. TODO: resolve the bits
  // against the declared flag labels at the lowering site.
  std::vector<std::string> SetLabels{};
};
struct EnumVal {
  uint32_t Case = 0;
  // Case label recorded like VariantVal::Label.
  std::string Label{};
};
struct OwnVal {
  uint32_t Handle;
};
struct BorrowVal {
  uint32_t Handle;
};

struct ValComp {
  std::variant<RecordVal, TupleVal, VariantVal, ListVal, OptionVal, ResultVal,
               FlagsVal, EnumVal, OwnVal, BorrowVal>
      V;
};

/// Wrap an aggregate value in a heap-allocated ValComp and lift it into a
/// ComponentValVariant. Saves the make_shared + V= + variant-wrap dance at the
/// many lift/load call sites.
template <typename T> inline ComponentValVariant makeComponentVal(T &&Inner) {
  auto VC = std::make_shared<ValComp>();
  VC->V = std::forward<T>(Inner);
  return ComponentValVariant{std::move(VC)};
}

} // namespace WasmEdge
