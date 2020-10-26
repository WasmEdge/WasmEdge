// SPDX-License-Identifier: Apache-2.0
#pragma once

#include "common/errcode.h"
#include "runtime/hostfunc.h"
#include "runtime/importobj.h"
#include "runtime/instance/memory.h"

#include <cstdlib>
#include <iostream>
#include <set>

/// Test: function to pass as function pointer
uint32_t MulFunc(uint32_t A, uint32_t B) { return A * B; }

/// Test: class to pass as reference
class AddClass {
public:
  uint32_t add(uint32_t A, uint32_t B) const { return A + B; }
};

/// Test: functor to pass as reference
struct SquareStruct {
  uint32_t operator()(uint32_t Val) const { return Val * Val; }
};

namespace WasmEdge {
/// Host function to access class as reference
class ExternClassAdd : public Runtime::HostFunction<ExternClassAdd> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        ExternRef Ref, uint32_t A, uint32_t B) {
    auto &Obj = retrieveExternRef<AddClass>(Ref);
    return Obj.add(A, B);
  }
};

/// Host function to call function as function pointer
class ExternFuncMul : public Runtime::HostFunction<ExternFuncMul> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        ExternRef Ref, uint32_t A, uint32_t B) {
    auto &Obj = retrieveExternRef<decltype(MulFunc)>(Ref);
    return Obj(A, B);
  }
};

/// Host function to call functor as reference
class ExternFunctorSquare : public Runtime::HostFunction<ExternFunctorSquare> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        ExternRef Ref, uint32_t Val) {
    auto &Obj = retrieveExternRef<SquareStruct>(Ref);
    return Obj(Val);
  }
};

/// Host function to output std::string through std::ostream
class ExternSTLOStreamStr : public Runtime::HostFunction<ExternSTLOStreamStr> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, ExternRef RefOS,
                    ExternRef RefStr) {
    retrieveExternRef<std::ostream>(RefOS)
        << retrieveExternRef<std::string>(RefStr);
    return {};
  }
};

/// Host function to output uint32_t through std::ostream
class ExternSTLOStreamU32 : public Runtime::HostFunction<ExternSTLOStreamU32> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst, ExternRef RefOS,
                    uint32_t Val) {
    retrieveExternRef<std::ostream>(RefOS) << Val;
    return {};
  }
};

/// Host function to insert {key, val} to std::map<std::string, std::string>
class ExternSTLMapInsert : public Runtime::HostFunction<ExternSTLMapInsert> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    ExternRef RefMap, ExternRef RefKey, ExternRef RefVal) {
    auto &Map = retrieveExternRef<std::map<std::string, std::string>>(RefMap);
    auto &Key = retrieveExternRef<std::string>(RefKey);
    auto &Val = retrieveExternRef<std::string>(RefVal);
    Map[Key] = Val;
    return {};
  }
};

/// Host function to erase std::map<std::string, std::string> with key
class ExternSTLMapErase : public Runtime::HostFunction<ExternSTLMapErase> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    ExternRef RefMap, ExternRef RefKey) {
    auto &Map = retrieveExternRef<std::map<std::string, std::string>>(RefMap);
    auto &Key = retrieveExternRef<std::string>(RefKey);
    Map.erase(Key);
    return {};
  }
};

/// Host function to insert key to std::set<uint32_t>
class ExternSTLSetInsert : public Runtime::HostFunction<ExternSTLSetInsert> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    ExternRef RefSet, uint32_t Val) {
    auto &Set = retrieveExternRef<std::set<uint32_t>>(RefSet);
    Set.insert(Val);
    return {};
  }
};

/// Host function to erase std::set<uint32_t> with key
class ExternSTLSetErase : public Runtime::HostFunction<ExternSTLSetErase> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    ExternRef RefSet, uint32_t Val) {
    auto &Set = retrieveExternRef<std::set<uint32_t>>(RefSet);
    Set.erase(Val);
    return {};
  }
};

/// Host function to push value into std::vector<uint32_t>
class ExternSTLVectorPush : public Runtime::HostFunction<ExternSTLVectorPush> {
public:
  Expect<void> body(Runtime::Instance::MemoryInstance *MemInst,
                    ExternRef RefVec, uint32_t Val) {
    auto &Vec = retrieveExternRef<std::vector<uint32_t>>(RefVec);
    Vec.push_back(Val);
    return {};
  }
};

/// Host function to summarize value in slice of std::vector<uint32_t>
class ExternSTLVectorSum : public Runtime::HostFunction<ExternSTLVectorSum> {
public:
  Expect<uint32_t> body(Runtime::Instance::MemoryInstance *MemInst,
                        ExternRef RefItBegin, ExternRef RefItEnd) {
    auto &It = retrieveExternRef<std::vector<uint32_t>::iterator>(RefItBegin);
    auto &ItEnd = retrieveExternRef<std::vector<uint32_t>::iterator>(RefItEnd);
    uint32_t Sum = 0;
    while (It != ItEnd) {
      Sum += *It;
      It++;
    }
    return Sum;
  }
};

/// Host module to register
class ExternMod : public Runtime::ImportObject {
public:
  ExternMod() : ImportObject("extern_module") {
    addHostFunc("functor_square", std::make_unique<ExternFunctorSquare>());
    addHostFunc("class_add", std::make_unique<ExternClassAdd>());
    addHostFunc("func_mul", std::make_unique<ExternFuncMul>());
    addHostFunc("stl_ostream_str", std::make_unique<ExternSTLOStreamStr>());
    addHostFunc("stl_ostream_u32", std::make_unique<ExternSTLOStreamU32>());
    addHostFunc("stl_map_insert", std::make_unique<ExternSTLMapInsert>());
    addHostFunc("stl_map_erase", std::make_unique<ExternSTLMapErase>());
    addHostFunc("stl_set_insert", std::make_unique<ExternSTLSetInsert>());
    addHostFunc("stl_set_erase", std::make_unique<ExternSTLSetErase>());
    addHostFunc("stl_vector_push", std::make_unique<ExternSTLVectorPush>());
    addHostFunc("stl_vector_sum", std::make_unique<ExternSTLVectorSum>());
  }
  virtual ~ExternMod() = default;
};
} // namespace WasmEdge
