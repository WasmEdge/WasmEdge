// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "debuginfo/dwarf_reader.h"

#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>

#include <cstdint>
#include <optional>
#include <string>

namespace WasmEdge::LLVM::DebugInfo {

using llvm::DWARFDie;
namespace dwarf = llvm::dwarf;

std::string declFile(DWARFDie D) noexcept;

unsigned line(DWARFDie D) noexcept;

llvm::StringRef name(DWARFDie D) noexcept;

/// A key that tells DIEs apart across .debug_info and the DWARF 4
/// .debug_types section, which have separate offset spaces.
uint64_t dieKey(DWARFDie D) noexcept;

/// The type of a DIE, also through DW_AT_abstract_origin and
/// DW_AT_specification. The reference resolves in the unit that holds it. A
/// type declaration with DW_AT_signature resolves to the definition in its
/// type unit.
DWARFDie ref(DWARFDie D) noexcept;

/// The declaration of D: the DIE that DW_AT_specification or
/// DW_AT_abstract_origin of D leads to, or D itself.
DWARFDie declarationOf(DWARFDie D) noexcept;

/// True when Param is the parameter that DW_AT_object_pointer of Fn names,
/// directly or through the abstract origin of each.
bool isObjectPointer(DWARFDie Fn, DWARFDie Param) noexcept;

/// Follows typedefs and qualifiers to the type that they name.
DWARFDie stripAliases(DWARFDie T) noexcept;

bool isSignedType(DWARFDie T) noexcept;

uint64_t byteSize(DWARFDie D) noexcept;

/// The byte offset of a member or base in its parent, or std::nullopt if
/// the offset is not a constant or a lone DW_OP_plus_uconst.
std::optional<uint64_t> memberOffset(DWARFDie D) noexcept;

/// The bit offset of a member in its parent: from DW_AT_data_member_location
/// if it exists, else from DW_AT_data_bit_offset, else 0.
std::optional<uint64_t> memberBitOffset(DWARFDie D) noexcept;

/// The byte offset that a DW_AT_data_member_location value gives in a unit
/// of DWARF Version. Before DWARF 4, DW_FORM_data4 and DW_FORM_data8 point
/// to a location list, so they give no offset.
std::optional<uint64_t> memberOffset(const llvm::DWARFFormValue &V,
                                     uint16_t Version) noexcept;

} // namespace WasmEdge::LLVM::DebugInfo
