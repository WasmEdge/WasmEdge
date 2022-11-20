#include "loader/serialize.h"
#include "ast/module.h"
#include "ast/type.h"
#include "loader/loader.h"

#include <algorithm>
#include <functional>

namespace WasmEdge {
namespace Serialize {

// Helper functions
template <typename T, typename F>
size_t encodeLEB128Implementation(T Num, F Func) {
  // unsigned LEB128 encoding
  size_t Val = 0;
  do {
    unsigned int X = Num & 0b01111111;
    Num >>= 7;
    if (Num)
      X |= 0b10000000;
    Val++;
    Func(X);
  } while (Num);

  return Val;
}

template <typename T> size_t leb128Len(T Num) {
  return encodeLEB128Implementation(std::move(Num), [](auto &&) {});
}

template <typename T, typename OutIterator>
OutIterator encodeLEB128(T Num, OutIterator OutItr) {
  encodeLEB128Implementation(std::move(Num),
                             [&](uint8_t Val) { *OutItr++ = Val; });
  return OutItr;
}

template <typename RangeIterator, typename OutIterator>
OutIterator encodeRange(RangeIterator F, RangeIterator L, OutIterator OutItr,
                        int encodeRangeTypeFlag) {
  using Val = typename std::iterator_traits<RangeIterator>::value_type;
  size_t const NoOfElements = std::distance(F, L);

  if (encodeRangeTypeFlag == 0) {
    // bytes
    auto const *Bytes = reinterpret_cast<uint8_t const *>(std::addressof(*F));
    return std::copy(Bytes, Bytes + NoOfElements * sizeof(Val), OutItr);

  } else if (encodeRangeTypeFlag == 1) {
    // size:u32, bytes
    auto const *Bytes = reinterpret_cast<uint8_t const *>(std::addressof(*F));
    return std::copy(Bytes, Bytes + NoOfElements * sizeof(Val),
                     encodeLEB128(NoOfElements, OutItr));
  } else {
    // size:u32, bytes
    auto const *Bytes = reinterpret_cast<uint32_t const *>(std::addressof(*F));
    return std::copy(Bytes, Bytes + NoOfElements,
                     encodeLEB128(NoOfElements, OutItr));
  }
}

template <typename R, typename OutIterator>
OutIterator encodeRange(R const &Range, OutIterator outItr,
                        int encodeRangeTypeFlag) {
  return encodeRange(std::begin(Range), std::end(Range), outItr,
                     encodeRangeTypeFlag);
}

template <typename R> size_t rangeLen(R const &Range, int RangeLenTypeFlag) {
  using Val = decltype(*std::begin(Range));
  size_t NoOfElements = Range.size();
  if (RangeLenTypeFlag == 1 || RangeLenTypeFlag == 0) {
    return leb128Len(NoOfElements) + NoOfElements * sizeof(Val);

  } else {
    return leb128Len(NoOfElements) + NoOfElements;
  }
}

std::vector<uint8_t> encodeU32(uint32_t Num) {
  std::vector<uint8_t> Encoded;
  // unsigned LEB128 encoding
  do {
    auto x = Num & 0b01111111;
    Num >>= 7;
    if (Num)
      x |= 0b10000000;
    Encoded.push_back(x);
  } while (Num);

  return Encoded;
}

template <typename T, typename B, typename L>
std::vector<uint8_t> serializeSectionContent(T Sec, B Byte, L Func) {
  if (Sec.getContent().size()) {

    std::vector<uint8_t> SerializeSection;
    std::vector<uint8_t> Content;

    for (auto &ContentItemType : Sec.getContent()) {
      auto SerializedItemType = Func(ContentItemType);
      Content.insert(Content.end(), SerializedItemType.begin(),
                     SerializedItemType.end());
    }
    std::vector<uint8_t> ContentVectorSize = encodeU32(Sec.getContent().size());
    std::vector<uint8_t> ContentSize =
        encodeU32(Content.size() + ContentVectorSize.size());
    // one-byte section id
    SerializeSection.push_back(Byte);
    // U32 size of the contents in bytes
    SerializeSection.insert(SerializeSection.end(), ContentSize.begin(),
                            ContentSize.end());
    SerializeSection.insert(SerializeSection.end(), ContentVectorSize.begin(),
                            ContentVectorSize.end());
    SerializeSection.insert(SerializeSection.end(), Content.begin(),
                            Content.end());
    return SerializeSection;
  }
  return {};
}

// serialize Module
// std::vector<uint8_t> Serialize::serializeModule(const AST::Module &Mod) {

//   std::vector<uint8_t> serializedModule;
//   std::vector<uint8_t> serializedCustom =
//       serializeCustomSection(Mod.getCustomSections());
//   serializedModule.insert(serializedModule.end(), serializedCustom.begin(),
//                           serializedCustom.end());

//   std::vector<uint8_t> serializedType =
//       serializeTypeSection(Mod.getTypeSection());
//   serializedModule.insert(serializedModule.end(), serializedType.begin(),
//                           serializedType.end());

//   std::vector<uint8_t> serializedImport =
//       serializeImportSection(Mod.getImportSection());
//   serializedModule.insert(serializedModule.end(), serializedImport.begin(),
//                           serializedImport.end());

//   std::vector<uint8_t> serializedFunction =
//       serializeFunctionSection(Mod.getFunctionSection());
//   serializedModule.insert(serializedModule.end(), serializedFunction.begin(),
//                           serializedFunction.end());

//   std::vector<uint8_t> serializedTable =
//       serializeTableSection(Mod.getTableSection());
//   serializedModule.insert(serializedModule.end(), serializedTable.begin(),
//                           serializedTable.end());

//   std::vector<uint8_t> serializedMemory =
//       serializeMemorySection(Mod.getMemorySection());
//   serializedModule.insert(serializedModule.end(), serializedMemory.begin(),
//                           serializedMemory.end());

//   std::vector<uint8_t> serializedGlobal =
//       serializeGlobalSection(Mod.getGlobalSection());
//   serializedModule.insert(serializedModule.end(), serializedGlobal.begin(),
//                           serializedGlobal.end());

//   std::vector<uint8_t> serializedExport =
//       serializeExportSection(Mod.getExportSection());
//   serializedModule.insert(serializedModule.end(), serializedExport.begin(),
//                           serializedExport.end());

//   std::vector<uint8_t> serializedStart =
//       serializeStartSection(Mod.getStartSection());
//   serializedModule.insert(serializedModule.end(), serializedStart.begin(),
//                           serializedStart.end());

//   std::vector<uint8_t> serializedElement =
//       serializeElementSection(Mod.getElementSection());
//   serializedModule.insert(serializedModule.end(), serializedElement.begin(),
//                           serializedElement.end());

//   std::vector<uint8_t> serializedCode =
//       serializeCodeSection(Mod.getCodeSection());
//   serializedModule.insert(serializedModule.end(), serializedCode.begin(),
//                           serializedCode.end());

//   std::vector<uint8_t> serializedData =
//       serializeDataSection(Mod.getDataSection());
//   serializedModule.insert(serializedModule.end(), serializedData.begin(),
//                           serializedData.end());

//   std::vector<uint8_t> serializedDataCount =
//       serializeDataCountSection(Mod.getDataCountSection());
//   serializedModule.insert(serializedModule.end(), serializedDataCount.begin(),
//                           serializedDataCount.end());

//   return serializedModule;
// }

// Serialize Sections
std::vector<uint8_t>
Serialize::serializeCustomSection(AST::CustomSection &CustomSec) {
  std::string_view Name = CustomSec.getName();
  std::vector<uint8_t> Content = CustomSec.getContent();
  auto const ContentSize = rangeLen(Name, 1) + rangeLen(Content, 1);

  std::vector<uint8_t> SerializeSection(1 + leb128Len(ContentSize) +
                                        ContentSize);
  std::vector<uint8_t>::iterator Iterator = SerializeSection.begin();
  *Iterator++ = 0x00U;
  Iterator = encodeLEB128(ContentSize, Iterator);
  // name: vec(byte) = size:u32, bytes
  Iterator = encodeRange(Name, Iterator, 1);
  // content: vec(byte) = size:u32, bytes
  Iterator = encodeRange(Content, Iterator, 1);
  return SerializeSection;
}

std::vector<uint8_t>
Serialize::serializeTypeSection(AST::TypeSection &TypeSec) {

  std::function<std::vector<uint8_t>(AST::FunctionType &)> Func =
      [this](AST::FunctionType &r) { return this->serializeFunctionType(r); };

  return serializeSectionContent(TypeSec, 0x01U, Func);
}

std::vector<uint8_t>
Serialize::serializeImportSection(AST::ImportSection &ImportSec) {

  std::function<std::vector<uint8_t>(AST::ImportDesc &)> Func =
      [this](AST::ImportDesc &r) { return this->serializeImportDesc(r); };

  return serializeSectionContent(ImportSec, 0x02U, Func);
}

std::vector<uint8_t>
Serialize::serializeFunctionSection(AST::FunctionSection &FunctionSec) {
  if (FunctionSec.getContent().size()) {
    std::vector<uint32_t> Section = FunctionSec.getContent();
    auto SectionSize = rangeLen(Section, 3);
    std::vector<uint8_t> SerializeSection(1 + leb128Len(SectionSize) +
                                          SectionSize);
    std::vector<uint8_t>::iterator Iterator = SerializeSection.begin();
    *Iterator++ = 0x03U;
    // U32 size of the contents in bytes
    Iterator = encodeLEB128(SectionSize, Iterator);
    // Connect the section content.
    Iterator = encodeRange(Section, Iterator, 3);
    return SerializeSection;
  } else {
    return {};
  }
}

std::vector<uint8_t>
Serialize::serializeTableSection(AST::TableSection &TabSec) {

  std::function<std::vector<uint8_t>(AST::TableType &)> Func =
      [this](AST::TableType &r) { return this->serializeTableType(r); };

  return serializeSectionContent(TabSec, 0x04U, Func);
}

std::vector<uint8_t>
Serialize::serializeMemorySection(AST::MemorySection &MemSec) {

  std::function<std::vector<uint8_t>(AST::MemoryType &)> Func =
      [this](AST::MemoryType &r) { return this->serializeMemType(r); };

  return serializeSectionContent(MemSec, 0x05U, Func);
}

std::vector<uint8_t>
Serialize::serializeExportSection(AST::ExportSection &ExportSec) {

  std::function<std::vector<uint8_t>(AST::ExportDesc &)> Func =
      [this](AST::ExportDesc &r) { return this->serializeExportDesc(r); };

  return serializeSectionContent(ExportSec, 0x07U, Func);
}

std::vector<uint8_t>
Serialize::serializeStartSection(AST::StartSection &StartSec) {
  if (StartSec.getContent()) {
    std::vector<uint8_t> EncodedSectionSize = encodeU32(*StartSec.getContent());
    std::vector<uint8_t> SerializeSection(
        1 + leb128Len(EncodedSectionSize.size()) + EncodedSectionSize.size());
    std::vector<uint8_t>::iterator Iterator = SerializeSection.begin();
    *Iterator++ = 0x08U;
    // The section size.
    Iterator = encodeLEB128(EncodedSectionSize.size(), Iterator);
    // Connect the section content: bytes
    Iterator = encodeRange(EncodedSectionSize, Iterator, 0);
    return SerializeSection;
  } else {
    return {};
  }
}

std::vector<uint8_t>
Serialize::serializeDataCountSection(AST::DataCountSection &DataCountSec) {
  if (DataCountSec.getContent()) {
    std::vector<uint8_t> EncodedSectionSize =
        encodeU32(*DataCountSec.getContent());
    std::vector<uint8_t> SerializeSection(
        1 + leb128Len(EncodedSectionSize.size()) + EncodedSectionSize.size());
    std::vector<uint8_t>::iterator Iterator = SerializeSection.begin();
    *Iterator++ = 0x12U;
    // The section size.
    Iterator = encodeLEB128(EncodedSectionSize.size(), Iterator);
    // section content: bytes
    Iterator = encodeRange(EncodedSectionSize, Iterator, 0);
    return SerializeSection;
  } else {
    return {};
  }
}

} // namespace Serialize
} // namespace WasmEdge

// https://compiler-explorer.com/z/jEejhGqfh
//
// https://onlinegdb.com/G8hPAzlNQE

// https://onlinegdb.com/gdLD0amq-Z