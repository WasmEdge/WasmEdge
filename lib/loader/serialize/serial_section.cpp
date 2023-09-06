#include "loader/serialize.h"

namespace WasmEdge {
namespace Loader {

// Serialize custom section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::CustomSection &Sec) {
  // Custom section: 0x00 + size:u32 + name:vec(byte) + content:byte*.
  // Section ID.
  std::vector<uint8_t> OutVec = {0x00U};
  // Name: vec(byte).
  serializeU32(static_cast<uint32_t>(Sec.getName().size()), OutVec);
  OutVec.insert(OutVec.end(), Sec.getName().begin(), Sec.getName().end());
  // Content: byte*.
  OutVec.insert(OutVec.end(), Sec.getContent().begin(), Sec.getContent().end());
  // Backward insert the section size.
  serializeU32(static_cast<uint32_t>(OutVec.size()) - 1, OutVec,
               std::next(OutVec.begin(), 1));
  return OutVec;
}

// Serialize type section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::TypeSection &Sec) {
  // Type section: 0x01 + size:u32 + content:vec(functype).
  return serializeSectionContent(
      Sec, 0x01U, [=](const AST::FunctionType &R, std::vector<uint8_t> &V) {
        return serializeType(R, V);
      });
}

// Serialize import section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::ImportSection &Sec) {
  // Import section: 0x02 + size:u32 + content:vec(importdesc).
  return serializeSectionContent(
      Sec, 0x02U, [=](const AST::ImportDesc &R, std::vector<uint8_t> &V) {
        return serializeDesc(R, V);
      });
}

// Serialize function section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::FunctionSection &Sec) {
  // Function section: 0x03 + size:u32 + content:vec(u32).
  return serializeSectionContent(
      Sec, 0x03U,
      [=](const uint32_t &R, std::vector<uint8_t> &V) -> Expect<void> {
        serializeU32(R, V);
        return {};
      });
}

// Serialize table section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::TableSection &Sec) {
  // Table section: 0x04 + size:u32 + content:vec(tabletype).
  return serializeSectionContent(
      Sec, 0x04U, [=](const AST::TableType &R, std::vector<uint8_t> &V) {
        return serializeType(R, V);
      });
}

// Serialize memory section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::MemorySection &Sec) {
  // Memory section: 0x05 + size:u32 + content:vec(memorytype).
  return serializeSectionContent(
      Sec, 0x05U, [=](const AST::MemoryType &R, std::vector<uint8_t> &V) {
        return serializeType(R, V);
      });
}

// Serialize global section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::GlobalSection &Sec) {
  // Global section: 0x06 + size:u32 + content:vec(globaltype).
  return serializeSectionContent(
      Sec, 0x06U, [=](const AST::GlobalSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize export section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::ExportSection &Sec) {
  // Export section: 0x07 + size:u32 + content:vec(exportdesc).
  return serializeSectionContent(
      Sec, 0x07U, [=](const AST::ExportDesc &R, std::vector<uint8_t> &V) {
        return serializeDesc(R, V);
      });
}

// Serialize start section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::StartSection &Sec) {
  // Start section: 0x08 + size:u32 + idx:u32.
  if (Sec.getContent()) {
    // Section ID.
    std::vector<uint8_t> OutVec = {0x08U};
    // Idx: u32.
    serializeU32(*Sec.getContent(), OutVec);
    // Backward insert the section size.
    serializeU32(static_cast<uint32_t>(OutVec.size()) - 1, OutVec,
                 std::next(OutVec.begin(), 1));
    return OutVec;
  }
  return {};
}

// Serialize element section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::ElementSection &Sec) {
  // Element section: 0x09 + size:u32 + content:vec(elemseg).
  return serializeSectionContent(
      Sec, 0x09U, [=](const AST::ElementSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize code section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::CodeSection &Sec) {
  // Code section: 0x0A + size:u32 + content:vec(codeseg).
  return serializeSectionContent(
      Sec, 0x0AU, [=](const AST::CodeSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize data section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::DataSection &Sec) {
  // Data section: 0x0B + size:u32 + content:vec(dataseg).
  return serializeSectionContent(
      Sec, 0x0BU, [=](const AST::DataSegment &R, std::vector<uint8_t> &V) {
        return serializeSegment(R, V);
      });
}

// Serialize datacount section. See "include/loader/serialize.h".
Expect<std::vector<uint8_t>>
Serializer::serializeSection(const AST::DataCountSection &Sec) {
  // Datacount section: 0x0C + size:u32 + idx:u32.
  if (Sec.getContent()) {
    // Section ID.
    std::vector<uint8_t> OutVec = {0x0CU};
    // Idx: u32.
    serializeU32(*Sec.getContent(), OutVec);
    // Backward insert the section size.
    serializeU32(static_cast<uint32_t>(OutVec.size()) - 1, OutVec,
                 std::next(OutVec.begin(), 1));
    return OutVec;
  }
  return {};
}

} // namespace Loader
} // namespace WasmEdge
