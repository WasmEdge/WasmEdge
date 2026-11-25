// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/loader/name_section.h - Name section reader --------------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the readers of the subsections of the name custom
/// section. The callers choose how to report and recover from errors.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "common/errcode.h"
#include "common/span.h"
#include "common/types.h"
#include "loader/filemgr.h"

#include <cstdint>
#include <string>
#include <utility>

namespace WasmEdge {
namespace Loader {

/// The ID of the function name subsection.
inline constexpr Byte FunctionNameSubsection = 1;
/// The ID of the global name subsection.
inline constexpr Byte GlobalNameSubsection = 7;

/// Calls Visit with the ID and the content of each subsection of a name
/// section, in order. Returns false at the first malformed subsection
/// header. The subsections before that header were visited. The format
/// requires unique subsections in increasing ID order. This reader does not
/// check that, so the caller chooses how to recover from a subsection that
/// repeats or is out of order.
template <typename VisitorT>
bool forEachNameSubsection(Span<const Byte> Data, VisitorT &&Visit) {
  FileMgr Mgr;
  if (!Mgr.setCode(Data)) {
    return false;
  }
  while (Mgr.getRemainSize() > 0) {
    const auto Id = Mgr.readByte();
    const auto Size = Mgr.readU32();
    if (!Id || !Size || *Size > Mgr.getRemainSize()) {
      return false;
    }
    const auto Start = Mgr.getOffset();
    Visit(*Id, Data.subspan(Start, *Size));
    Mgr.seek(Start + *Size);
  }
  return true;
}

/// Reads a name map subsection: a count of index and name pairs. Calls
/// Visit with each pair in order, so a caller can keep the pairs before an
/// error. A subsection with bytes after the last pair is malformed. The
/// format requires unique indices in increasing order. This reader does not
/// check that, so the caller chooses how to recover from an index that
/// repeats or is out of order.
template <typename VisitorT>
Expect<void> readNameMap(Span<const Byte> Data, VisitorT &&Visit) {
  FileMgr Mgr;
  EXPECTED_TRY(Mgr.setCode(Data));
  EXPECTED_TRY(uint32_t Count, Mgr.readU32());
  for (uint32_t I = 0; I < Count; ++I) {
    EXPECTED_TRY(uint32_t Index, Mgr.readU32());
    EXPECTED_TRY(std::string Name, Mgr.readName());
    Visit(Index, std::move(Name));
  }
  if (Mgr.getRemainSize() != 0) {
    return Unexpect(ErrCode::Value::SectionSizeMismatch);
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
