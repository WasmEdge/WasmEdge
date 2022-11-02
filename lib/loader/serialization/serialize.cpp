#include "ast/module.h"
#include "loader/loader.h"
#include "serialization/serialize/serializetype.cpp"

namespace WasmEdge {
namespace Serialize {

std::vector<uint8_t> Serialize::serializeModule(const AST::Module &Mod) {
  std::vector<uint8_t> serializedModule;
  std::vector<uint8_t> serializedCustom =
      serializeCustomSection(Mod.getCustomSections()); // 0
  std::vector<uint8_t> serializedType =
      serializeTypeSection(Mod.getTypeSection()); // 1
  std::vector<uint8_t> serializedImport =
      serializeImportSection(Mod.getImportSection()); // 2
  std::vector<uint8_t> serializedFunction =
      serializeFunctionSection(Mod.getFunctionSection()); // 3
  std::vector<uint8_t> serializedTable =
      serializeTableSection(Mod.getTableSection()); // 4
  std::vector<uint8_t> serializedmemery =
      serializeMemorySection(Mod.getMemorySection()); // 5
  std::vector<uint8_t> serializedGlobal =
      serializeGlobalSection(Mod.getGlobalSection()); // 6
  std::vector<uint8_t> serializedExport =
      serializeExportSection(Mod.getExportSection()); // 7
  std::vector<uint8_t> serializedStart =
      serializeStartSection(Mod.getStartSection()); // 8
  std::vector<uint8_t> serializedElement =
      serializeElementSection(Mod.getElementSection()); // 9
  std::vector<uint8_t> serializedCode =
      serializeCodeSection(Mod.getCodeSection()); // 10
  std::vector<uint8_t> serializedData =
      serializeDataSection(Mod.getDataSection()); // 11
  std::vector<uint8_t> serializedDataCount =
      serializeDataCountSection(Mod.getDataCountSection()); // 12
  return serializedModule;
}

////////////////HelperFunctions////////////////////////////////////////////////////////////////

template <typename T, typename F> size_t EncodeLEB128_Impl(T d, F callback) {
  static_assert(std::is_unsigned_v<T> && std::is_integral_v<T>);

  // unsigned LEB128 encoding
  size_t n = 0;
  do {
    unsigned int x = d & 0b01111111;
    d >>= 7;
    if (d)
      x |= 0b10000000;
    n++;
    callback(x);
  } while (d);

  return n;
}

template <typename T> size_t LEB128_Len(T d) {
  return EncodeLEB128_Impl(std::move(d), [](auto &&) {});
}

template <typename T, typename Out> Out EncodeLEB128(T d, Out out) {
  EncodeLEB128_Impl(std::move(d), [&](uint8_t v) { *out++ = v; });
  return out;
}

template <std::contiguous_iterator It, typename Out>
Out EncodeRange(It f, It l, Out out, int x) {
  using V = typename std::iterator_traits<It>::value_type;
  static_assert(std::is_trivially_copyable_v<V>);

  size_t const n = std::distance(f, l);
  auto const *bytes = reinterpret_cast<uint8_t const *>(std::addressof(*f));
  if (x) {
    return std::copy(bytes, bytes + n * sizeof(V),
                     EncodeLEB128(n, out)); // LEB128(vector.size()) vector
  } else {
    return std::copy(bytes, bytes + n * sizeof(V),
                     out); // LEB128(vector.size()) vector
  }
}

template <typename R, typename Out>
Out EncodeRange(R const &range, Out out, int x) {
  return EncodeRange(std::begin(range), std::end(range), out, x);
}

template <typename R> size_t Range_Len(R const &range) {
  using V = decltype(*std::begin(range));
  size_t n = std::size(range);
  return LEB128_Len(n) + n * sizeof(V);
}

std::vector<uint8_t> EncodeU32(uint32_t d) {
  std::vector<uint8_t> encoded;
  // unsigned LEB128 encoding
  do {
    auto x = d & 0b01111111;
    d >>= 7;
    if (d)
      x |= 0b10000000;
    encoded.push_back(x);
  } while (d);

  return encoded;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////

template <typename T, typename B, typename L>
std::vector<uint8_t> serializeSectionContent(T &Sec, B &Byte, L &&Func) {
  if (Sec.getContent().size()) {

    std::vector<uint8_t> Result;
    std::vector<uint8_t> content;

    for (auto &type : Sec.getContent()) {
      auto serializedType = Func(type);
      content.insert(content.end(), serializedType.begin(),
                     serializedType.end());
    }
    std::vector<uint8_t> ContentVectorSize = EncodeU32(Sec.getContent().size());
    std::vector<uint8_t> ContentSize =
        EncodeU32(content.size() + ContentVectorSize.size());
    Result.push_back(0x01U); // one-byte section id
    Result.insert(Result.end(), ContentSize.begin(),
                  ContentSize.end()); //  U32 size of the contents in bytes
    Result.insert(Result.end(), ContentVectorSize.begin(),
                  ContentVectorSize.end()); // No of function types present.
    Result.insert(Result.end(), content.begin(), content.end());
    return result;
  }
  return {};
}

//////////////////////////////////////////////////////////////////Sections

std::vector<uint8_t>
Serialize::serializeCustomSection(const AST::CustomSection &CustomSecs) {
  auto const &name = CustomSecs.getName();
  auto const &content = CustomSecs.getContent();
  auto const ContentSize = Range_Len(name) + Range_Len(content);

  std::vector<uint8_t> CustomSecsSerialized(1 + LEB128_Len(ContentSize) +
                                            ContentSize);
  auto out = CustomSecsSerialized.begin();
  *out++ = 0x00U;                       // section ID
  out = EncodeLEB128(ContentSize, out); //  U32 size of the contents in bytes
  out = EncodeRange(name, out, 1);      // name: vec(byte) = size:u32, bytes
  out = EncodeRange(content, out, 1);   // content: vec(byte) = size:u32, bytes
  return CustomSecsSerialized;
}

std::vector<uint8_t>
Serialize::serializeTypeSection(const AST::TypeSection &TypeSec) {
  return serializeSectionContent(
      TypeSec, 0x01U, serializeFunctionType(const AST::FunctionType &FuncType));
}

std::vector<uint8_t>
Serialize::serializeImportSection(const AST::ImportSection &ImportSec) {
  return serializeSectionContent(
      ImportSec, 0x02U, serializeImportDesc(const AST::ImportDesc &ImpDesc));
}

std::vector<uint8_t>
Serialize::serializeFunctionSection(const AST::FunctionSection &FunctionSec) {
  if (FunctionSecs.getContent().size()) {
    auto section = FunctionSecs.getContent();
    auto contentSize = Range_Len(section);
    std::vector<uint8_t> result(1 + LEB128_Len(contentSize) + contentSize);
    auto out = result.begin();
    *out++ = 0x03U;                       // section ID
    out = EncodeLEB128(contentSize, out); // U32 size of the contents in bytes
    out = EncodeRange(section, out, 1);   // Connect the section content.
    return result;
  } else {
    return {};
  }
}

std::vector<uint8_t>
Serialize::serializeTableSection(const AST::TableSection &TabSec) {
  return serializeSectionContent(TabSec, 0x04U,
                                 serializeTableType(AST::TableType & TabType));
}

std::vector<uint8_t>
Serialize::serializeMemorySection(const AST::MemorySection &MemSec) {
  return serializeSectionContent(MemSec, 0x05U,
                                 serializeMemType(AST::MemoryType & MemType));
}

std::vector<uint8_t>
Serialize::serializeGlobalSection(const AST::GlobalSection &GlobSec) {
  return serializeSectionContent(
      GlobSec, 0x06U, serializeGlobType(AST::GlobalSegment & GlobalSegment));
}
std::vector<uint8_t>
Serialize::serializeExportSection(const AST::ExportSection &ExportSec) {
  return serializeSectionContent(
      ExportSec, 0x07U, serializeExportSec(const AST::ExportDesc &ExpDesc));
}

std::vector<uint8_t>
Serialize::serializeStartSection(const AST::StartSection &StartSec) {
  if (StartSec.getContent()) {
    auto sectionsize = EncodeU32(*StartSec.getContent());
    auto domSize = sectionsize.size();
    std::vector<uint8_t> result(1 + LEB128_Len(domSize) + domSize);
    auto out = result.begin();
    *out++ = 0x08U; // The section ID of custom section is 0x08.
    out = EncodeLEB128(domSize, out);       // The section size.
    out = EncodeRange(sectionsize, out, 0); // Connect the section content.
    return result;
  } else {
    return {};
  }
}

std::vector<uint8_t>
Serialize::serializeElementSection(const AST::ElementSection &ElemSec) {
  return serializeSectionContent(
      CodeSec, 0x09U, serializeElemSec(const AST::ElementSegment &ElemSeg));
}

std::vector<uint8_t>
Serialize::serializeCodeSection(const AST::CodeSection &CodeSec) {
  return serializeSectionContent(
      CodeSec, 0x10U, serializeCodeSec(AST::CodeSegment & CodeSegment));
}

std::vector<uint8_t>
Serialize::serializeDataSection(const AST::DataSection &DataSec) {
  return serializeSectionContent(
      ExportSec, 0x11U, serializeDataSegment(AST::DataSegment & DataSegment));
}

std::vector<uint8_t> Serialize::serializeDataCountSection(
    const AST::DataCountSection &DataCountSec) {
  if (DataCountSec.getContent()) {
    auto sectionsize = EncodeU32(*DataCountSec.getContent());
    auto domSize = sectionsize.size();
    std::vector<uint8_t> result(1 + LEB128_Len(domSize) + domSize);
    auto out = result.begin();
    *out++ = 0x12U; // The section ID of custom section is 0x08.
    out = EncodeLEB128(domSize, out);       // The section size.
    out = EncodeRange(sectionsize, out, 0); // Connect the section content.
    return result;
  } else {
    return {};
  }
}

} // namespace Serialize
} // namespace WasmEdge