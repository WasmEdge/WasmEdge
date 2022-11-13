#include "ast/module.h"
#include "loader/loader.h"

#include <functional>

namespace WasmEdge {
namespace Serialize {

std::vector<uint8_t> Serialize::serializeModule(const AST::Module &Mod) {

  std::vector<uint8_t> serializedModule;
  std::vector<uint8_t> serializedCustom =
      serializeCustomSection(Mod.getCustomSections());
  serializedModule.insert(serializedModule.end(), serializedCustom.begin(),
                          serializedCustom.end());

  std::vector<uint8_t> serializedType =
      serializeTypeSection(Mod.getTypeSection());
  serializedModule.insert(serializedModule.end(), serializedType.begin(),
                          serializedType.end());

  std::vector<uint8_t> serializedImport =
      serializeImportSection(Mod.getImportSection());
  serializedModule.insert(serializedModule.end(), serializedImport.begin(),
                          serializedImport.end());

  std::vector<uint8_t> serializedFunction =
      serializeFunctionSection(Mod.getFunctionSection());
  serializedModule.insert(serializedModule.end(), serializedFunction.begin(),
                          serializedFunction.end());

  std::vector<uint8_t> serializedTable =
      serializeTableSection(Mod.getTableSection());
  serializedModule.insert(serializedModule.end(), serializedTable.begin(),
                          serializedTable.end());

  std::vector<uint8_t> serializedMemory =
      serializeMemorySection(Mod.getMemorySection());
  serializedModule.insert(serializedModule.end(), serializedMemory.begin(),
                          serializedMemory.end());

  std::vector<uint8_t> serializedGlobal =
      serializeGlobalSection(Mod.getGlobalSection());
  serializedModule.insert(serializedModule.end(), serializedGlobal.begin(),
                          serializedGlobal.end());

  std::vector<uint8_t> serializedExport =
      serializeExportSection(Mod.getExportSection());
  serializedModule.insert(serializedModule.end(), serializedExport.begin(),
                          serializedExport.end());

  std::vector<uint8_t> serializedStart =
      serializeStartSection(Mod.getStartSection());
  serializedModule.insert(serializedModule.end(), serializedStart.begin(),
                          serializedStart.end());

  std::vector<uint8_t> serializedElement =
      serializeElementSection(Mod.getElementSection());
  serializedModule.insert(serializedModule.end(), serializedElement.begin(),
                          serializedElement.end());

  std::vector<uint8_t> serializedCode =
      serializeCodeSection(Mod.getCodeSection());
  serializedModule.insert(serializedModule.end(), serializedCode.begin(),
                          serializedCode.end());

  std::vector<uint8_t> serializedData =
      serializeDataSection(Mod.getDataSection());
  serializedModule.insert(serializedModule.end(), serializedData.begin(),
                          serializedData.end());

  std::vector<uint8_t> serializedDataCount =
      serializeDataCountSection(Mod.getDataCountSection());
  serializedModule.insert(serializedModule.end(), serializedDataCount.begin(),
                          serializedDataCount.end());

  return serializedModule;
}

// Helper functions

template <typename T, typename F>
size_t encodeLEB128Implementation(T Num, F Func) {
  // unsigned LEB128 encoding
  size_t Val = 0;
  do {
    unsigned int x = Num & 0b01111111;
    Num >>= 7;
    if (Num)
      x |= 0b10000000;
    Val++;
    Func(x);
  } while (Num);

  return Val;
}

template <typename T> size_t leb128Len(T Num) {
  return encodeLEB128Implementation(std::move(Num), [](auto &&) {});
}

template <typename T, typename Out> Out encodeLEB128(T Num, Out outItr) {
  encodeLEB128Implementation(std::move(Num),
                             [&](uint8_t Val) { *outItr++ = Val; });
  return outItr;
}

template <std::contiguous_iterator It, typename Out>
Out encodeRange(It f, It l, Out outItr, int Flag) {
  using Val = typename std::iterator_traits<It>::value_type;
  size_t const NoOfElements = std::distance(f, l);
  auto const *bytes = reinterpret_cast<uint8_t const *>(std::addressof(*f));
  if (Flag) {
    // size:u32, bytes
    return std::copy(bytes, bytes + NoOfElements * sizeof(Val),
                     encodeLEB128(NoOfElements, outItr));
  } else {
    // bytes
    return std::copy(bytes, bytes + NoOfElements * sizeof(Val), outItr);
  }
}
template <typename R, typename Out>
Out encodeRange(R const &Range, Out outItr, int Flag) {
  return encodeRange(std::begin(Range), std::end(Range), outItr, Flag);
}

template <typename R> size_t rangeLen(R const &Range) {
  using Val = decltype(*std::begin(Range));
  size_t NoOfElements = std::size(Range);
  return leb128Len(NoOfElements) + NoOfElements * sizeof(Val);
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
std::vector<uint8_t> serializeSectionContent(T &Sec, B Byte, L Func) {
  if (Sec.getContent().size()) {

    std::vector<uint8_t> Result;
    std::vector<uint8_t> Content;

    for (auto &type : Sec.getContent()) {
      auto serializedType = Func(type);
      Content.insert(Content.end(), serializedType.begin(),
                     serializedType.end());
    }
    std::vector<uint8_t> ContentVectorSize = encodeU32(Sec.getContent().size());
    std::vector<uint8_t> ContentSize =
        encodeU32(Content.size() + ContentVectorSize.size());
    // one-byte section id
    Result.push_back(Byte);
    // U32 size of the contents in bytes
    Result.insert(Result.end(), ContentSize.begin(), ContentSize.end());
    Result.insert(Result.end(), ContentVectorSize.begin(),
                  ContentVectorSize.end());
    Result.insert(Result.end(), Content.begin(), Content.end());
    return Result;
  }
  return {};
}

// Sections
std::vector<uint8_t>
Serialize::serializeCustomSection(AST::CustomSection &CustomSecs) {
  auto const &name = CustomSecs.getName();
  auto const &content = CustomSecs.getContent();
  auto const ContentSize = rangeLen(name) + rangeLen(content);

  std::vector<uint8_t> CustomSecsSerialized(1 + leb128Len(ContentSize) +
                                            ContentSize);
  auto out = CustomSecsSerialized.begin();
  *out++ = 0x00U;
  out = encodeLEB128(ContentSize, out);
  // name: vec(byte) = size:u32, bytes
  out = encodeRange(name, out, 1);
  // content: vec(byte) = size:u32, bytes
  out = encodeRange(content, out, 1);
  return CustomSecsSerialized;
}

std::vector<uint8_t>
Serialize::serializeTypeSection(AST::TypeSection &TypeSec) {

  std::function<std::vector<uint8_t>(AST::FunctionType)> Func =
      &serializeFunctionType;
  return serializeSectionContent(TypeSec, 0x01U, Func);
}

std::vector<uint8_t>
Serialize::serializeImportSection(AST::ImportSection &ImportSec) {

  std::function<std::vector<uint8_t>(AST::ImportDesc)> Func =
      &serializeImportDesc;
  return serializeSectionContent(ImportSec, 0x02U, Func);
}

std::vector<uint8_t>
Serialize::serializeFunctionSection(AST::FunctionSection &FunctionSec) {
  if (FunctionSec.getContent().size()) {
    auto section = FunctionSec.getContent();
    auto contentSize = rangeLen(section);
    std::vector<uint8_t> result(1 + leb128Len(contentSize) + contentSize);
    auto out = result.begin();
    *out++ = 0x03U;
    // U32 size of the contents in bytes
    out = encodeLEB128(contentSize, out);
    // Connect the section content.
    out = encodeRange(section, out, 1);
    return result;
  } else {
    return {};
  }
}

std::vector<uint8_t>
Serialize::serializeTableSection(AST::TableSection &TabSec) {

  std::function<std::vector<uint8_t>(AST::TableType)> Func =
      &serializeTableType;

  return serializeSectionContent(TabSec, 0x04U, Func);
}

std::vector<uint8_t>
Serialize::serializeMemorySection(AST::MemorySection &MemSec) {

  std::function<std::vector<uint8_t>(AST::MemoryType)> Func = &serializeMemType;
  return serializeSectionContent(MemSec, 0x05U, Func);
}

std::vector<uint8_t>
Serialize::serializeGlobalSection(AST::GlobalSection &GlobSec) {

  std::function<std::vector<uint8_t>(AST::GlobalSegment)> Func =
      &serializeGlobType;
  return serializeSectionContent(GlobSec, 0x06U, Func);
}
std::vector<uint8_t>
Serialize::serializeExportSection(AST::ExportSection &ExportSec) {

  std::function<std::vector<uint8_t>(AST::ExportDesc)> Func =
      &serializeExportSec;

  return serializeSectionContent(ExportSec, 0x07U, Func);
}

std::vector<uint8_t>
Serialize::serializeStartSection(AST::StartSection &StartSec) {
  if (StartSec.getContent()) {
    auto sectionsize = encodeU32(*StartSec.getContent());
    auto domSize = sectionsize.size();
    std::vector<uint8_t> result(1 + leb128Len(domSize) + domSize);
    auto out = result.begin();
    *out++ = 0x08U;
    // The section size.
    out = encodeLEB128(domSize, out);
    // Connect the section content: bytes
    out = encodeRange(sectionsize, out, 0);
    return result;
  } else {
    return {};
  }
}

std::vector<uint8_t>
Serialize::serializeElementSection(AST::ElementSection &ElemSec) {

  std::function<std::vector<uint8_t>(AST::ElementSegment)> Func =
      &serializeElemSec;

  return serializeSectionContent(ElemSec, 0x09U, Func);
}

std::vector<uint8_t>
Serialize::serializeCodeSection(AST::CodeSection &CodeSec) {

  std::function<std::vector<uint8_t>(AST::CodeSegment)> Func =
      &serializeCodeSec;
  return serializeSectionContent(CodeSec, 0x10U, Func);
}

std::vector<uint8_t>
Serialize::serializeDataSection(AST::DataSection &DataSec) {

  std::function<std::vector<uint8_t>(AST::DataSegment)> Func =
      &serializeDataSegment;

  return serializeSectionContent(DataSec, 0x11U, Func);
}

std::vector<uint8_t>
Serialize::serializeDataCountSection(AST::DataCountSection &DataCountSec) {
  if (DataCountSec.getContent()) {
    auto sectionsize = encodeU32(*DataCountSec.getContent());
    auto domSize = sectionsize.size();
    std::vector<uint8_t> result(1 + leb128Len(domSize) + domSize);
    auto out = result.begin();
    *out++ = 0x12U;
    // The section size.
    out = encodeLEB128(domSize, out);
    // section content: bytes
    out = encodeRange(sectionsize, out, 0);
    return result;
  } else {
    return {};
  }
}

} // namespace Serialize
} // namespace WasmEdge