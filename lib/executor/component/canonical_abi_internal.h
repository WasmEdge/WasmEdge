// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

//===-- wasmedge/lib/executor/component/canonical_abi_internal.h ----------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file declares the canonical ABI helpers that the layout, load / store
/// and lift / lower sources share. It is private to lib/executor/component.
//===----------------------------------------------------------------------===//
#pragma once

#include "executor/component/canonical_abi.h"

#include "ast/component/type.h"
#include "common/errcode.h"
#include "common/types.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace WasmEdge {
namespace Executor {
namespace Component {
namespace CanonicalABI {

// The shared upper bound on string and list byte lengths.
constexpr uint32_t kMaxCanonByteLength = (1u << 28) - 1u;

// Layout leaves, defined in component_canon_layout.cpp.

AST::Component::DefValType
mapEntryType(const AST::Component::MapTy &M) noexcept;

Expect<uint32_t> maxCaseAlignment(
    const Context &Cx,
    const std::vector<std::pair<std::string, std::optional<ComponentValType>>>
        &Cases) noexcept;

// Value hygiene and traps, defined in component_canon_loadstore.cpp.

float canonicalizeNaN32(float F) noexcept;

double canonicalizeNaN64(double F) noexcept;

[[nodiscard]] Expect<void> trapMemoryOOB(const std::string_view What,
                                         uint64_t Ptr, uint64_t Len) noexcept;

[[nodiscard]] Expect<void>
trapDataInvalid(const std::string_view Msg,
                ErrCode::Value Code = ErrCode::Value::ComponentTrap) noexcept;

[[nodiscard]] Expect<void> validateUSV(uint32_t I) noexcept;

void assumeValidUSV(uint32_t I) noexcept;

// Representation primitives, defined in component_canon_loadstore.cpp.

Expect<uint64_t> callRealloc(const Context &Cx, uint64_t OldPtr,
                             uint64_t OldSize, uint32_t Align,
                             uint64_t NewSize) noexcept;

Expect<uint64_t> liftOwnHandle(const Context &Cx, uint32_t TypeIdx,
                               uint32_t Idx) noexcept;

Expect<uint64_t> liftBorrowHandle(const Context &Cx, uint32_t TypeIdx,
                                  uint32_t Idx) noexcept;

uint32_t lowerHandle(const Context &Cx, uint32_t TypeIdx, uint64_t Rep,
                     bool Own) noexcept;

Expect<std::shared_ptr<void>> liftCopyEnd(const Context &Cx, bool IsStream,
                                          uint32_t Idx) noexcept;

Expect<uint32_t> lowerCopyEnd(const Context &Cx, bool IsStream,
                              const std::shared_ptr<void> &SharedV) noexcept;

Expect<ComponentValVariant> liftErrorContext(const Context &Cx,
                                             uint32_t Idx) noexcept;

Expect<uint32_t> lowerErrorContext(const Context &Cx,
                                   const ComponentValVariant &V) noexcept;

uint32_t resolveVariantCase(const VariantVal &V,
                            const AST::Component::VariantTy &T) noexcept;

uint32_t resolveEnumCase(const EnumVal &E,
                         const AST::Component::EnumTy &T) noexcept;

uint64_t packFlags(const FlagsVal &F,
                   const AST::Component::FlagsTy &T) noexcept;

Expect<std::string> decodeString(const Context &Cx, uint64_t Begin,
                                 uint64_t TaggedCodeUnits) noexcept;

Expect<std::pair<uint64_t, uint64_t>>
encodeString(const Context &Cx, const std::string &S) noexcept;

Expect<ComponentValVariant>
liftListFromRange(const Context &Cx, uint64_t Begin, uint64_t Length,
                  const ComponentValType &ElemT) noexcept;

Expect<ComponentValVariant>
liftListFromRangeDef(const Context &Cx, uint64_t Begin, uint64_t Length,
                     const AST::Component::DefValType &ElemT) noexcept;

Expect<std::pair<uint64_t, uint64_t>>
storeListWithDefElem(const Context &Cx, const ListVal &Lv,
                     const AST::Component::DefValType &ElemT) noexcept;

/// Read a pointer or length at Ptr, honouring the memory's address width.
Expect<uint64_t> loadPtr(const Context &Cx, uint64_t Ptr) noexcept;

/// Write a pointer or length at Ptr, honouring the memory's address width.
Expect<void> storePtr(const Context &Cx, uint64_t V, uint64_t Ptr) noexcept;

} // namespace CanonicalABI
} // namespace Component
} // namespace Executor
} // namespace WasmEdge
