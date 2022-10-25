#include "ast/module.h"
#include "loader/loader.h"
#include <vector>

namespace WasmEdge {

namespace Serializer {

std::vector<uint8_t> Serializer::serialize(const AST::Module &Mod) {

  std::vector<uint8_t> serialized_vector;
  std::vector<uint8_t> serialize_section(Mod.getCustomSections());  // 0
  std::vector<uint8_t> serialize_section(Mod.getTypeSection());      // 1
  std::vector<uint8_t> serialize_section(Mod.getImportSection());    // 2
  std::vector<uint8_t> serialize_section(Mod.getFunctionSection());  // 3
  std::vector<uint8_t> serialize_section(Mod.getTableSection());     // 4
  std::vector<uint8_t> serialize_section(Mod.getMemorySection());    // 5
  std::vector<uint8_t> serialize_section(Mod.getGlobalSection());    // 6
  std::vector<uint8_t> serialize_section(Mod.getExportSection());    // 7
  std::vector<uint8_t> serialize_section(Mod.getStartSection());     // 8
  std::vector<uint8_t> serialize_section(Mod.getElementSection());   // 9
  std::vector<uint8_t> serialize_section(Mod.getCodeSection());      // 10
  std::vector<uint8_t> serialize_section(Mod.getDataSection());      // 11
  std::vector<uint8_t> serialize_section(Mod.getDataCountSection()); // 12

  return serialized_vector;
}

/////////////////////////////////////////////////////////////////////////////////
// Helper Functions
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
Out EncodeRange(It f, It l, Out out) {
  using V = typename std::iterator_traits<It>::value_type;
  static_assert(std::is_trivially_copyable_v<V>);

  size_t const n = std::distance(f, l);
  auto const *bytes = reinterpret_cast<uint8_t const *>(std::addressof(*f));
  return std::copy(bytes, bytes + n * sizeof(V), EncodeLEB128(n, out));    // LEB128(vector.size()) vector
}

template <typename R, typename Out> Out EncodeRange(R const &range, Out out) {
  return EncodeRange(std::begin(range), std::end(range), out);
}

template <typename R> size_t Range_Len(R const &range) {
  using V = decltype(*std::begin(range));   // size(vector.size()))
  size_t n = std::size(range);
  return LEB128_Len(n) + n * sizeof(V);
}

////////////////////////////////////////////////////////////////////////////////////Sections

std::vector<uint8_t> serialize_section(const AST::CustomSection &CustomSecs) {
  // Name:vec(byte), Buf:vec(byte)
  auto const &name = CustomSecs.getName();
  auto const &content = CustomSecs.getContent();
  auto const ContentSize = Range_Len(name) + Range_Len(content);

  std::vector<uint8_t> CustomSecsSerialized(1 + LEB128_Len(ContentSize) +
                                            ContentSize);
  auto out = CustomSecsSerialized.begin();

  *out++ = 0x00U;                       // section ID
  out = EncodeLEB128(ContentSize, out); //  U32 size of the contents in bytes
  out = EncodeRange(name, out);         // name: vec(byte) = size:u32, bytes
  out = EncodeRange(content, out);      // content: vec(byte) = size:u32, bytes
  return CustomSecsSerialized;
}




std::vector<uint8_t> serialize_section(const AST::TypeSection &TypeSec) {

  std::vector<uint8_t> funtion_data;
  for (int i = 0; i < TypeSec.getContent().size(); i++) {
    funtion_data.push_back(0x60U); //  Function types begin with 0x60
    funtion_data.push_back(LEB128(TypeSec.getContent()[i].getParamTypes().size())); // U32 params vector size
    funtion_data.insert(funtion_data.end(), TypeSec.getContent()[i].getParamTypes().begin(), // vector of value types
                        TypeSec.getContent()[i].getParamTypes().end());
    funtion_data.push_back((LEB128(TypeSec.getContent()[i].getReturnTypes().size())); // U32 reference vector size
    funtion_data.insert(funtion_data.end(),TypeSec.getContent()[i].getReturnTypes().begin(), // vector of value types
                        TypeSec.getContent()[i].getReturnTypes().end());
  }

  std::vector<uint8_t> result;
  result.push_back(0x01U); // section ID
  result.push_back(LEB128(funtion_data.size() + 1)); //  U32 size of the contents in bytes
  result.push_back( TypeSec.getContent().size()); // No of function types present.
  result.insert( result.end(), funtion_data.begin(), funtion_data.end()); // Contents : 0x60 [vec-for-parameter-type] [vec-for-return-type]
  return result;
}




std::vector<uint8_t> serialize_section(const AST::ImportSection &ImportSec) {  //Incomplete

  std::vector<uint8_t> funtion_data;
  for (int i = 0; i < ImportSec.getContent().size(); i++) {

    // funtion_data.push_back(ImportSec.getContent()[i].getExternalFuncTypeIdx());
    funtion_data.insert(funtion_data.end(),
                        ImportSec.getContent()[i].getModuleName().begin(),
                        ImportSec.getContent()[i].getModuleName().end());
    funtion_data.insert(funtion_data.end(),
                        ImportSec.getContent()[i].getExternalName().begin(),
                        ImportSec.getContent()[i].getExternalName().end());

    funtion_data.push_back(TypeSec.getContent()[i].getReturnTypes().size());
    funtion_data.insert(funtion_data.end(),
                        TypeSec.getContent()[i].getReturnTypes().begin(),
                        TypeSec.getContent()[i].getReturnTypes().end());
  }

  std::vector<uint8_t> result;
  result.push_back(0x02U);                         // section ID
  result.push_back(LEB128(funtion_data.size() + 1));       //  U32 size of the contents in bytes
  result.push_back(ImportSec.getContent().size())    // No of function types present.

  return result;
}






std::vector<uint8_t> serialize_section(const AST::FunctionSection &FunctionSec) {

  auto const &section;
  auto const contentSize;

  if (FunctionSecs.getContent()) {
    &section = FunctionSec.getContent();
    contentSize = Range_Len(section);
  } else {
    return {};
  }
  std::vector<uint8_t> result(1 + LEB128_Len(contentSize) + content);
  auto out = result.begin();
  *out++ = 0x03U;                   // section ID
  out = EncodeLEB128(domSize, out);  // U32 size of the contents in bytes
  out = EncodeRange(section, out);  // Connect the section content.
  return Result;
}





std::vector<uint8_t> serialize_section(const AST::TableSection &TableSec) {

  std::vector<uint8_t> function_data;

  for (int i = 0; i < TableSec.getContent().size(); i++) {

    // funtion_data.push_back(ImportSec.getContent()[i].getExternalFuncTypeIdx());
    funtion_data.push_back(TableSec.getContent()[i].getRefType());  //element reference type
    if (TableSec.getContent()[i].getLimit().getMax())              //check whether a maximum is present.
    {
      funtion_data.push_back(0x01);                       
      funtion_data.push_back(LEB128(TableSec.getContent()[i].getLimit().getMin())); // Encode
      funtion_data.push_back(LEB128(TableSec.getContent()[i].getLimit().getMax())); // Encode
    } else {
      funtion_data.push_back(LEB128(TableSec.getContent()[i].getLimit().getMin());
    }
  }
  std::vector<uint8_t> result;
  result.push_back(0x04U);                              // section ID
  result.push_back(LEB128(funtion_data.size() + 1));   // U32 size of the contents in bytes
  result.push_back(TableSec.getContent().size())      //No of Table types present.

  // std::vector<uint8_t> TableSecSerialized;
  // auto out = TableSecSerialized.begin();
  // *out++ = 0x04U; // section ID
}
////




std::vector<uint8_t> serialize_section(const AST::MemorySection &MemorySec) {

  std::vector<uint8_t> function_data;

  for (int i = 0; i < MemorySec.getContent().size(); i++) {

    if (MemorySec.getContent()[i].getLimit().hasMax) {
      funtion_data.push_back(0x01);
      result.insert(Encode(MemorySec.getContent()[i].getLimit().getMax()),
                  funtion_data.begin(), funtion_data.end())
      result.insert(Encode(MemorySec.getContent()[i].getLimit().getMin()),
                  funtion_data.begin(), funtion_data.end())
    } else {
      funtion_data.push_back(0x00);
      result.insert(Encode(MemorySec.getContent()[i].getLimit().getMin()),
                    funtion_data.begin(), funtion_data.end())
    }
  }

  result.push_back(0x05U);                             // section ID
  result.push_back(LEB128(funtion_data.size() + 1));          // U32 size of the contents in bytes
  result.push_back(TableSec.getContent().size())       //No of Table types present.

  // std::vector<uint8_t> MemorySecSerialized;
  // auto out = MemorySecSerialized.begin();
  // *out++ = 0x05U; // section ID
}


std::vector<uint8_t> serialize_section(const AST::GlobalSection &GlobalSec) {

  std::vector<uint8_t> function_data;

  for (int i = 0; i < GlobalSec.getContent().size(); i++) {
    function_data.push_back(GlobalSec.getContent()[i].getGlobalType().getValType());
    function_data.push_back(GlobalSec.getContent()[i].getGlobalType().getValMut());
    function_data.insert(
        function_data.end(),
        GlobalSec.getContent()[i].getExpr().getInstrs().begin(),   //instruction sequence
        GlobalSec.getContent()[i].getExpr().getInstrs().end());
    function_data.push_back(0x0B);                                 //terminated with an explicit 0X0B  opcode for .
  }

  result.push_back(0x06U);                   // section ID
  result.push_back((LEB128(funtion_data.size() + 1));  // U32 size of the contents in bytes
  result.push_back(GlobalSec.getContent().size())     //No of Table types present.
}


/////////////////////////

std::vector<uint8_t> serialize_section(const AST::ExportSection &ExportSec) {


}





std::vector<uint8_t> serialize_section(const AST::StartSection &StartSec) {

  auto const &section;
  auto const domSize;
  // FuncIndex: u32
  if (StartSec.getContent()) {
    &section = StartSec.getContent();
    domSize = Range_Len(section);
  } else {
    return {};
  }
  std::vector<uint8_t> result(1 + LEB128_Len(domSize) + domSize);
  auto out = result.begin();
  *out++ = 0x08U;                   // The section ID of custom section is 0x08.
  out = EncodeLEB128(domSize, out); // The section size.
  out = EncodeRange(section, out);  // Connect the section content.
  return Result;
}



std::vector<uint8_t> serialize_section(const AST::ElementSection &ElementSec) {}
std::vector<uint8_t> serialize_section(const AST::CodeSection &CodeSec) {}
std::vector<uint8_t> serialize_section(const AST::DataSection &DataSec) {}




std::vector<uint8_t> serialize_section(const AST::DataCountSection &DataCountSec) {

  auto const &section;
  auto const contentSize;
  // FuncIndex: u32
  if (DataCountSec.getContent()) {
    &section = DataCountSec.getContent();
    contentSize = Range_Len(section);
  } else {
    return {};
  }
  std::vector<uint8_t> result(1 + LEB128_Len(contentSize) + contentSize);
  auto out = result.begin();
  *out++ = 0x12U;                   // The section ID of custom section is 0x08.
  out = EncodeLEB128(contentSize, out); // The section size.
  out = EncodeRange(section, out);  // Connect the section content.
  return Result;
}

} // namespace Serializer

} // namespace WasmEdge
