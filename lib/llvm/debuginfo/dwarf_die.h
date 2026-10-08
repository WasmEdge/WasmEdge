// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

#pragma once

#include "debuginfo/dwarf_reader.h"

#include <llvm/ADT/SmallVector.h>
#include <llvm/BinaryFormat/Dwarf.h>
#include <llvm/DebugInfo/DWARF/DWARFDebugLine.h>
#include <llvm/DebugInfo/DWARF/DWARFDie.h>
#include <llvm/DebugInfo/DWARF/DWARFUnit.h>

#include <cstdint>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace WasmEdge::LLVM::DebugInfo {

using llvm::DWARFDie;
namespace dwarf = llvm::dwarf;

inline constexpr uint32_t MaxDepth = 128;

std::string lineTableFile(const llvm::DWARFDebugLine::LineTable *LT,
                          llvm::DWARFUnit *U, uint64_t Index) noexcept;

std::string declFile(DWARFDie D) noexcept;

std::string callFile(DWARFDie D) noexcept;

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

/// The inlined subroutines at Address, innermost first, and then the
/// subprogram, as DWARFUnit::getInlinedChainForAddress gives them. It reads
/// only U, never a split DWARF file that U names.
void inlinedChain(llvm::DWARFUnit &U, uint64_t Address,
                  llvm::SmallVectorImpl<DWARFDie> &Chain) noexcept;

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

/// The lexical blocks of a function by address. Each block belongs to its
/// frame, the subprogram or the inlined subroutine around it.
class ScopeIndex {
public:
  /// Adds an address range of a lexical block that is Depth levels deep.
  void add(DWARFDie Frame, DWARFDie Block, uint32_t Depth, uint64_t Low,
           uint64_t High);

  /// Builds the search index after the last add, and frees the added ranges.
  void build() noexcept;

  /// The innermost lexical block of the frame at the address, else the frame.
  DWARFDie innermost(DWARFDie Frame, uint64_t Addr) const noexcept;

private:
  struct Range {
    uint64_t Low;
    uint64_t High;
    uint32_t Depth;
    DWARFDie Block;
  };
  std::unordered_map<uint64_t, std::vector<Range>> Ranges;
  std::unordered_map<uint64_t, std::vector<AddressSpan<DWARFDie>>> Spans;
};

} // namespace WasmEdge::LLVM::DebugInfo
