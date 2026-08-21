// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: Copyright The WasmEdge Authors

//===-- wasmedge/loader/serialize.h - Serializer class definition ---------===//
//
// Part of the WasmEdge Project.
//
//===----------------------------------------------------------------------===//
///
/// \file
/// This file contains the declaration of the Serializer class, which serializes
/// a validated AST Module into its canonical encoding.
///
//===----------------------------------------------------------------------===//
#pragma once

#include "ast/module.h"
#include "common/errcode.h"
#include "loader/serialize_writer.h"

#include <vector>

namespace WasmEdge {
namespace Loader {

class Serializer {
public:
  Serializer() noexcept = default;
  ~Serializer() noexcept = default;

  /// Serialize a WASM module.
  Expect<std::vector<uint8_t>>
  serializeModule(const AST::Module &Mod) const noexcept;

  /// \name Serialize functions for WASM sections.
  /// @{
  void serializeSection(const AST::CustomSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::TypeSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::ImportSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::FunctionSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::TableSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::MemorySection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::GlobalSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::ExportSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::StartSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::ElementSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::CodeSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::DataSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::DataCountSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSection(const AST::TagSection &Sec,
                        std::vector<uint8_t> &OutVec) const noexcept;
  /// @}

  /// Exposed for the coredump writer, which encodes its own payload.
  void serializeU32(uint32_t Num, std::vector<uint8_t> &OutVec) const noexcept {
    writeU32(Num, OutVec);
  }

private:
  /// \name Serialize functions for the other nodes of AST.
  /// @{
  void serializeSegment(const AST::TableSegment &Seg,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSegment(const AST::GlobalSegment &Seg,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSegment(const AST::ElementSegment &Seg,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSegment(const AST::CodeSegment &Seg,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeSegment(const AST::DataSegment &Seg,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeDesc(const AST::ImportDesc &Desc,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeDesc(const AST::ExportDesc &Desc,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeHeapType(const ValType &Type,
                         std::vector<uint8_t> &OutVec) const noexcept;
  void serializeRefType(const ValType &Type,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeValType(const ValType &Type,
                        std::vector<uint8_t> &OutVec) const noexcept;
  void serializeLimit(const AST::Limit &Lim,
                      std::vector<uint8_t> &OutVec) const noexcept;
  void serializeType(const AST::SubType &SType,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeType(const AST::FunctionType &Type,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeType(const AST::TableType &Type,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeType(const AST::MemoryType &Type,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeType(const AST::GlobalType &Type,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeType(const AST::FieldType &Type,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeType(const AST::TagType &Type,
                     std::vector<uint8_t> &OutVec) const noexcept;
  void serializeExpression(const AST::Expression &Expr,
                           std::vector<uint8_t> &OutVec) const noexcept;
  void serializeInstruction(const AST::Instruction &Instr,
                            std::vector<uint8_t> &OutVec) const noexcept;
  /// @}

  /// \name Helper functions
  /// @{
  template <typename T, typename L>
  void serializeSectionContent(const T &Sec, uint8_t Code,
                               std::vector<uint8_t> &OutVec,
                               L &&Func) const noexcept {
    // Section: section_id + size:u32 + content.
    auto Content = Sec.getContent();
    if (Content.size()) {
      assuming(Content.size() <= UINT32_MAX);
      // Section ID.
      OutVec.push_back(Code);
      auto OrgSize = OutVec.size();
      // Content: vec(T).
      writeU32(static_cast<uint32_t>(Content.size()), OutVec);
      for (const auto &Item : Content) {
        Func(Item, OutVec);
      }
      // Backward insert the section size.
      writeU32(static_cast<uint32_t>(OutVec.size() - OrgSize), OutVec,
               std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
    }
  }
  /// @}
};

} // namespace Loader
} // namespace WasmEdge
