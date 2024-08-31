// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText: 2019-2024 Second State INC

#include "common/filesystem.h"
#include "experimental/span.hpp"
#include "wasmedge/wasmedge.h"

#include <cstdint>
#include <fmt/format.h>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace {

std::vector<uint8_t> FuncsWasm = {
    0x0,  0x61, 0x73, 0x6d, 0x1,  0x0,  0x0,  0x0,  0x1,  0x16, 0x3,  0x60,
    0x2,  0x6f, 0x7f, 0x1,  0x7f, 0x60, 0x3,  0x6f, 0x7f, 0x7f, 0x1,  0x7f,
    0x60, 0x4,  0x6f, 0x6f, 0x7f, 0x7f, 0x1,  0x7f, 0x2,  0x53, 0x3,  0xd,
    0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x5f, 0x6d, 0x6f, 0x64, 0x75, 0x6c,
    0x65, 0xe,  0x66, 0x75, 0x6e, 0x63, 0x74, 0x6f, 0x72, 0x5f, 0x73, 0x71,
    0x75, 0x61, 0x72, 0x65, 0x0,  0x0,  0xd,  0x65, 0x78, 0x74, 0x65, 0x72,
    0x6e, 0x5f, 0x6d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0x9,  0x63, 0x6c, 0x61,
    0x73, 0x73, 0x5f, 0x61, 0x64, 0x64, 0x0,  0x1,  0xd,  0x65, 0x78, 0x74,
    0x65, 0x72, 0x6e, 0x5f, 0x6d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0x8,  0x66,
    0x75, 0x6e, 0x63, 0x5f, 0x6d, 0x75, 0x6c, 0x0,  0x1,  0x3,  0x5,  0x4,
    0x1,  0x1,  0x0,  0x2,  0x5,  0x3,  0x1,  0x0,  0x1,  0x7,  0x40, 0x5,
    0x6,  0x6d, 0x65, 0x6d, 0x6f, 0x72, 0x79, 0x2,  0x0,  0x8,  0x63, 0x61,
    0x6c, 0x6c, 0x5f, 0x61, 0x64, 0x64, 0x0,  0x3,  0x8,  0x63, 0x61, 0x6c,
    0x6c, 0x5f, 0x6d, 0x75, 0x6c, 0x0,  0x4,  0xb,  0x63, 0x61, 0x6c, 0x6c,
    0x5f, 0x73, 0x71, 0x75, 0x61, 0x72, 0x65, 0x0,  0x5,  0xf,  0x63, 0x61,
    0x6c, 0x6c, 0x5f, 0x61, 0x64, 0x64, 0x5f, 0x73, 0x71, 0x75, 0x61, 0x72,
    0x65, 0x0,  0x6,  0xa,  0x2f, 0x4,  0xa,  0x0,  0x20, 0x0,  0x20, 0x1,
    0x20, 0x2,  0x10, 0x1,  0xb,  0xa,  0x0,  0x20, 0x0,  0x20, 0x1,  0x20,
    0x2,  0x10, 0x2,  0xb,  0x8,  0x0,  0x20, 0x0,  0x20, 0x1,  0x10, 0x0,
    0xb,  0xe,  0x0,  0x20, 0x1,  0x20, 0x0,  0x20, 0x2,  0x20, 0x3,  0x10,
    0x1,  0x10, 0x0,  0xb,  0x0,  0x66, 0x4,  0x6e, 0x61, 0x6d, 0x65, 0x1,
    0x26, 0x3,  0x0,  0xe,  0x66, 0x75, 0x6e, 0x63, 0x74, 0x6f, 0x72, 0x5f,
    0x73, 0x71, 0x75, 0x61, 0x72, 0x65, 0x1,  0x9,  0x63, 0x6c, 0x61, 0x73,
    0x73, 0x5f, 0x61, 0x64, 0x64, 0x2,  0x8,  0x66, 0x75, 0x6e, 0x63, 0x5f,
    0x6d, 0x75, 0x6c, 0x2,  0x37, 0x7,  0x0,  0x2,  0x0,  0x0,  0x1,  0x0,
    0x1,  0x3,  0x0,  0x0,  0x1,  0x0,  0x2,  0x0,  0x2,  0x3,  0x0,  0x0,
    0x1,  0x0,  0x2,  0x0,  0x3,  0x3,  0x0,  0x0,  0x1,  0x0,  0x2,  0x0,
    0x4,  0x3,  0x0,  0x0,  0x1,  0x0,  0x2,  0x0,  0x5,  0x2,  0x0,  0x0,
    0x1,  0x0,  0x6,  0x4,  0x0,  0x0,  0x1,  0x0,  0x2,  0x0,  0x3,  0x0};
std::vector<uint8_t> STLWasm = {
    0x0,  0x61, 0x73, 0x6d, 0x1,  0x0,  0x0,  0x0,  0x1,  0x17, 0x4,  0x60,
    0x2,  0x6f, 0x6f, 0x0,  0x60, 0x2,  0x6f, 0x7f, 0x0,  0x60, 0x3,  0x6f,
    0x6f, 0x6f, 0x0,  0x60, 0x2,  0x6f, 0x6f, 0x1,  0x7f, 0x2,  0xfa, 0x1,
    0x8,  0xd,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x5f, 0x6d, 0x6f, 0x64,
    0x75, 0x6c, 0x65, 0xf,  0x73, 0x74, 0x6c, 0x5f, 0x6f, 0x73, 0x74, 0x72,
    0x65, 0x61, 0x6d, 0x5f, 0x73, 0x74, 0x72, 0x0,  0x0,  0xd,  0x65, 0x78,
    0x74, 0x65, 0x72, 0x6e, 0x5f, 0x6d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0xf,
    0x73, 0x74, 0x6c, 0x5f, 0x6f, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6d, 0x5f,
    0x75, 0x33, 0x32, 0x0,  0x1,  0xd,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e,
    0x5f, 0x6d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0xe,  0x73, 0x74, 0x6c, 0x5f,
    0x6d, 0x61, 0x70, 0x5f, 0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x0,  0x2,
    0xd,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x5f, 0x6d, 0x6f, 0x64, 0x75,
    0x6c, 0x65, 0xd,  0x73, 0x74, 0x6c, 0x5f, 0x6d, 0x61, 0x70, 0x5f, 0x65,
    0x72, 0x61, 0x73, 0x65, 0x0,  0x0,  0xd,  0x65, 0x78, 0x74, 0x65, 0x72,
    0x6e, 0x5f, 0x6d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0xe,  0x73, 0x74, 0x6c,
    0x5f, 0x73, 0x65, 0x74, 0x5f, 0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x0,
    0x1,  0xd,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x5f, 0x6d, 0x6f, 0x64,
    0x75, 0x6c, 0x65, 0xd,  0x73, 0x74, 0x6c, 0x5f, 0x73, 0x65, 0x74, 0x5f,
    0x65, 0x72, 0x61, 0x73, 0x65, 0x0,  0x1,  0xd,  0x65, 0x78, 0x74, 0x65,
    0x72, 0x6e, 0x5f, 0x6d, 0x6f, 0x64, 0x75, 0x6c, 0x65, 0xf,  0x73, 0x74,
    0x6c, 0x5f, 0x76, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x5f, 0x70, 0x75, 0x73,
    0x68, 0x0,  0x1,  0xd,  0x65, 0x78, 0x74, 0x65, 0x72, 0x6e, 0x5f, 0x6d,
    0x6f, 0x64, 0x75, 0x6c, 0x65, 0xe,  0x73, 0x74, 0x6c, 0x5f, 0x76, 0x65,
    0x63, 0x74, 0x6f, 0x72, 0x5f, 0x73, 0x75, 0x6d, 0x0,  0x3,  0x3,  0x9,
    0x8,  0x0,  0x1,  0x2,  0x0,  0x1,  0x1,  0x1,  0x3,  0x5,  0x3,  0x1,
    0x0,  0x1,  0x7,  0x9b, 0x1,  0x9,  0x6,  0x6d, 0x65, 0x6d, 0x6f, 0x72,
    0x79, 0x2,  0x0,  0x10, 0x63, 0x61, 0x6c, 0x6c, 0x5f, 0x6f, 0x73, 0x74,
    0x72, 0x65, 0x61, 0x6d, 0x5f, 0x73, 0x74, 0x72, 0x0,  0x8,  0x10, 0x63,
    0x61, 0x6c, 0x6c, 0x5f, 0x6f, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6d, 0x5f,
    0x75, 0x33, 0x32, 0x0,  0x9,  0xf,  0x63, 0x61, 0x6c, 0x6c, 0x5f, 0x6d,
    0x61, 0x70, 0x5f, 0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x0,  0xa,  0xe,
    0x63, 0x61, 0x6c, 0x6c, 0x5f, 0x6d, 0x61, 0x70, 0x5f, 0x65, 0x72, 0x61,
    0x73, 0x65, 0x0,  0xb,  0xf,  0x63, 0x61, 0x6c, 0x6c, 0x5f, 0x73, 0x65,
    0x74, 0x5f, 0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x0,  0xc,  0xe,  0x63,
    0x61, 0x6c, 0x6c, 0x5f, 0x73, 0x65, 0x74, 0x5f, 0x65, 0x72, 0x61, 0x73,
    0x65, 0x0,  0xd,  0x10, 0x63, 0x61, 0x6c, 0x6c, 0x5f, 0x76, 0x65, 0x63,
    0x74, 0x6f, 0x72, 0x5f, 0x70, 0x75, 0x73, 0x68, 0x0,  0xe,  0xf,  0x63,
    0x61, 0x6c, 0x6c, 0x5f, 0x76, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x5f, 0x73,
    0x75, 0x6d, 0x0,  0xf,  0xa,  0x4b, 0x8,  0x8,  0x0,  0x20, 0x0,  0x20,
    0x1,  0x10, 0x0,  0xb,  0x8,  0x0,  0x20, 0x0,  0x20, 0x1,  0x10, 0x1,
    0xb,  0xa,  0x0,  0x20, 0x0,  0x20, 0x1,  0x20, 0x2,  0x10, 0x2,  0xb,
    0x8,  0x0,  0x20, 0x0,  0x20, 0x1,  0x10, 0x3,  0xb,  0x8,  0x0,  0x20,
    0x0,  0x20, 0x1,  0x10, 0x4,  0xb,  0x8,  0x0,  0x20, 0x0,  0x20, 0x1,
    0x10, 0x5,  0xb,  0x8,  0x0,  0x20, 0x0,  0x20, 0x1,  0x10, 0x6,  0xb,
    0x8,  0x0,  0x20, 0x0,  0x20, 0x1,  0x10, 0x7,  0xb,  0x0,  0xf1, 0x1,
    0x4,  0x6e, 0x61, 0x6d, 0x65, 0x1,  0x82, 0x1,  0x8,  0x0,  0xf,  0x73,
    0x74, 0x6c, 0x5f, 0x6f, 0x73, 0x74, 0x72, 0x65, 0x61, 0x6d, 0x5f, 0x73,
    0x74, 0x72, 0x1,  0xf,  0x73, 0x74, 0x6c, 0x5f, 0x6f, 0x73, 0x74, 0x72,
    0x65, 0x61, 0x6d, 0x5f, 0x75, 0x33, 0x32, 0x2,  0xe,  0x73, 0x74, 0x6c,
    0x5f, 0x6d, 0x61, 0x70, 0x5f, 0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x3,
    0xd,  0x73, 0x74, 0x6c, 0x5f, 0x6d, 0x61, 0x70, 0x5f, 0x65, 0x72, 0x61,
    0x73, 0x65, 0x4,  0xe,  0x73, 0x74, 0x6c, 0x5f, 0x73, 0x65, 0x74, 0x5f,
    0x69, 0x6e, 0x73, 0x65, 0x72, 0x74, 0x5,  0xd,  0x73, 0x74, 0x6c, 0x5f,
    0x73, 0x65, 0x74, 0x5f, 0x65, 0x72, 0x61, 0x73, 0x65, 0x6,  0xf,  0x73,
    0x74, 0x6c, 0x5f, 0x76, 0x65, 0x63, 0x74, 0x6f, 0x72, 0x5f, 0x70, 0x75,
    0x73, 0x68, 0x7,  0xe,  0x73, 0x74, 0x6c, 0x5f, 0x76, 0x65, 0x63, 0x74,
    0x6f, 0x72, 0x5f, 0x73, 0x75, 0x6d, 0x2,  0x65, 0x10, 0x0,  0x2,  0x0,
    0x0,  0x1,  0x0,  0x1,  0x2,  0x0,  0x0,  0x1,  0x0,  0x2,  0x3,  0x0,
    0x0,  0x1,  0x0,  0x2,  0x0,  0x3,  0x2,  0x0,  0x0,  0x1,  0x0,  0x4,
    0x2,  0x0,  0x0,  0x1,  0x0,  0x5,  0x2,  0x0,  0x0,  0x1,  0x0,  0x6,
    0x2,  0x0,  0x0,  0x1,  0x0,  0x7,  0x2,  0x0,  0x0,  0x1,  0x0,  0x8,
    0x2,  0x0,  0x0,  0x1,  0x0,  0x9,  0x2,  0x0,  0x0,  0x1,  0x0,  0xa,
    0x3,  0x0,  0x0,  0x1,  0x0,  0x2,  0x0,  0xb,  0x2,  0x0,  0x0,  0x1,
    0x0,  0xc,  0x2,  0x0,  0x0,  0x1,  0x0,  0xd,  0x2,  0x0,  0x0,  0x1,
    0x0,  0xe,  0x2,  0x0,  0x0,  0x1,  0x0,  0xf,  0x2,  0x0,  0x0,  0x1,
    0x0};

void HexToFile(cxx20::span<const uint8_t> Wasm, const char *Path) {
  std::ofstream TFile(std::filesystem::u8path(Path), std::ios_base::binary);
  TFile.write(reinterpret_cast<const char *>(Wasm.data()),
              static_cast<std::streamsize>(Wasm.size()));
  TFile.close();
}

// The following are the functions and class definitions to pass as
// references.

// Test: function to pass as function pointer
uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }

// Test: class to pass as reference
class AddClass {
public:
  uint32_t add(uint32_t A, uint32_t B) const { return A + B; }
};

// Test: functor to pass as reference
struct SquareStruct {
  uint32_t operator()(uint32_t Val) const { return Val * Val; }
};

// The following are the host function definitions.

// Host function to call functor by external reference
WasmEdge_Result ExternFunctorSquare(void *,
                                    const WasmEdge_CallingFrameContext *,
                                    const WasmEdge_Value *In,
                                    WasmEdge_Value *Out) {
  // Function type: {externref, i32} -> {i32}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  SquareStruct &Obj = *reinterpret_cast<SquareStruct *>(Ptr);
  uint32_t C = Obj(static_cast<uint32_t>(WasmEdge_ValueGetI32(In[1])));
  Out[0] = WasmEdge_ValueGenI32(static_cast<int32_t>(C));
  return WasmEdge_Result_Success;
}

// Host function to access class by external reference
WasmEdge_Result ExternClassAdd(void *, const WasmEdge_CallingFrameContext *,
                               const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // Function type: {externref, i32, i32} -> {i32}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  AddClass &Obj = *reinterpret_cast<AddClass *>(Ptr);
  uint32_t C = Obj.add(static_cast<uint32_t>(WasmEdge_ValueGetI32(In[1])),
                       static_cast<uint32_t>(WasmEdge_ValueGetI32(In[2])));
  Out[0] = WasmEdge_ValueGenI32(static_cast<int32_t>(C));
  return WasmEdge_Result_Success;
}

// Host function to call function by external reference as a function pointer
WasmEdge_Result ExternFuncMul(void *, const WasmEdge_CallingFrameContext *,
                              const WasmEdge_Value *In, WasmEdge_Value *Out) {
  // Function type: {externref, i32, i32} -> {i32}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  uint32_t (*Obj)(uint32_t, uint32_t) =
      *reinterpret_cast<uint32_t (*)(uint32_t, uint32_t)>(Ptr);
  uint32_t C = Obj(static_cast<uint32_t>(WasmEdge_ValueGetI32(In[1])),
                   static_cast<uint32_t>(WasmEdge_ValueGetI32(In[2])));
  Out[0] = WasmEdge_ValueGenI32(static_cast<int32_t>(C));
  return WasmEdge_Result_Success;
}

// Host function to output std::string through std::ostream
WasmEdge_Result ExternSTLOStreamStr(void *,
                                    const WasmEdge_CallingFrameContext *,
                                    const WasmEdge_Value *In,
                                    WasmEdge_Value *) {
  // Function type: {externref, externref} -> {}
  void *Ptr0 = WasmEdge_ValueGetExternRef(In[0]);
  void *Ptr1 = WasmEdge_ValueGetExternRef(In[1]);
  std::ostream &RefOS = *reinterpret_cast<std::ostream *>(Ptr0);
  std::string &RefStr = *reinterpret_cast<std::string *>(Ptr1);
  RefOS << RefStr;
  return WasmEdge_Result_Success;
}

// Host function to output uint32_t through std::ostream
WasmEdge_Result ExternSTLOStreamU32(void *,
                                    const WasmEdge_CallingFrameContext *,
                                    const WasmEdge_Value *In,
                                    WasmEdge_Value *) {
  // Function type: {externref, i32} -> {}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  std::ostream &RefOS = *reinterpret_cast<std::ostream *>(Ptr);
  RefOS << static_cast<uint32_t>(WasmEdge_ValueGetI32(In[1]));
  return WasmEdge_Result_Success;
}

// Host function to insert {key, val} to std::map<std::string, std::string>
WasmEdge_Result ExternSTLMapInsert(void *, const WasmEdge_CallingFrameContext *,
                                   const WasmEdge_Value *In, WasmEdge_Value *) {
  // Function type: {externref, externref, externref} -> {}
  void *Ptr0 = WasmEdge_ValueGetExternRef(In[0]);
  void *Ptr1 = WasmEdge_ValueGetExternRef(In[1]);
  void *Ptr2 = WasmEdge_ValueGetExternRef(In[2]);
  auto &Map = *reinterpret_cast<std::map<std::string, std::string> *>(Ptr0);
  auto &Key = *reinterpret_cast<std::string *>(Ptr1);
  auto &Val = *reinterpret_cast<std::string *>(Ptr2);
  Map[Key] = Val;
  return WasmEdge_Result_Success;
}

// Host function to erase std::map<std::string, std::string> with key
WasmEdge_Result ExternSTLMapErase(void *, const WasmEdge_CallingFrameContext *,
                                  const WasmEdge_Value *In, WasmEdge_Value *) {
  // Function type: {externref, externref} -> {}
  void *Ptr0 = WasmEdge_ValueGetExternRef(In[0]);
  void *Ptr1 = WasmEdge_ValueGetExternRef(In[1]);
  auto &Map = *reinterpret_cast<std::map<std::string, std::string> *>(Ptr0);
  auto &Key = *reinterpret_cast<std::string *>(Ptr1);
  Map.erase(Key);
  return WasmEdge_Result_Success;
}

// Host function to insert key to std::set<uint32_t>
WasmEdge_Result ExternSTLSetInsert(void *, const WasmEdge_CallingFrameContext *,
                                   const WasmEdge_Value *In, WasmEdge_Value *) {
  // Function type: {externref, i32} -> {}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  auto &Set = *reinterpret_cast<std::set<uint32_t> *>(Ptr);
  Set.insert(static_cast<uint32_t>(WasmEdge_ValueGetI32(In[1])));
  return WasmEdge_Result_Success;
}

// Host function to erase std::set<uint32_t> with key
WasmEdge_Result ExternSTLSetErase(void *, const WasmEdge_CallingFrameContext *,
                                  const WasmEdge_Value *In, WasmEdge_Value *) {
  // Function type: {externref, i32} -> {}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  auto &Set = *reinterpret_cast<std::set<uint32_t> *>(Ptr);
  Set.erase(static_cast<uint32_t>(WasmEdge_ValueGetI32(In[1])));
  return WasmEdge_Result_Success;
}

// Host function to push value into std::vector<uint32_t>
WasmEdge_Result ExternSTLVectorPush(void *,
                                    const WasmEdge_CallingFrameContext *,
                                    const WasmEdge_Value *In,
                                    WasmEdge_Value *) {
  // Function type: {externref, i32} -> {}
  void *Ptr = WasmEdge_ValueGetExternRef(In[0]);
  auto &Vec = *reinterpret_cast<std::vector<uint32_t> *>(Ptr);
  Vec.push_back(static_cast<uint32_t>(WasmEdge_ValueGetI32(In[1])));
  return WasmEdge_Result_Success;
}

// Host function to summarize value in slice of std::vector<uint32_t>
WasmEdge_Result ExternSTLVectorSum(void *, const WasmEdge_CallingFrameContext *,
                                   const WasmEdge_Value *In,
                                   WasmEdge_Value *Out) {
  // Function type: {externref, externref} -> {i32}
  void *Ptr0 = WasmEdge_ValueGetExternRef(In[0]);
  void *Ptr1 = WasmEdge_ValueGetExternRef(In[1]);
  auto &It = *reinterpret_cast<std::vector<uint32_t>::iterator *>(Ptr0);
  auto &ItEnd = *reinterpret_cast<std::vector<uint32_t>::iterator *>(Ptr1);
  uint32_t Sum = 0;
  while (It != ItEnd) {
    Sum += *It;
    It++;
  }
  Out[0] = WasmEdge_ValueGenI32(static_cast<int32_t>(Sum));
  return WasmEdge_Result_Success;
}

// Helper function to create the "extern_module" module instance.
WasmEdge_ModuleInstanceContext *createExternModule() {
  WasmEdge_String HostName;
  WasmEdge_FunctionTypeContext *HostFType = nullptr;
  WasmEdge_FunctionInstanceContext *HostFunc = nullptr;
  WasmEdge_ValType P[3], R[1];

  HostName = WasmEdge_StringCreateByCString("extern_module");
  WasmEdge_ModuleInstanceContext *HostMod =
      WasmEdge_ModuleInstanceCreate(HostName);
  WasmEdge_StringDelete(HostName);

  // Add host function "functor_square": {externref, i32} -> {i32}
  P[0] = WasmEdge_ValTypeGenExternRef();
  P[1] = WasmEdge_ValTypeGenI32();
  R[0] = WasmEdge_ValTypeGenI32();
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, R, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternFunctorSquare,
                                             nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("functor_square");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "class_add": {externref, i32, i32} -> {i32}
  P[2] = WasmEdge_ValTypeGenI32();
  HostFType = WasmEdge_FunctionTypeCreate(P, 3, R, 1);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, ExternClassAdd, nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("class_add");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "func_mul": {externref, i32, i32} -> {i32}
  HostFType = WasmEdge_FunctionTypeCreate(P, 3, R, 1);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, ExternFuncMul, nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("func_mul");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "stl_ostream_str": {externref, externref} -> {}
  P[1] = WasmEdge_ValTypeGenExternRef();
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, nullptr, 0);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternSTLOStreamStr,
                                             nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("stl_ostream_str");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "stl_ostream_u32": {externref, i32} -> {}
  P[1] = WasmEdge_ValTypeGenI32();
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, nullptr, 0);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternSTLOStreamU32,
                                             nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("stl_ostream_u32");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "stl_map_insert": {externref, externref, externref}->{}
  P[1] = WasmEdge_ValTypeGenExternRef();
  P[2] = WasmEdge_ValTypeGenExternRef();
  HostFType = WasmEdge_FunctionTypeCreate(P, 3, nullptr, 0);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternSTLMapInsert,
                                             nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("stl_map_insert");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "stl_map_erase": {externref, externref}->{}
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, nullptr, 0);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, ExternSTLMapErase, nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("stl_map_erase");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "stl_set_insert": {externref, i32}->{}
  P[1] = WasmEdge_ValTypeGenI32();
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, nullptr, 0);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternSTLSetInsert,
                                             nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("stl_set_insert");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "stl_set_erase": {externref, i32}->{}
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, nullptr, 0);
  HostFunc =
      WasmEdge_FunctionInstanceCreate(HostFType, ExternSTLSetErase, nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("stl_set_erase");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "stl_vector_push": {externref, i32}->{}
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, nullptr, 0);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternSTLVectorPush,
                                             nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("stl_vector_push");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  // Add host function "stl_vector_sum": {externref, externref} -> {i32}
  P[1] = WasmEdge_ValTypeGenExternRef();
  HostFType = WasmEdge_FunctionTypeCreate(P, 2, R, 1);
  HostFunc = WasmEdge_FunctionInstanceCreate(HostFType, ExternSTLVectorSum,
                                             nullptr, 0);
  WasmEdge_FunctionTypeDelete(HostFType);
  HostName = WasmEdge_StringCreateByCString("stl_vector_sum");
  WasmEdge_ModuleInstanceAddFunction(HostMod, HostName, HostFunc);
  WasmEdge_StringDelete(HostName);

  return HostMod;
}

TEST(ExternRefTest, Ref__Functions) {
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(nullptr, nullptr);
  WasmEdge_ModuleInstanceContext *HostMod = createExternModule();
  WasmEdge_Value P[4], R[1];
  WasmEdge_String FuncName;

  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VMCxt, HostMod)));
  char FuncsPath[] = "externrefTestData/funcs.wasm";
  HexToFile(FuncsWasm, FuncsPath);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromFile(VMCxt, FuncsPath)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VMCxt)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VMCxt)));

  // Functor instance
  SquareStruct SS;
  // Class instance
  AddClass AC;

  // Test 1: call add -- 1234 + 5678
  P[0] = WasmEdge_ValueGenExternRef(&AC);
  P[1] = WasmEdge_ValueGenI32(1234);
  P[2] = WasmEdge_ValueGenI32(5678);
  FuncName = WasmEdge_StringCreateByCString("call_add");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 3, R, 1)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(WasmEdge_ValueGetI32(R[0]), 6912);

  // Test 2: call mul -- 789 * 4321
  P[0] = WasmEdge_ValueGenExternRef(reinterpret_cast<void *>(&MulFunc));
  P[1] = WasmEdge_ValueGenI32(789);
  P[2] = WasmEdge_ValueGenI32(4321);
  FuncName = WasmEdge_StringCreateByCString("call_mul");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 3, R, 1)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(WasmEdge_ValueGetI32(R[0]), 3409269);

  // Test 3: call square -- 8256^2
  P[0] = WasmEdge_ValueGenExternRef(&SS);
  P[1] = WasmEdge_ValueGenI32(8256);
  FuncName = WasmEdge_StringCreateByCString("call_square");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 2, R, 1)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(WasmEdge_ValueGetI32(R[0]), 68161536);

  // Test 4: call sum and square -- (210 + 654)^2
  P[0] = WasmEdge_ValueGenExternRef(&AC);
  P[1] = WasmEdge_ValueGenExternRef(&SS);
  P[2] = WasmEdge_ValueGenI32(210);
  P[3] = WasmEdge_ValueGenI32(654);
  FuncName = WasmEdge_StringCreateByCString("call_add_square");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 4, R, 1)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(WasmEdge_ValueGetI32(R[0]), 746496);

  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ModuleInstanceDelete(HostMod);
}

TEST(ExternRefTest, Ref__STL) {
  WasmEdge_VMContext *VMCxt = WasmEdge_VMCreate(nullptr, nullptr);
  WasmEdge_ModuleInstanceContext *HostMod = createExternModule();
  WasmEdge_Value P[3], R[1];
  WasmEdge_String FuncName;

  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMRegisterModuleFromImport(VMCxt, HostMod)));
  char STLPath[] = "externrefTestData/stl.wasm";
  HexToFile(STLWasm, STLPath);
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMLoadWasmFromFile(VMCxt, STLPath)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMValidate(VMCxt)));
  EXPECT_TRUE(WasmEdge_ResultOK(WasmEdge_VMInstantiate(VMCxt)));

  // STL Instances
  std::stringstream STLSS;
  std::string STLStr, STLStrKey, STLStrVal;
  std::vector<uint32_t> STLVec;
  std::map<std::string, std::string> STLMap;
  std::set<uint32_t> STLSet;

  // Test 1: call ostream << std::string
  STLStr = "hello world!";
  P[0] = WasmEdge_ValueGenExternRef(&STLSS);
  P[1] = WasmEdge_ValueGenExternRef(&STLStr);
  FuncName = WasmEdge_StringCreateByCString("call_ostream_str");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 2, nullptr, 0)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_EQ(STLSS.str(), "hello world!");

  // Test 2: call ostream << uint32_t
  P[0] = WasmEdge_ValueGenExternRef(&STLSS);
  P[1] = WasmEdge_ValueGenI32(123456);
  FuncName = WasmEdge_StringCreateByCString("call_ostream_u32");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 2, nullptr, 0)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_EQ(STLSS.str(), "hello world!123456");

  // Test 3: call map insert {key, val}
  STLStrKey = "one";
  STLStrVal = "1";
  P[0] = WasmEdge_ValueGenExternRef(&STLMap);
  P[1] = WasmEdge_ValueGenExternRef(&STLStrKey);
  P[2] = WasmEdge_ValueGenExternRef(&STLStrVal);
  FuncName = WasmEdge_StringCreateByCString("call_map_insert");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 3, nullptr, 0)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(STLMap.find(STLStrKey), STLMap.end());
  EXPECT_EQ(STLMap.find(STLStrKey)->second, STLStrVal);

  // Test 4: call map erase {key}
  STLStrKey = "one";
  P[0] = WasmEdge_ValueGenExternRef(&STLMap);
  P[1] = WasmEdge_ValueGenExternRef(&STLStrKey);
  FuncName = WasmEdge_StringCreateByCString("call_map_erase");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 2, nullptr, 0)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_EQ(STLMap.find(STLStrKey), STLMap.end());

  // Test 5: call set insert {key}
  P[0] = WasmEdge_ValueGenExternRef(&STLSet);
  P[1] = WasmEdge_ValueGenI32(123456);
  FuncName = WasmEdge_StringCreateByCString("call_set_insert");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 2, nullptr, 0)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(STLSet.find(123456), STLSet.end());

  // Test 6: call set erase {key}
  STLSet.insert(3456);
  P[0] = WasmEdge_ValueGenExternRef(&STLSet);
  P[1] = WasmEdge_ValueGenI32(3456);
  FuncName = WasmEdge_StringCreateByCString("call_set_erase");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 2, nullptr, 0)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_NE(STLSet.find(123456), STLSet.end());
  EXPECT_EQ(STLSet.find(3456), STLSet.end());

  // Test 7: call vector push {val}
  STLVec = {10, 20, 30, 40, 50, 60, 70, 80, 90};
  P[0] = WasmEdge_ValueGenExternRef(&STLVec);
  P[1] = WasmEdge_ValueGenI32(100);
  FuncName = WasmEdge_StringCreateByCString("call_vector_push");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 2, nullptr, 0)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_EQ(STLVec.size(), 10U);
  EXPECT_EQ(STLVec[9], 100U);

  // Test 8: call vector[3:8) sum
  auto ItBegin = STLVec.begin() + 3;
  auto ItEnd = STLVec.end() - 2;
  P[0] = WasmEdge_ValueGenExternRef(&ItBegin);
  P[1] = WasmEdge_ValueGenExternRef(&ItEnd);
  FuncName = WasmEdge_StringCreateByCString("call_vector_sum");
  EXPECT_TRUE(
      WasmEdge_ResultOK(WasmEdge_VMExecute(VMCxt, FuncName, P, 2, R, 1)));
  WasmEdge_StringDelete(FuncName);
  EXPECT_TRUE(WasmEdge_ValTypeIsI32(R[0].Type));
  EXPECT_EQ(WasmEdge_ValueGetI32(R[0]), 40 + 50 + 60 + 70 + 80);

  WasmEdge_VMDelete(VMCxt);
  WasmEdge_ModuleInstanceDelete(HostMod);
}

} // namespace

GTEST_API_ int main(int argc, char **argv) {
  WasmEdge_LogSetErrorLevel();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
