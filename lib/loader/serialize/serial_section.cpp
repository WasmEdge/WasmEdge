// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize custom section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::CustomSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Custom section: 0x00 + size:u32 + name:vec(byte) + content:byte*.
  // Section ID.
  OutVec.push_back(0x00U);
  auto OrgSize = OutVec.size();
  // Name: vec(byte).
  serializeU32(static_cast<uint32_t>(Sec.getName().size()), OutVec);
  OutVec.insert(OutVec.end(), Sec.getName().begin(), Sec.getName().end());
  // Content: byte*.
  OutVec.insert(OutVec.end(), Sec.getContent().begin(), Sec.getContent().end());
  // Backward insert the section size.
  serializeU32(static_cast<uint32_t>(OutVec.size() - OrgSize), OutVec,
               std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
  return {};
}

// Serialize type section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::TypeSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Type section: 0x01 + size:u32 + content:vec(rectype).
  auto STypes = Sec.getContent();

  // record the recursive type vector size.
  if (STypes.size() > 0) {
    // Section ID.
    OutVec.push_back(0x01U);
    auto OrgSize = OutVec.size();
    uint32_t RecCnt = 0;
    // Content: vec(rectype).
    for (uint32_t I = 0; I < STypes.size(); I++) {
      auto RecInfo = STypes[I].getRecursiveInfo();
      if (!RecInfo.has_value()) {
        RecCnt++;
      } else if (RecInfo->Index == 0) {
        // First element of recursive type.
        if (!Conf.hasProposal(Proposal::GC)) {
          return logNeedProposal(ErrCode::Value::MalformedValType, Proposal::GC,
                                 ASTNodeAttr::Sec_Type);
        }
        OutVec.push_back(static_cast<uint8_t>(TypeCode::Rec));
        serializeU32(RecInfo->RecTypeSize, OutVec);
        RecCnt++;
      }
      if (auto Res = serializeType(STypes[I], OutVec); unlikely(!Res)) {
        spdlog::error(ASTNodeAttr::Sec_Type);
        return Unexpect(Res);
      }
    }
    // Backward insert the recursive type vector size.
    serializeU32(RecCnt, OutVec,
                 std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
    // Backward insert the section size.
    serializeU32(static_cast<uint32_t>(OutVec.size() - OrgSize), OutVec,
                 std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
  }
  return {};
}

// Serialize import section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::ImportSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Import section: 0x02 + size:u32 + content:vec(importdesc).
  return serializeSectionContent(
      Sec, 0x02U, OutVec,
      [=](const AST::ImportDesc &R, std::vector<uint8_t> &V) {
        return serializeDesc(R, V);
      });
}

// Serialize function section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::FunctionSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Function section: 0x03 + size:u32 + content:vec(u32).
  return serializeSectionContent(
      Sec, 0x03U, OutVec,
      [=](const uint32_t &R, std::vector<uint8_t> &V) -> Expect<void> {
        serializeU32(R, V);
        return {};
      });
}

// Serialize table section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::TableSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Table section: 0x04 + size:u32 + content:vec(tabletype).
  return serializeSectionContent(
      Sec, 0x04U, OutVec,
      [=](const AST::TableSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize memory section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::MemorySection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Memory section: 0x05 + size:u32 + content:vec(memorytype).
  return serializeSectionContent(
      Sec, 0x05U, OutVec,
      [=](const AST::MemoryType &R, std::vector<uint8_t> &V) {
        return serializeType(R, V);
      });
}

// Serialize global section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::GlobalSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Global section: 0x06 + size:u32 + content:vec(globaltype).
  return serializeSectionContent(
      Sec, 0x06U, OutVec,
      [=](const AST::GlobalSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize export section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::ExportSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Export section: 0x07 + size:u32 + content:vec(exportdesc).
  return serializeSectionContent(
      Sec, 0x07U, OutVec,
      [=](const AST::ExportDesc &R, std::vector<uint8_t> &V) {
        return serializeDesc(R, V);
      });
}

// Serialize start section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::StartSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Start section: 0x08 + size:u32 + idx:u32.
  if (Sec.getContent()) {
    // Section ID.
    OutVec.push_back(0x08U);
    auto OrgSize = OutVec.size();
    // Idx: u32.
    serializeU32(*Sec.getContent(), OutVec);
    // Backward insert the section size.
    serializeU32(static_cast<uint32_t>(OutVec.size() - OrgSize), OutVec,
                 std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
  }
  return {};
}

// Serialize element section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::ElementSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Element section: 0x09 + size:u32 + content:vec(elemseg).
  return serializeSectionContent(
      Sec, 0x09U, OutVec,
      [=](const AST::ElementSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize code section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::CodeSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Code section: 0x0A + size:u32 + content:vec(codeseg).
  return serializeSectionContent(
      Sec, 0x0AU, OutVec,
      [=](const AST::CodeSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize data section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::DataSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Data section: 0x0B + size:u32 + content:vec(dataseg).
  return serializeSectionContent(
      Sec, 0x0BU, OutVec,
      [=](const AST::DataSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize datacount section. See "include/loader/serialize.h".
Expect<void>
Serializer::serializeSection(const AST::DataCountSection &Sec,
                             std::vector<uint8_t> &OutVec) const noexcept {
  // Datacount section: 0x0C + size:u32 + idx:u32.
  if (Sec.getContent()) {
    // This section is for BulkMemoryOperations or ReferenceTypes proposal.
    if (!Conf.hasProposal(Proposal::BulkMemoryOperations) &&
        !Conf.hasProposal(Proposal::ReferenceTypes)) {
      return logNeedProposal(ErrCode::Value::MalformedSection,
                             Proposal::BulkMemoryOperations,
                             ASTNodeAttr::Sec_DataCount);
    }
    // Section ID.
    OutVec.push_back(0x0CU);
    auto OrgSize = OutVec.size();
    // Idx: u32.
    serializeU32(*Sec.getContent(), OutVec);
    // Backward insert the section size.
    serializeU32(static_cast<uint32_t>(OutVec.size() - OrgSize), OutVec,
                 std::next(OutVec.begin(), static_cast<ptrdiff_t>(OrgSize)));
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
